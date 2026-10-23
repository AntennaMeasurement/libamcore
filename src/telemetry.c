#include "operation.h"
#include "telemetry_backend.h"

#include <cjson/cJSON.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>

static void report_error(const char *message)
{
    fprintf(stderr, "amcore telemetry: %s\n", message);
}

static cJSON *integer_value(int64_t value)
{
    char number[32];
    int length = snprintf(number, sizeof(number), "%" PRId64, value);

    if (length < 0 || (size_t)length >= sizeof(number)) {
        return NULL;
    }
    /* cJSON numbers use double; a generated raw literal preserves all 64 bits. */
    return cJSON_CreateRaw(number);
}

static cJSON *field_value(const amcore_operation_field *field)
{
    double value;

    if (field->data == NULL) {
        return cJSON_CreateNull();
    }
    switch (field->type) {
    case AMCORE_OPERATION_DOUBLE:
        value = *(const double *)field->data;
        if (isfinite(value)) {
            return cJSON_CreateNumber(value);
        }
        return cJSON_CreateString(isnan(value) ? "NaN" :
                                  value > 0.0 ? "+Infinity" : "-Infinity");
    case AMCORE_OPERATION_INT64:
        return integer_value(*(const int64_t *)field->data);
    case AMCORE_OPERATION_BOOL:
        return cJSON_CreateBool(*(const int *)field->data != 0);
    case AMCORE_OPERATION_TEXT:
        return cJSON_CreateString(field->data);
    }
    return NULL;
}

static int add_value(cJSON *object, const char *name, cJSON *value)
{
    if (value == NULL) {
        return 0;
    }
    if (!cJSON_AddItemToObject(object, name, value)) {
        cJSON_Delete(value);
        return 0;
    }
    return 1;
}

static cJSON *field_values(const amcore_operation_field *fields, size_t count)
{
    cJSON *object = cJSON_CreateObject();
    size_t i;

    if (object == NULL) {
        return NULL;
    }
    for (i = 0; i < count; ++i) {
        if (!add_value(object, fields[i].name, field_value(&fields[i]))) {
            cJSON_Delete(object);
            return NULL;
        }
    }
    return object;
}

void amcore_telemetry_write(const amcore_operation_event *event)
{
    cJSON *object = cJSON_CreateObject();
    char *json;
    int has_output = event->status == AMCORE_STATUS_SUCCESS &&
                    !(event->output_count == 1 && event->output_is_null);

    if (object == NULL ||
        !add_value(object, "function", cJSON_CreateString(event->function_name)) ||
        !add_value(object, "inputs", field_values(event->inputs, event->input_count)) ||
        !add_value(object, "output_is_null", cJSON_CreateBool(event->output_is_null)) ||
        !add_value(object, "return_value", cJSON_CreateNumber(event->status)) ||
        !add_value(object, "output", has_output
            ? field_values(event->outputs, event->output_count) : cJSON_CreateNull()) ||
        !add_value(object, "executed_at_us", integer_value(event->executed_at_us)) ||
        !add_value(object, "duration_ns", integer_value(event->duration_ns))) {
        report_error("cannot allocate calculation event");
        cJSON_Delete(object);
        return;
    }
    json = cJSON_PrintUnformatted(object);
    cJSON_Delete(object);
    if (json == NULL) {
        report_error("cannot serialize calculation event");
        return;
    }
    amcore_telemetry_send(json);
    cJSON_free(json);
}
