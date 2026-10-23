#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "operation.h"
#include "operation_log_path.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

static void report_error(const char *message)
{
    fprintf(stderr, "amcore operation logging: %s\n", message);
}

static int bind_number(sqlite3_stmt *statement, int index, double value)
{
    if (isfinite(value)) {
        return sqlite3_bind_double(statement, index, value);
    }
    return sqlite3_bind_text(statement, index,
                             isnan(value) ? "NaN" :
                             (value > 0.0 ? "+Infinity" : "-Infinity"),
                             -1, SQLITE_STATIC);
}

static const char *column_type(amcore_operation_type type)
{
    switch (type) {
    case AMCORE_OPERATION_DOUBLE: return "REAL";
    case AMCORE_OPERATION_INT64:
    case AMCORE_OPERATION_BOOL: return "INTEGER";
    case AMCORE_OPERATION_TEXT: return "TEXT";
    }
    return NULL;
}

static int bind_field(sqlite3_stmt *statement, int index,
                       const amcore_operation_field *field, int available)
{
    if (!available || field->data == NULL)
        return sqlite3_bind_null(statement, index);
    switch (field->type) {
    case AMCORE_OPERATION_DOUBLE:
        return bind_number(statement, index, *(const double *)field->data);
    case AMCORE_OPERATION_INT64:
        return sqlite3_bind_int64(statement, index, *(const int64_t *)field->data);
    case AMCORE_OPERATION_BOOL:
        return sqlite3_bind_int(statement, index, *(const int *)field->data != 0);
    case AMCORE_OPERATION_TEXT:
        return sqlite3_bind_text(statement, index, field->data, -1, SQLITE_STATIC);
    }
    return SQLITE_MISUSE;
}

static int ensure_version_column(sqlite3 *database, const char *table_name)
{
    sqlite3_stmt *statement = NULL;
    char *sql = sqlite3_mprintf("PRAGMA table_info(\"%w\")", table_name);
    int found = 0;
    int rc;

    if (sql == NULL) {
        return SQLITE_NOMEM;
    }
    rc = sqlite3_prepare_v2(database, sql, -1, &statement, NULL);
    sqlite3_free(sql);
    if (rc != SQLITE_OK) {
        return rc;
    }
    while ((rc = sqlite3_step(statement)) == SQLITE_ROW) {
        const unsigned char *name = sqlite3_column_text(statement, 1);
        if (name != NULL && strcmp((const char *)name, "version") == 0) {
            found = 1;
        }
    }
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(statement);
        return rc;
    }
    rc = sqlite3_finalize(statement);
    if (rc != SQLITE_OK || found) {
        return rc;
    }

    sql = sqlite3_mprintf(
        "ALTER TABLE \"%w\" ADD COLUMN version TEXT NOT NULL DEFAULT %Q",
        table_name, AMCORE_VERSION);
    if (sql == NULL) {
        return SQLITE_NOMEM;
    }
    rc = sqlite3_exec(database, sql, NULL, NULL, NULL);
    sqlite3_free(sql);
    return rc;
}

void amcore_operation_log_write(const amcore_operation_event *event)
{
    sqlite3 *database = NULL;
    sqlite3_stmt *statement = NULL;
    sqlite3_str *builder;
    char *path;
    char *table_name = NULL;
    char *sql = NULL;
    size_t i;
    int index = 1;
    int rc;

    path = amcore_operation_log_path(1);
    if (path == NULL) {
        return;
    }
    rc = sqlite3_open_v2(path, &database,
                         SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                         SQLITE_OPEN_FULLMUTEX, NULL);
    free(path);
    if (rc != SQLITE_OK) {
        goto cleanup;
    }
    rc = sqlite3_busy_timeout(database, 5000);
    if (rc != SQLITE_OK) {
        goto cleanup;
    }
    table_name = sqlite3_mprintf("%s_%s", event->function_name, AMCORE_VERSION);
    if (table_name == NULL) {
        rc = SQLITE_NOMEM;
        goto cleanup;
    }
    builder = sqlite3_str_new(database);
    sqlite3_str_appendf(builder, "CREATE TABLE IF NOT EXISTS \"%w\" ("
        "id INTEGER PRIMARY KEY, version TEXT NOT NULL", table_name);
    for (i = 0; i < event->input_count; ++i)
        sqlite3_str_appendf(builder, ", \"%w\" %s", event->inputs[i].name,
                            column_type(event->inputs[i].type));
    sqlite3_str_appendall(builder,
        ", output_is_null INTEGER NOT NULL, return_value INTEGER NOT NULL");
    for (i = 0; i < event->output_count; ++i)
        sqlite3_str_appendf(builder, ", \"%w\" %s", event->outputs[i].name,
                            column_type(event->outputs[i].type));
    sqlite3_str_appendall(builder, ", executed_at_us INTEGER NOT NULL, "
        "duration_ns INTEGER NOT NULL CHECK(duration_ns >= 0))");
    sql = sqlite3_str_finish(builder);
    if (sql == NULL) {
        rc = SQLITE_NOMEM;
        goto cleanup;
    }
    rc = sqlite3_exec(database, sql, NULL, NULL, NULL);
    sqlite3_free(sql);
    sql = NULL;
    if (rc != SQLITE_OK) {
        goto cleanup;
    }
    rc = ensure_version_column(database, table_name);
    if (rc != SQLITE_OK) {
        goto cleanup;
    }
    builder = sqlite3_str_new(database);
    sqlite3_str_appendf(builder, "INSERT INTO \"%w\" ("
        "version, output_is_null, return_value, executed_at_us, duration_ns",
        table_name);
    for (i = 0; i < event->input_count; ++i)
        sqlite3_str_appendf(builder, ", \"%w\"", event->inputs[i].name);
    for (i = 0; i < event->output_count; ++i)
        sqlite3_str_appendf(builder, ", \"%w\"", event->outputs[i].name);
    sqlite3_str_appendall(builder, ") VALUES (?, ?, ?, ?, ?");
    for (i = 0; i < event->input_count; ++i)
        sqlite3_str_appendall(builder, ", ?");
    for (i = 0; i < event->output_count; ++i)
        sqlite3_str_appendall(builder, ", ?");
    sqlite3_str_appendall(builder, ")");
    sql = sqlite3_str_finish(builder);
    if (sql == NULL) {
        rc = SQLITE_NOMEM;
        goto cleanup;
    }
    rc = sqlite3_prepare_v2(database, sql, -1, &statement, NULL);
    if (rc != SQLITE_OK) {
        goto cleanup;
    }
    rc = sqlite3_bind_text(statement, index++, AMCORE_VERSION, -1, SQLITE_STATIC);
    if (rc == SQLITE_OK) {
        rc = sqlite3_bind_int(statement, index++, event->output_is_null);
    }
    if (rc == SQLITE_OK) {
        rc = sqlite3_bind_int(statement, index++, event->status);
    }
    if (rc == SQLITE_OK) {
        rc = sqlite3_bind_int64(statement, index++, event->executed_at_us);
    }
    if (rc == SQLITE_OK) {
        rc = sqlite3_bind_int64(statement, index++, event->duration_ns);
    }
    for (i = 0; rc == SQLITE_OK && i < event->input_count; ++i)
        rc = bind_field(statement, index++, &event->inputs[i], 1);
    for (i = 0; rc == SQLITE_OK && i < event->output_count; ++i)
        rc = bind_field(statement, index++, &event->outputs[i],
                        event->status == AMCORE_STATUS_SUCCESS);
    if (rc == SQLITE_OK) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_DONE) {
            rc = SQLITE_OK;
        }
    }

cleanup:
    sqlite3_free(sql);
    sqlite3_free(table_name);
    if (rc != SQLITE_OK) {
        report_error(database != NULL ? sqlite3_errmsg(database) : sqlite3_errstr(rc));
    }
    rc = sqlite3_finalize(statement);
    if (rc != SQLITE_OK) {
        report_error(sqlite3_errstr(rc));
    }
    rc = sqlite3_close(database);
    if (rc != SQLITE_OK) {
        report_error(sqlite3_errstr(rc));
    }
}
