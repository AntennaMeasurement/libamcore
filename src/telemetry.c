#include "operation.h"
#include "telemetry_backend.h"

#include <core_json.h>
#include <inttypes.h>
#include <stdarg.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
    int too_large;
    int allocation_failed;
} json_writer;

static void report_error(const char *message)
{
    fprintf(stderr, "amcore telemetry: %s\n", message);
}

static int writer_reserve(json_writer *writer, size_t additional)
{
    size_t needed;
    size_t capacity;
    char *data;

    if (additional > AMCORE_TELEMETRY_MAX_BODY_SIZE - writer->length) {
        writer->too_large = 1;
        return 0;
    }
    needed = writer->length + additional + 1;
    if (needed <= writer->capacity) {
        return 1;
    }
    capacity = writer->capacity == 0 ? 256 : writer->capacity;
    while (capacity < needed) {
        if (capacity > (AMCORE_TELEMETRY_MAX_BODY_SIZE + 1) / 2) {
            capacity = AMCORE_TELEMETRY_MAX_BODY_SIZE + 1;
            break;
        }
        capacity *= 2;
    }
    data = realloc(writer->data, capacity);
    if (data == NULL) {
        writer->allocation_failed = 1;
        return 0;
    }
    writer->data = data;
    writer->capacity = capacity;
    return 1;
}

static int writer_append(json_writer *writer, const char *text, size_t length)
{
    if (!writer_reserve(writer, length)) {
        return 0;
    }
    memcpy(writer->data + writer->length, text, length);
    writer->length += length;
    writer->data[writer->length] = '\0';
    return 1;
}

static int writer_char(json_writer *writer, char value)
{
    return writer_append(writer, &value, 1);
}

static int writer_string(json_writer *writer, const char *text)
{
    static const char hex[] = "0123456789abcdef";
    const unsigned char *character = (const unsigned char *)text;

    if (!writer_char(writer, '"')) {
        return 0;
    }
    while (*character != '\0') {
        char escape[6];
        const char *escaped = NULL;
        size_t escape_length = 0;

        switch (*character) {
        case '"': escaped = "\\\""; escape_length = 2; break;
        case '\\': escaped = "\\\\"; escape_length = 2; break;
        case '\b': escaped = "\\b"; escape_length = 2; break;
        case '\f': escaped = "\\f"; escape_length = 2; break;
        case '\n': escaped = "\\n"; escape_length = 2; break;
        case '\r': escaped = "\\r"; escape_length = 2; break;
        case '\t': escaped = "\\t"; escape_length = 2; break;
        default:
            if (*character < 0x20) {
                escape[0] = '\\';
                escape[1] = 'u';
                escape[2] = '0';
                escape[3] = '0';
                escape[4] = hex[*character >> 4];
                escape[5] = hex[*character & 0x0f];
                escaped = escape;
                escape_length = sizeof(escape);
            }
            break;
        }
        if (escaped != NULL) {
            if (!writer_append(writer, escaped, escape_length)) {
                return 0;
            }
        } else if (!writer_append(writer, (const char *)character, 1)) {
            return 0;
        }
        ++character;
    }
    return writer_char(writer, '"');
}

static int writer_format(json_writer *writer, const char *format, ...)
{
    char number[64];
    va_list arguments;
    int length;

    va_start(arguments, format);
    length = vsnprintf(number, sizeof(number), format, arguments);
    va_end(arguments);
    if (length < 0 || (size_t)length >= sizeof(number)) {
        return 0;
    }
    return writer_append(writer, number, (size_t)length);
}

static int writer_double(json_writer *writer, double value)
{
    char number[64];
    const char *decimal_point;
    char *separator;
    int length = snprintf(number, sizeof(number), "%.17g", value);

    if (length < 0 || (size_t)length >= sizeof(number)) {
        return 0;
    }
    decimal_point = localeconv()->decimal_point;
    if (decimal_point != NULL && decimal_point[0] != '\0' &&
        strcmp(decimal_point, ".") != 0 &&
        (separator = strstr(number, decimal_point)) != NULL) {
        size_t decimal_length = strlen(decimal_point);
        size_t suffix_length = strlen(separator + decimal_length);

        memmove(separator + 1, separator + decimal_length, suffix_length + 1);
        *separator = '.';
        length -= (int)(decimal_length - 1);
    }
    return writer_append(writer, number, (size_t)length);
}

static int write_field_value(json_writer *writer,
                             const amcore_operation_field *field)
{
    double value;

    if (field->data == NULL) {
        return writer_append(writer, "null", 4);
    }
    switch (field->type) {
    case AMCORE_OPERATION_DOUBLE:
        value = *(const double *)field->data;
        if (isfinite(value)) {
            return writer_double(writer, value);
        }
        return writer_string(writer, isnan(value) ? "NaN" :
                             value > 0.0 ? "+Infinity" : "-Infinity");
    case AMCORE_OPERATION_INT64:
        return writer_format(writer, "%" PRId64,
                             *(const int64_t *)field->data);
    case AMCORE_OPERATION_BOOL:
        return writer_append(writer,
            *(const int *)field->data != 0 ? "true" : "false",
            *(const int *)field->data != 0 ? 4 : 5);
    case AMCORE_OPERATION_TEXT:
        return writer_string(writer, field->data);
    }
    return 0;
}

static int write_fields(json_writer *writer,
                        const amcore_operation_field *fields, size_t count)
{
    size_t i;

    if (count != 0 && fields == NULL) {
        return 0;
    }
    if (!writer_char(writer, '{')) {
        return 0;
    }
    for (i = 0; i < count; ++i) {
        if ((i != 0 && !writer_char(writer, ',')) ||
            !writer_string(writer, fields[i].name) ||
            !writer_char(writer, ':') ||
            !write_field_value(writer, &fields[i])) {
            return 0;
        }
    }
    return writer_char(writer, '}');
}

void amcore_telemetry_write(const amcore_operation_event *event)
{
    json_writer writer = {0};
    int has_output = event->status == AMCORE_STATUS_SUCCESS &&
                    !(event->output_count == 1 && event->output_is_null);

    if (!writer_char(&writer, '{') ||
        !writer_string(&writer, "version") || !writer_char(&writer, ':') ||
        !writer_string(&writer, AMCORE_VERSION) || !writer_char(&writer, ',') ||
        !writer_string(&writer, "function") || !writer_char(&writer, ':') ||
        !writer_string(&writer, event->function_name) || !writer_char(&writer, ',') ||
        !writer_string(&writer, "inputs") || !writer_char(&writer, ':') ||
        !write_fields(&writer, event->inputs, event->input_count) ||
        !writer_char(&writer, ',') ||
        !writer_string(&writer, "output_is_null") || !writer_char(&writer, ':') ||
        !writer_append(&writer, event->output_is_null ? "true" : "false",
                       event->output_is_null ? 4 : 5) ||
        !writer_char(&writer, ',') ||
        !writer_string(&writer, "return_value") || !writer_char(&writer, ':') ||
        !writer_format(&writer, "%d", (int)event->status) ||
        !writer_char(&writer, ',') ||
        !writer_string(&writer, "output") || !writer_char(&writer, ':') ||
        (has_output
            ? !write_fields(&writer, event->outputs, event->output_count)
            : !writer_append(&writer, "null", 4)) ||
        !writer_char(&writer, ',') ||
        !writer_string(&writer, "executed_at_us") || !writer_char(&writer, ':') ||
        !writer_format(&writer, "%" PRId64, event->executed_at_us) ||
        !writer_char(&writer, ',') ||
        !writer_string(&writer, "duration_ns") || !writer_char(&writer, ':') ||
        !writer_format(&writer, "%" PRId64, event->duration_ns) ||
        !writer_char(&writer, '}')) {
        if (writer.too_large) {
            report_error("calculation event exceeds the 1 MiB JSON body limit");
        } else {
            report_error("cannot allocate calculation event");
        }
        free(writer.data);
        return;
    }
    if (JSON_Validate(writer.data, writer.length) != JSONSuccess) {
        report_error("cannot serialize calculation event");
        free(writer.data);
        return;
    }
    amcore_telemetry_send(writer.data, writer.length);
    free(writer.data);
}
