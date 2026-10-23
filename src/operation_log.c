#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "operation.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#endif

static void report_error(const char *message)
{
    fprintf(stderr, "amcore operation logging: %s\n", message);
}

#ifdef _WIN32
static char *local_data_root(void)
{
    const wchar_t *root = _wgetenv(L"LOCALAPPDATA");
    char *utf8;
    int length;

    if (root == NULL || root[0] == L'\0' ||
        !((root[0] != L'\0' && root[1] == L':' &&
           (root[2] == L'\\' || root[2] == L'/')) ||
          (root[0] == L'\\' && root[1] == L'\\'))) {
        report_error("LOCALAPPDATA must specify an absolute directory");
        return NULL;
    }
    length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, root, -1,
                                NULL, 0, NULL, NULL);
    if (length == 0) {
        report_error("cannot encode LOCALAPPDATA as UTF-8");
        return NULL;
    }
    utf8 = malloc((size_t)length);
    if (utf8 == NULL) {
        report_error("cannot allocate database path");
        return NULL;
    }
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, root, -1,
                             utf8, length, NULL, NULL)) {
        report_error("cannot encode LOCALAPPDATA as UTF-8");
        free(utf8);
        return NULL;
    }
    return utf8;
}

static int make_directory(const char *path)
{
    int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path,
                                    -1, NULL, 0);
    wchar_t *wide;
    DWORD attributes;
    int success;

    if (length == 0) {
        return 0;
    }
    wide = malloc((size_t)length * sizeof(*wide));
    if (wide == NULL) {
        return 0;
    }
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1,
                            wide, length)) {
        free(wide);
        return 0;
    }
    success = CreateDirectoryW(wide, NULL) != 0;
    if (!success && GetLastError() == ERROR_ALREADY_EXISTS) {
        attributes = GetFileAttributesW(wide);
        success = attributes != INVALID_FILE_ATTRIBUTES &&
                  (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }
    free(wide);
    return success;
}
#else
static char *local_data_root(void)
{
    const char *root;
    const char *suffix;
    char *path;

#ifdef __APPLE__
    root = getenv("HOME");
    suffix = "/Library/Application Support";
#else
    root = getenv("XDG_DATA_HOME");
    suffix = "";
    if (root == NULL || root[0] != '/') {
        root = getenv("HOME");
        suffix = "/.local/share";
    }
#endif
    if (root == NULL || root[0] != '/') {
        report_error("user data location must specify an absolute directory");
        return NULL;
    }
    path = malloc(strlen(root) + strlen(suffix) + 1);
    if (path == NULL) {
        report_error("cannot allocate database path");
        return NULL;
    }
    snprintf(path, strlen(root) + strlen(suffix) + 1, "%s%s", root, suffix);
    return path;
}

static int make_directory(const char *path)
{
    struct stat info;
    if (mkdir(path, 0700) == 0) {
        return 1;
    }
    return errno == EEXIST && stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}
#endif

static int ensure_directory(char *path)
{
    char *separator;
    int success;

    if (make_directory(path)) {
        return 1;
    }
    separator = strrchr(path, '/');
#ifdef _WIN32
    {
        char *backslash = strrchr(path, '\\');
        if (backslash != NULL && (separator == NULL || backslash > separator)) {
            separator = backslash;
        }
    }
#endif
    if (separator == NULL || separator == path) {
        return 0;
    }
    {
        char saved = *separator;
        *separator = '\0';
        success = ensure_directory(path);
        *separator = saved;
    }
    return success && make_directory(path);
}

static char *database_path(void)
{
    char *root = local_data_root();
    char *path;
    size_t directory_length;
    size_t path_capacity;
#ifdef _WIN32
    const char *directory = "\\" AMCORE_LOG_PROJECT_NAME;
    const char *filename = "\\" AMCORE_LOG_PROJECT_NAME ".db";
#else
    const char *directory = "/" AMCORE_LOG_PROJECT_NAME;
    const char *filename = "/" AMCORE_LOG_PROJECT_NAME ".db";
#endif

    if (root == NULL) {
        return NULL;
    }
    directory_length = strlen(root) + strlen(directory);
    path_capacity = directory_length + strlen(filename) + 1;
    path = malloc(path_capacity);
    if (path == NULL) {
        report_error("cannot allocate database path");
        free(root);
        return NULL;
    }
    snprintf(path, path_capacity, "%s%s", root, directory);
    free(root);
    if (!ensure_directory(path)) {
        report_error("cannot create user data directory");
        free(path);
        return NULL;
    }
    snprintf(path + directory_length, path_capacity - directory_length,
             "%s", filename);
    return path;
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

void amcore_operation_log_write(const amcore_operation_event *event)
{
    sqlite3 *database = NULL;
    sqlite3_stmt *statement = NULL;
    sqlite3_str *builder;
    char *path;
    char *sql = NULL;
    size_t i;
    int index = 1;
    int rc;

    path = database_path();
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
    builder = sqlite3_str_new(database);
    sqlite3_str_appendf(builder, "CREATE TABLE IF NOT EXISTS \"%w\" ("
        "id INTEGER PRIMARY KEY", event->function_name);
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
    builder = sqlite3_str_new(database);
    sqlite3_str_appendf(builder, "INSERT INTO \"%w\" ("
        "output_is_null, return_value, executed_at_us, duration_ns",
        event->function_name);
    for (i = 0; i < event->input_count; ++i)
        sqlite3_str_appendf(builder, ", \"%w\"", event->inputs[i].name);
    for (i = 0; i < event->output_count; ++i)
        sqlite3_str_appendf(builder, ", \"%w\"", event->outputs[i].name);
    sqlite3_str_appendall(builder, ") VALUES (?, ?, ?, ?");
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
    rc = sqlite3_bind_int(statement, index++, event->output_is_null);
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
