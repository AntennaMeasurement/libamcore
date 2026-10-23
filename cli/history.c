#include "history.h"
#include "operation_log_path.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

static int parse_positive_integer(const char *text, const char *name,
                                  sqlite3_int64 *value_out)
{
    const char *p;
    char *end;
    long long value;

    for (p = text; *p >= '0' && *p <= '9'; ++p) {}
    errno = 0;
    value = strtoll(text, &end, 10);
    if (p == text || *p != '\0' || *end != '\0' ||
        errno == ERANGE || value <= 0) {
        fprintf(stderr, "amcore history: %s must be a positive integer "
                        "no greater than 9223372036854775807; got '%s'\n", name, text);
        return 0;
    }
    *value_out = (sqlite3_int64)value;
    return 1;
}

static char *display_text(const unsigned char *text)
{
    sqlite3_str *builder;
    const unsigned char *p;
    if (text == NULL)
        text = (const unsigned char *)"NULL";
    if (*text == '\0')
        return sqlite3_mprintf("%s", "");
    builder = sqlite3_str_new(NULL);
    for (p = text; *p != '\0'; ++p) {
        if (*p < 32 || *p == 127 || *p == '\\' || *p == '|')
            sqlite3_str_appendf(builder, "\\x%02X", (unsigned int)*p);
        else
            sqlite3_str_appendchar(builder, 1, (char)*p);
    }
    return sqlite3_str_finish(builder);
}

static void print_border(const int *widths, int count)
{
    int i, j;
    putchar('+');
    for (i = 0; i < count; ++i) {
        for (j = 0; j < widths[i] + 2; ++j)
            putchar('-');
        putchar('+');
    }
    putchar('\n');
}

static char *display_column(sqlite3_stmt *statement, int column)
{
    if (sqlite3_column_type(statement, column) == SQLITE_NULL)
        return display_text(NULL);
    {
        const unsigned char *text = sqlite3_column_text(statement, column);
        return text == NULL ? NULL : display_text(text);
    }
}

static int render_table(sqlite3_stmt *statement)
{
    int count = sqlite3_column_count(statement);
    int *widths = calloc((size_t)count, sizeof(*widths));
    int i, rc;
    if (widths == NULL)
        return SQLITE_NOMEM;
    for (i = 0; i < count; ++i)
        widths[i] = (int)strlen(sqlite3_column_name(statement, i));
    while ((rc = sqlite3_step(statement)) == SQLITE_ROW) {
        for (i = 0; i < count; ++i) {
            char *text = display_column(statement, i);
            int length;
            if (text == NULL) {
                rc = SQLITE_NOMEM;
                goto cleanup;
            }
            length = (int)strlen(text);
            if (length > widths[i])
                widths[i] = length;
            sqlite3_free(text);
        }
    }
    if (rc != SQLITE_DONE)
        goto cleanup;
    rc = sqlite3_reset(statement);
    if (rc != SQLITE_OK)
        goto cleanup;
    print_border(widths, count);
    putchar('|');
    for (i = 0; i < count; ++i)
        printf(" %-*s |", widths[i], sqlite3_column_name(statement, i));
    putchar('\n');
    print_border(widths, count);
    while ((rc = sqlite3_step(statement)) == SQLITE_ROW) {
        putchar('|');
        for (i = 0; i < count; ++i) {
            char *text = display_column(statement, i);
            if (text == NULL) {
                rc = SQLITE_NOMEM;
                goto cleanup;
            }
            printf(" %-*s |", widths[i], text);
            sqlite3_free(text);
        }
        putchar('\n');
    }
    if (rc == SQLITE_DONE) {
        print_border(widths, count);
        rc = SQLITE_OK;
    }
cleanup:
    free(widths);
    return rc;
}

static int collect_table(sqlite3 *database, const char *table,
                         sqlite3_int64 limit, const char *filter,
                         sqlite3_int64 id)
{
    static const char *required[] = {
        "id", "executed_at_us", "duration_ns", "return_value"
    };
    sqlite3_stmt *statement = NULL;
    char *sql = sqlite3_mprintf("PRAGMA main.table_info(\"%w\")", table);
    const char *suffix = strrchr(table, '_');
    const char *version = "unknown";
    char *function = NULL;
    unsigned int columns = 0;
    int has_version = 0;
    int rc;
    size_t i;

    if (filter != NULL) {
        size_t length = suffix != NULL && suffix[1] >= '0' && suffix[1] <= '9'
                      ? (size_t)(suffix - table) : strlen(table);
        if (strlen(filter) != length || strncmp(filter, table, length) != 0) {
            sqlite3_free(sql);
            return SQLITE_OK;
        }
    }
    if (sql == NULL)
        return SQLITE_NOMEM;
    rc = sqlite3_prepare_v2(database, sql, -1, &statement, NULL);
    sqlite3_free(sql);
    if (rc != SQLITE_OK)
        return rc;
    while ((rc = sqlite3_step(statement)) == SQLITE_ROW) {
        const char *name = (const char *)sqlite3_column_text(statement, 1);
        if (name == NULL) {
            rc = SQLITE_NOMEM;
            break;
        }
        for (i = 0; i < sizeof(required) / sizeof(required[0]); ++i)
            if (strcmp(name, required[i]) == 0)
                columns |= 1u << i;
        if (strcmp(name, "version") == 0)
            has_version = 1;
    }
    sqlite3_finalize(statement);
    statement = NULL;
    if (rc != SQLITE_DONE)
        return rc;
    if (columns != 15u) {
        fprintf(stderr, "amcore history: incompatible log table '%s'\n", table);
        return SQLITE_SCHEMA;
    }

    if (suffix != NULL && suffix[1] >= '0' && suffix[1] <= '9') {
        version = suffix + 1;
        function = sqlite3_mprintf("%.*s", (int)(suffix - table), table);
    } else {
        function = sqlite3_mprintf("%s", table);
    }
    if (function == NULL)
        return SQLITE_NOMEM;

    /* Each table can contribute at most limit rows to the global result. */
    sql = sqlite3_mprintf(
        "INSERT INTO temp.amcore_history "
        "SELECT %Q, %s, %Q, id, executed_at_us, return_value, duration_ns "
        "FROM main.\"%w\" WHERE (%lld = 0 OR id = %lld) "
        "ORDER BY executed_at_us DESC, id DESC LIMIT ?",
        function, has_version ? "\"version\"" : "?",
        table, table, (long long)id, (long long)id);
    sqlite3_free(function);
    if (sql == NULL)
        return SQLITE_NOMEM;
    rc = sqlite3_prepare_v2(database, sql, -1, &statement, NULL);
    sqlite3_free(sql);
    if (rc == SQLITE_OK && !has_version)
        rc = sqlite3_bind_text(statement, 1, version, -1, SQLITE_TRANSIENT);
    if (rc == SQLITE_OK)
        rc = sqlite3_bind_int64(statement, has_version ? 1 : 2, limit);
    if (rc == SQLITE_OK) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_DONE)
            rc = SQLITE_OK;
    }
    {
        int finalized = sqlite3_finalize(statement);
        return rc == SQLITE_OK ? finalized : rc;
    }
}

static int print_details(sqlite3 *database, sqlite3_stmt *calls)
{
    int rc;
    while ((rc = sqlite3_step(calls)) == SQLITE_ROW) {
        sqlite3_stmt *row = NULL;
        sqlite3_stmt *fields = NULL;
        sqlite3_stmt *insert = NULL;
        const char *table = (const char *)sqlite3_column_text(calls, 6);
        char *sql;
        int i;
        if (table == NULL)
            return SQLITE_NOMEM;
        sql = sqlite3_mprintf("SELECT * FROM main.\"%w\" WHERE id = ?", table);
        if (sql == NULL)
            return SQLITE_NOMEM;
        rc = sqlite3_prepare_v2(database, sql, -1, &row, NULL);
        sqlite3_free(sql);
        if (rc == SQLITE_OK)
            rc = sqlite3_bind_int64(row, 1, sqlite3_column_int64(calls, 0));
        if (rc == SQLITE_OK) {
            rc = sqlite3_step(row);
            if (rc == SQLITE_ROW)
                rc = SQLITE_OK;
            else if (rc == SQLITE_DONE)
                rc = SQLITE_NOTFOUND;
        }
        if (rc == SQLITE_OK)
            rc = sqlite3_exec(database,
                "CREATE TEMP TABLE IF NOT EXISTS amcore_details (field TEXT, value TEXT);"
                "DELETE FROM temp.amcore_details", NULL, NULL, NULL);
        if (rc == SQLITE_OK)
            rc = sqlite3_prepare_v2(database,
                "INSERT INTO temp.amcore_details VALUES (?, ?)",
                -1, &insert, NULL);
        for (i = 0; rc == SQLITE_OK && i < sqlite3_column_count(row); ++i) {
            rc = sqlite3_bind_text(insert, 1, sqlite3_column_name(row, i),
                                   -1, SQLITE_TRANSIENT);
            if (rc == SQLITE_OK) {
                if (sqlite3_column_type(row, i) == SQLITE_NULL)
                    rc = sqlite3_bind_null(insert, 2);
                else {
                    const unsigned char *value = sqlite3_column_text(row, i);
                    rc = value == NULL ? SQLITE_NOMEM :
                        sqlite3_bind_text(insert, 2, (const char *)value,
                                          -1, SQLITE_TRANSIENT);
                }
            }
            if (rc == SQLITE_OK) {
                rc = sqlite3_step(insert);
                if (rc == SQLITE_DONE)
                    rc = sqlite3_reset(insert);
            }
        }
        sqlite3_finalize(insert);
        sqlite3_finalize(row);
        if (rc == SQLITE_OK)
            rc = sqlite3_prepare_v2(database,
                "SELECT field AS Field, value AS Value FROM temp.amcore_details ORDER BY rowid",
                -1, &fields, NULL);
        if (rc == SQLITE_OK) {
            char *function = display_column(calls, 1);
            char *version = display_column(calls, 2);
            if (function == NULL || version == NULL)
                rc = SQLITE_NOMEM;
            else {
                printf("Function: %s  Version: %s  ID: %lld\n", function, version,
                       (long long)sqlite3_column_int64(calls, 0));
                rc = render_table(fields);
            }
            sqlite3_free(function);
            sqlite3_free(version);
        }
        sqlite3_finalize(fields);
        if (rc != SQLITE_OK)
            return rc;
    }
    return rc == SQLITE_DONE ? SQLITE_OK : rc;
}

static int print_history(sqlite3 *database, sqlite3_int64 limit,
                         const char *function, sqlite3_int64 id)
{
    sqlite3_stmt *statement = NULL;
    int rc = sqlite3_exec(database,
        "BEGIN;"
        "CREATE TEMP TABLE amcore_history ("
        "function TEXT, version TEXT, source_table TEXT, id INTEGER, "
        "executed_at_us INTEGER, return_value INTEGER, duration_ns INTEGER)",
        NULL, NULL, NULL);
    if (rc != SQLITE_OK)
        return rc;
    rc = sqlite3_prepare_v2(database,
        "SELECT name FROM main.sqlite_master "
        "WHERE type='table' AND name GLOB 'amcore_*' ORDER BY name",
        -1, &statement, NULL);
    if (rc != SQLITE_OK)
        return rc;
    while ((rc = sqlite3_step(statement)) == SQLITE_ROW) {
        const char *table = (const char *)sqlite3_column_text(statement, 0);
        rc = table == NULL ? SQLITE_NOMEM :
             collect_table(database, table, limit, function, id);
        if (rc != SQLITE_OK)
            break;
    }
    sqlite3_finalize(statement);
    statement = NULL;
    if (rc != SQLITE_DONE)
        return rc;

    rc = sqlite3_prepare_v2(database,
        id != 0 ?
        "SELECT id AS ID, function AS Function, version AS Version, "
        "executed_at_us, return_value, duration_ns, source_table "
        "FROM temp.amcore_history "
        "ORDER BY executed_at_us DESC, source_table ASC, id DESC LIMIT ?" :
        "SELECT id AS ID, function AS Function, version AS Version, "
        "executed_at_us, return_value, duration_ns FROM temp.amcore_history "
        "ORDER BY executed_at_us DESC, source_table ASC, id DESC LIMIT ?",
        -1, &statement, NULL);
    if (rc == SQLITE_OK)
        rc = sqlite3_bind_int64(statement, 1, limit);
    if (rc != SQLITE_OK) {
        sqlite3_finalize(statement);
        return rc;
    }
    if (id != 0) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_DONE) {
            fprintf(stderr, "amcore history: no call found for function '%s' with ID %lld\n",
                    function, (long long)id);
            rc = SQLITE_NOTFOUND;
        } else if (rc == SQLITE_ROW) {
            rc = sqlite3_reset(statement);
            if (rc == SQLITE_OK)
                rc = print_details(database, statement);
        }
    } else {
        rc = render_table(statement);
    }
    {
        int finalized = sqlite3_finalize(statement);
        return rc == SQLITE_OK ? finalized : rc;
    }
}

int amcore_cli_history(int argc, char **argv)
{
    sqlite3_int64 limit = 10;
    sqlite3 *database = NULL;
    char *path;
    int rc;
    int i;
    int has_limit = 0;
    sqlite3_int64 id = 0;
    const char *function = NULL;
    int result = EXIT_FAILURE;

    for (i = 0; i < argc; ++i) {
        const char *value = NULL;
        const char *option = NULL;
        if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--limit") == 0) {
            option = "limit";
            if (i + 1 < argc)
                value = argv[++i];
        } else if (strncmp(argv[i], "--limit=", 8) == 0) {
            option = "limit";
            value = argv[i] + 8;
        } else if (strcmp(argv[i], "--id") == 0 ||
                   strcmp(argv[i], "--function") == 0) {
            option = strcmp(argv[i], "--id") == 0 ? "id" : "function";
            if (i + 1 < argc)
                value = argv[++i];
        } else if (strncmp(argv[i], "--id=", 5) == 0) {
            option = "id";
            value = argv[i] + 5;
        } else if (strncmp(argv[i], "--function=", 11) == 0) {
            option = "function";
            value = argv[i] + 11;
        }
        if (value == NULL || value[0] == '\0' ||
            (strcmp(option, "limit") == 0 && has_limit) ||
            (strcmp(option, "id") == 0 && id != 0) ||
            (strcmp(option, "function") == 0 && function != NULL)) {
            fputs("Usage: amcore history [--function <name>] "
                  "[-l <limit> | --limit <limit> | --id <id>]\n", stderr);
            return EXIT_FAILURE;
        }
        if (strcmp(option, "function") == 0) {
            const char *p;
            for (p = value; (*p >= 'a' && *p <= 'z') ||
                 (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') ||
                 *p == '_'; ++p) {}
            if (strncmp(value, "amcore_", 7) != 0 || value[7] == '\0' || *p != '\0') {
                fputs("amcore history: function must be a library function name (amcore_...)\n",
                      stderr);
                return EXIT_FAILURE;
            }
            function = value;
        } else if (strcmp(option, "id") == 0) {
            if (!parse_positive_integer(value, "id", &id))
                return EXIT_FAILURE;
        } else {
            if (!parse_positive_integer(value, "limit", &limit))
                return EXIT_FAILURE;
            has_limit = 1;
        }
    }
    if (id != 0 && (function == NULL || has_limit)) {
        fputs("amcore history: --id requires --function and cannot be combined with --limit\n",
              stderr);
        return EXIT_FAILURE;
    }
    if (id != 0)
        limit = 9223372036854775807LL;

    path = amcore_operation_log_path(0);
    if (path == NULL)
        return EXIT_FAILURE;
    rc = sqlite3_open_v2(path, &database,
                       SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "amcore history: cannot open '%s': %s\n", path,
                database != NULL ? sqlite3_errmsg(database) : sqlite3_errstr(rc));
    } else {
        rc = sqlite3_busy_timeout(database, 5000);
        if (rc == SQLITE_OK)
            rc = print_history(database, limit, function, id);
        if (rc != SQLITE_OK)
            fprintf(stderr, "amcore history: %s (%s)\n",
                    sqlite3_errstr(rc), sqlite3_errmsg(database));
        else if (ferror(stdout))
            fputs("amcore history: cannot write history output\n", stderr);
        else
            result = EXIT_SUCCESS;
    }
    free(path);
    rc = sqlite3_close(database);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "amcore history: %s\n", sqlite3_errstr(rc));
        result = EXIT_FAILURE;
    }
    return result;
}
