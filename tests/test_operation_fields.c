#include "operation_log.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYNTHETIC_TABLE "synthetic_operation_" AMCORE_VERSION
#define EMPTY_TABLE "no_fields_" AMCORE_VERSION
#define MANY_TABLE "many_fields_" AMCORE_VERSION

#ifdef AMCORE_ENABLE_OPERATION_LOGGING
#include <sqlite3.h>
#endif
#ifdef AMCORE_ENABLE_TELEMETRY
#include "operation.h"
#include <cjson/cJSON.h>
#endif

static char label[5000];
static int sent;
static int telemetry_valid = 1;

#ifdef AMCORE_ENABLE_TELEMETRY
static int testing_allocation;
static int allocation_sends;
static int allocations_left;
static int live_allocations;

static void *test_allocate(size_t size)
{
    void *memory;
    if (allocations_left == 0) {
        return NULL;
    }
    --allocations_left;
    memory = malloc(size);
    if (memory != NULL) {
        ++live_allocations;
    }
    return memory;
}

static void test_free(void *memory)
{
    if (memory != NULL) {
        --live_allocations;
        free(memory);
    }
}

void amcore_telemetry_send(const char *json)
{
    cJSON *object;
    cJSON *inputs;

    if (testing_allocation) {
        ++allocation_sends;
        telemetry_valid = telemetry_valid &&
            strstr(json, "\"executed_at_us\":9223372036854775807") != NULL &&
            strstr(json, "\"duration_ns\":9223372036854775807") != NULL &&
            strstr(json, "\"version\":\"" AMCORE_VERSION "\"") != NULL &&
            strstr(json, "\"minimum\":-9223372036854775808") != NULL;
        return;
    }
    object = cJSON_Parse(json);
    inputs = cJSON_GetObjectItemCaseSensitive(object, "inputs");
    telemetry_valid = telemetry_valid && cJSON_IsObject(object) &&
                      cJSON_IsObject(inputs);
    ++sent;
    if (sent <= 3) {
        cJSON *text = cJSON_GetObjectItemCaseSensitive(inputs, "label");
        telemetry_valid = telemetry_valid &&
            strlen(json) > 5000 &&
            strstr(json, "\"version\":\"" AMCORE_VERSION "\"") != NULL &&
            strstr(json, "\"function\":\"synthetic_operation\"") != NULL &&
            strstr(json, "\"count\":9223372036854775807") != NULL &&
            strstr(json, "\"enabled\":true") != NULL &&
            strstr(json, "\"optional\":null") != NULL &&
            cJSON_IsString(text) && strcmp(text->valuestring, label) == 0 &&
            strstr(json, "\"special\":\"+Infinity\"") != NULL;
        if (sent == 1)
            telemetry_valid = telemetry_valid &&
                strstr(json, "\"output\":{\"value\":2.5,\"accepted\":false}") != NULL;
        else if (sent == 2)
            telemetry_valid = telemetry_valid &&
                strstr(json, "\"output\":null") != NULL;
        else
            telemetry_valid = telemetry_valid &&
                strstr(json, "\"output_is_null\":true") != NULL &&
                strstr(json, "\"output\":{\"value\":null,\"accepted\":false}") != NULL;
    } else if (sent == 4) {
        telemetry_valid = telemetry_valid &&
            strstr(json, "\"inputs\":{},\"output_is_null\":false") != NULL &&
            strstr(json, "\"output\":{}") != NULL;
    } else {
        telemetry_valid = telemetry_valid && strlen(json) > 1024 &&
            strstr(json, "\"function\":\"many_fields\"") != NULL &&
            strstr(json, "\"field_79\":79") != NULL;
    }
    cJSON_Delete(object);
}

static int allocation_failures(void)
{
    int64_t minimum = INT64_MIN;
    double value = 1.25;
    int enabled = 1;
    const amcore_operation_field inputs[] = {
        {"label", AMCORE_OPERATION_TEXT, label},
        {"minimum", AMCORE_OPERATION_INT64, &minimum},
        {"value", AMCORE_OPERATION_DOUBLE, &value},
        {"enabled", AMCORE_OPERATION_BOOL, &enabled}
    };
    const amcore_operation_event event = {
        "allocation_test", inputs, 4, inputs + 2, 1, 0,
        AMCORE_STATUS_SUCCESS, INT64_MAX, INT64_MAX
    };
    cJSON_Hooks hooks = {test_allocate, test_free};
    int attempt;
    int success = 0;

    testing_allocation = 1;
    cJSON_InitHooks(&hooks);
    for (attempt = 0; attempt < 256; ++attempt) {
        allocations_left = attempt;
        amcore_telemetry_write(&event);
        if (live_allocations != 0) {
            break;
        }
        if (allocation_sends != 0) {
            success = allocation_sends == 1 && telemetry_valid;
            break;
        }
    }
    cJSON_InitHooks(NULL);
    testing_allocation = 0;
    return success;
}
#endif

static amcore_status synthetic_operation(double *value, int *accepted,
                                         amcore_status status)
{
    double input = 1.25;
    double special = INFINITY;
    int64_t count = INT64_MAX;
    int enabled = 7;
    AMCORE_OPERATION_FIELDS(inputs,
        {"input", AMCORE_OPERATION_DOUBLE, &input},
        {"count", AMCORE_OPERATION_INT64, &count},
        {"enabled", AMCORE_OPERATION_BOOL, &enabled},
        {"label", AMCORE_OPERATION_TEXT, label},
        {"special", AMCORE_OPERATION_DOUBLE, &special},
        {"optional", AMCORE_OPERATION_TEXT, NULL});
    AMCORE_OPERATION_FIELDS(outputs,
        {"value", AMCORE_OPERATION_DOUBLE, value},
        {"accepted", AMCORE_OPERATION_BOOL, accepted});
    AMCORE_OPERATION_BEGIN(inputs, 6, outputs, 2);
    if (status == AMCORE_STATUS_SUCCESS) {
        if (value != NULL) *value = input * 2;
        if (accepted != NULL) *accepted = 0;
    }
    AMCORE_OPERATION_RETURN(status);
}

static amcore_status no_fields(void)
{
    AMCORE_OPERATION_BEGIN(NULL, 0, NULL, 0);
    AMCORE_OPERATION_RETURN(AMCORE_STATUS_SUCCESS);
}

static amcore_status many_fields(void)
{
    amcore_operation_field fields[80];
    char names[80][24];
    int64_t values[80];
    size_t i;
    for (i = 0; i < 80; ++i) {
        snprintf(names[i], sizeof(names[i]), "field_%lu", (unsigned long)i);
        values[i] = (int64_t)i;
        fields[i].name = names[i];
        fields[i].type = AMCORE_OPERATION_INT64;
        fields[i].data = &values[i];
    }
    {
        AMCORE_OPERATION_BEGIN(fields, 80, NULL, 0);
        AMCORE_OPERATION_RETURN(AMCORE_STATUS_SUCCESS);
    }
}

static int invalid_fields(void)
{
    const amcore_operation_field reserved = {
        "VERSION", AMCORE_OPERATION_DOUBLE, NULL
    };
    const amcore_operation_field duplicate[] = {
        {"value", AMCORE_OPERATION_DOUBLE, NULL},
        {"VALUE", AMCORE_OPERATION_DOUBLE, NULL}
    };
    amcore_operation_start start = amcore_operation_begin();
    return
        amcore_operation_end(start, "invalid_fields", &reserved, 1, NULL, 0,
                             AMCORE_STATUS_OUT_OF_RANGE) ==
            AMCORE_STATUS_OUT_OF_RANGE &&
        amcore_operation_end(start, "invalid_fields", duplicate, 2, NULL, 0,
                             AMCORE_STATUS_SUCCESS) == AMCORE_STATUS_SUCCESS &&
        amcore_operation_end(start, "invalid_fields", NULL, 1, NULL, 0,
                             AMCORE_STATUS_SUCCESS) == AMCORE_STATUS_SUCCESS &&
        amcore_operation_end(start, "invalid-function", NULL, 0, NULL, 0,
                             AMCORE_STATUS_SUCCESS) == AMCORE_STATUS_SUCCESS;
}

#ifdef AMCORE_ENABLE_OPERATION_LOGGING
static int create_legacy_table(const char *path)
{
    sqlite3 *database = NULL;
    int success = sqlite3_open(path, &database) == SQLITE_OK;

    if (success) {
        success = sqlite3_exec(database,
            "CREATE TABLE \"" SYNTHETIC_TABLE "\" ("
            "id INTEGER PRIMARY KEY, \"count\" INTEGER, input REAL, "
            "enabled INTEGER, label TEXT, special REAL, optional TEXT, "
            "output_is_null INTEGER NOT NULL, return_value INTEGER NOT NULL, "
            "value REAL, accepted INTEGER, executed_at_us INTEGER NOT NULL, "
            "duration_ns INTEGER NOT NULL)",
            NULL, NULL, NULL) == SQLITE_OK;
    }
    if (database != NULL && sqlite3_close(database) != SQLITE_OK) {
        success = 0;
    }
    return success;
}

static int check_database(const char *path)
{
    sqlite3 *database = NULL;
    sqlite3_stmt *statement = NULL;
    int success = sqlite3_open(path, &database) == SQLITE_OK;
    const char *sql =
        "SELECT count(*)=3 AND "
        "sum(version='" AMCORE_VERSION "')=3 AND "
        "sum(\"count\"=9223372036854775807 AND typeof(\"count\")='integer')=3 AND "
        "sum(version='" AMCORE_VERSION "' AND input=1.25 AND enabled=1 AND length(label)=4998 AND "
        "special='+Infinity' AND optional IS NULL)=3 AND "
        "sum(version='" AMCORE_VERSION "' AND return_value=0 AND value=2.5 AND accepted=0)=1 AND "
        "sum(return_value=1 AND value IS NULL AND accepted IS NULL)=1 AND "
        "sum(output_is_null=1 AND value IS NULL AND accepted=0)=1 "
        "FROM \"" SYNTHETIC_TABLE "\"";
    if (success)
        success = sqlite3_prepare_v2(database, sql, -1, &statement, NULL) == SQLITE_OK;
    if (success)
        success = sqlite3_step(statement) == SQLITE_ROW &&
                  sqlite3_column_int(statement, 0) == 1;
    sqlite3_finalize(statement);
    statement = NULL;
    if (success)
        success = sqlite3_prepare_v2(database,
            "SELECT count(*)=1 AND output_is_null=0 AND return_value=0 FROM \""
            EMPTY_TABLE "\"",
            -1, &statement, NULL) == SQLITE_OK;
    if (success)
        success = sqlite3_step(statement) == SQLITE_ROW &&
                  sqlite3_column_int(statement, 0) == 1;
    if (!success)
        fprintf(stderr, "generic operation database check failed: %s\n",
                sqlite3_errmsg(database));
    sqlite3_finalize(statement);
    statement = NULL;
    if (success)
        success = sqlite3_prepare_v2(database,
            "SELECT field_0=0 AND field_79=79 FROM \"" MANY_TABLE "\"",
            -1, &statement, NULL) == SQLITE_OK;
    if (success)
        success = sqlite3_step(statement) == SQLITE_ROW &&
                  sqlite3_column_int(statement, 0) == 1;
    sqlite3_finalize(statement);
    sqlite3_close(database);
    return success;
}
#endif

int main(int argc, char **argv)
{
    double value = -1.0;
    double unwritten;
    int unwritten_flag;
    int accepted = 1;
    int success;

    memset(label, 'x', sizeof(label) - 1);
    label[0] = '"';
    label[1] = '\\';
    label[2] = '\n';
    label[3] = (char)0xc3;
    label[4] = (char)0xbc;
#ifdef AMCORE_ENABLE_OPERATION_LOGGING
    success = argc == 2 && create_legacy_table(argv[1]);
#else
    success = 1;
#endif
    success = success &&
        synthetic_operation(&value, &accepted, AMCORE_STATUS_SUCCESS) ==
                  AMCORE_STATUS_SUCCESS && value == 2.5 && accepted == 0;
    success = success &&
        synthetic_operation(&unwritten, &unwritten_flag, AMCORE_STATUS_INVALID_ARGUMENT) ==
            AMCORE_STATUS_INVALID_ARGUMENT;
    success = success &&
        synthetic_operation(NULL, &accepted, AMCORE_STATUS_SUCCESS) ==
            AMCORE_STATUS_SUCCESS &&
        no_fields() == AMCORE_STATUS_SUCCESS &&
        many_fields() == AMCORE_STATUS_SUCCESS &&
        invalid_fields();
#ifdef AMCORE_ENABLE_TELEMETRY
    success = success && sent == 5 && telemetry_valid && allocation_failures();
#endif
#ifdef AMCORE_ENABLE_OPERATION_LOGGING
    success = success && check_database(argv[1]);
#else
    (void)argc;
    (void)argv;
#endif
    if (!success) fputs("generic operation field tests failed\n", stderr);
    return success ? 0 : 1;
}
