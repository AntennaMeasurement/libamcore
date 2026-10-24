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
#include "telemetry_backend.h"
#include <core_json.h>
#endif

static char label[5000];
static int sent;
static int telemetry_valid = 1;

#ifdef AMCORE_ENABLE_TELEMETRY
static int testing_integer_boundaries;
static int integer_boundaries_valid = 1;
static int expected_trim_mode;
static int trim_valid = 1;

void amcore_telemetry_send(const char *json, size_t length)
{
    static const char inputs_query[] = "inputs";
    static const char label_query[] = "inputs.label";
    static const char expected_label_prefix[] = "\\\"\\\\\\n";
    const char *value;
    size_t value_length;
    JSONTypes_t value_type;

    if (testing_integer_boundaries) {
        integer_boundaries_valid = length <= AMCORE_TELEMETRY_MAX_BODY_SIZE &&
            JSON_Validate(json, length) == JSONSuccess &&
            strstr(json, "\"executed_at_us\":9223372036854775807") != NULL &&
            strstr(json, "\"duration_ns\":9223372036854775807") != NULL &&
            strstr(json, "\"minimum\":-9223372036854775808") != NULL;
        return;
    }
    if (expected_trim_mode != 0) {
        trim_valid = trim_valid && length <= AMCORE_TELEMETRY_MAX_BODY_SIZE &&
            JSON_Validate(json, length) == JSONSuccess;
        if (expected_trim_mode == 1) {
            trim_valid = trim_valid &&
                strstr(json, "\"inputs\":{\"small\":1}") != NULL &&
                strstr(json, "\"output_is_null\":false") != NULL &&
                strstr(json, "\"output\":null") != NULL;
        } else {
            trim_valid = trim_valid && strstr(json, "\"inputs\":{}") != NULL &&
                strstr(json, "\"output\":null") != NULL;
        }
        ++sent;
        return;
    }
    telemetry_valid = telemetry_valid &&
        length <= AMCORE_TELEMETRY_MAX_BODY_SIZE &&
        JSON_Validate(json, length) == JSONSuccess &&
        JSON_SearchConst(json, length, inputs_query, sizeof(inputs_query) - 1,
                         &value, &value_length, &value_type) == JSONSuccess &&
        value_type == JSONObject;
    ++sent;
    if (sent <= 3) {
        const char *text = NULL;
        size_t text_length = 0;
        JSONTypes_t text_type = JSONInvalid;
        int label_found = JSON_SearchConst(json, length, label_query,
            sizeof(label_query) - 1, &text, &text_length, &text_type) == JSONSuccess;

        telemetry_valid = telemetry_valid &&
            strlen(json) > 5000 &&
            strstr(json, "\"version\":\"" AMCORE_VERSION "\"") != NULL &&
            strstr(json, "\"function\":\"synthetic_operation\"") != NULL &&
            strstr(json, "\"count\":9223372036854775807") != NULL &&
            strstr(json, "\"enabled\":true") != NULL &&
            strstr(json, "\"optional\":null") != NULL &&
                        label_found && text_type == JSONString &&
                        text_length > sizeof(expected_label_prefix) - 1 &&
                        memcmp(text, expected_label_prefix,
                                     sizeof(expected_label_prefix) - 1) == 0 &&
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
}

static int integer_boundaries(void)
{
    int64_t minimum = INT64_MIN;
    int64_t maximum = INT64_MAX;
    const amcore_operation_field inputs[] = {
        {"minimum", AMCORE_OPERATION_INT64, &minimum},
        {"maximum", AMCORE_OPERATION_INT64, &maximum}
    };
    const amcore_operation_event event = {
        "integer_boundaries", inputs, 2, NULL, 0, 0,
        AMCORE_STATUS_SUCCESS, INT64_MAX, INT64_MAX
    };

    testing_integer_boundaries = 1;
    amcore_telemetry_write(&event);
    testing_integer_boundaries = 0;
    return integer_boundaries_valid;
}

static int oversized_payloads_trimmed(void)
{
    static char text[AMCORE_TELEMETRY_MAX_BODY_SIZE];
    int64_t small_input = 1;
    const amcore_operation_field small_inputs[] = {
        {"small", AMCORE_OPERATION_INT64, &small_input}
    };
    const amcore_operation_field large_inputs[] = {
        {"large", AMCORE_OPERATION_TEXT, text}
    };
    const amcore_operation_field outputs[] = {
        {"large", AMCORE_OPERATION_TEXT, text}
    };
    const amcore_operation_event output_oversized = {
        "oversized_output", small_inputs, 1, outputs, 1, 0,
        AMCORE_STATUS_SUCCESS, 0, 0
    };
    const amcore_operation_event both_oversized = {
        "oversized_input_and_output", large_inputs, 1, outputs, 1, 0,
        AMCORE_STATUS_SUCCESS, 0, 0
    };

    memset(text, 'x', sizeof(text) - 1);
    text[sizeof(text) - 1] = '\0';
    expected_trim_mode = 1;
    amcore_telemetry_write(&output_oversized);
    expected_trim_mode = 2;
    amcore_telemetry_write(&both_oversized);
    expected_trim_mode = 0;
    return trim_valid && sent == 7;
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
    success = success && sent == 5 && telemetry_valid &&
              integer_boundaries() && oversized_payloads_trimmed();
#endif
#ifdef AMCORE_ENABLE_OPERATION_LOGGING
    success = success && check_database(argv[1]);
#else
    (void)argc;
    (void)argv;
#endif
#ifdef AMCORE_ENABLE_TELEMETRY
    if (!success) {
        fprintf(stderr, "telemetry checks: sent=%d valid=%d integer_boundaries=%d\n",
                sent, telemetry_valid, integer_boundaries_valid);
    }
#endif
    if (!success) fputs("generic operation field tests failed\n", stderr);
    return success ? 0 : 1;
}
