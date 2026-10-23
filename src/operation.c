#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "operation.h"

#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

static void report_error(const char *message)
{
#ifdef AMCORE_ENABLE_OPERATION_LOGGING
    fprintf(stderr, "amcore operation logging: %s\n", message);
#endif
#ifdef AMCORE_ENABLE_TELEMETRY
    fprintf(stderr, "amcore telemetry: %s\n", message);
#endif
}

amcore_operation_start amcore_operation_begin(void)
{
    amcore_operation_start start = {0, 0, 0};
#ifdef _WIN32
    FILETIME wall;
    ULARGE_INTEGER value;
    LARGE_INTEGER ticks;

    GetSystemTimeAsFileTime(&wall);
    value.LowPart = wall.dwLowDateTime;
    value.HighPart = wall.dwHighDateTime;
    start.executed_at_us = (int64_t)(value.QuadPart / 10) - 11644473600000000LL;
    if (!QueryPerformanceCounter(&ticks)) {
        report_error("cannot read monotonic clock");
        return start;
    }
    start.ticks = ticks.QuadPart;
#else
    struct timespec wall;
    struct timespec monotonic;

    if (clock_gettime(CLOCK_REALTIME, &wall) != 0 ||
        clock_gettime(CLOCK_MONOTONIC, &monotonic) != 0) {
        report_error("cannot read operation clocks");
        return start;
    }
    start.executed_at_us = (int64_t)wall.tv_sec * 1000000 + wall.tv_nsec / 1000;
    start.ticks = (int64_t)monotonic.tv_sec * 1000000000 + monotonic.tv_nsec;
#endif
    start.valid = 1;
    return start;
}

static int elapsed_ns(amcore_operation_start start, int64_t *duration)
{
#ifdef _WIN32
    LARGE_INTEGER ticks;
    LARGE_INTEGER frequency;
    int64_t delta;
    if (!QueryPerformanceCounter(&ticks) ||
        !QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0) {
        report_error("cannot read monotonic clock");
        return 0;
    }
    delta = ticks.QuadPart - start.ticks;
    *duration = (int64_t)((long double)delta * 1000000000 /
                         frequency.QuadPart);
#else
    struct timespec ticks;
    if (clock_gettime(CLOCK_MONOTONIC, &ticks) != 0) {
        report_error("cannot read monotonic clock");
        return 0;
    }
    *duration = (int64_t)ticks.tv_sec * 1000000000 + ticks.tv_nsec - start.ticks;
#endif
    if (*duration < 0) {
        report_error("monotonic clock moved backwards");
        return 0;
    }
    return 1;
}

static int valid_identifier(const char *name)
{
    const unsigned char *p = (const unsigned char *)name;
    if (p == NULL || !((*p >= 'a' && *p <= 'z') ||
                      (*p >= 'A' && *p <= 'Z') || *p == '_')) {
        return 0;
    }
    for (++p; *p != '\0'; ++p) {
        if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
              (*p >= '0' && *p <= '9') || *p == '_')) {
            return 0;
        }
    }
    return 1;
}

static int same_identifier(const char *left, const char *right)
{
    while (*left != '\0' && *right != '\0') {
        char a = *left++;
        char b = *right++;
        if (a >= 'A' && a <= 'Z') {
            a = (char)(a + 'a' - 'A');
        }
        if (b >= 'A' && b <= 'Z') {
            b = (char)(b + 'a' - 'A');
        }
        if (a != b) {
            return 0;
        }
    }
    return *left == *right;
}

static int valid_fields(const amcore_operation_field *fields, size_t count,
                        const amcore_operation_field *previous,
                        size_t previous_count)
{
    static const char *reserved[] = {
        "id", "version", "output_is_null", "return_value",
        "executed_at_us", "duration_ns"
    };
    size_t i, j;

    if (count != 0 && fields == NULL) {
        return 0;
    }
    for (i = 0; i < count; ++i) {
        if (!valid_identifier(fields[i].name) ||
            fields[i].type < AMCORE_OPERATION_DOUBLE ||
            fields[i].type > AMCORE_OPERATION_TEXT) {
            return 0;
        }
        for (j = 0; j < sizeof(reserved) / sizeof(reserved[0]); ++j) {
            if (same_identifier(fields[i].name, reserved[j])) {
                return 0;
            }
        }
        for (j = 0; j < i; ++j) {
            if (same_identifier(fields[i].name, fields[j].name)) {
                return 0;
            }
        }
        for (j = 0; j < previous_count; ++j) {
            if (same_identifier(fields[i].name, previous[j].name)) {
                return 0;
            }
        }
    }
    return 1;
}

amcore_status amcore_operation_end(amcore_operation_start start,
                                  const char *function_name,
                                  const amcore_operation_field *inputs,
                                  size_t input_count,
                                  const amcore_operation_field *outputs,
                                  size_t output_count,
                                  amcore_status status)
{
    amcore_operation_event event;
    size_t i;

    if (!start.valid || !elapsed_ns(start, &event.duration_ns)) {
        return status;
    }
    if (!valid_identifier(function_name) ||
        !valid_fields(inputs, input_count, NULL, 0) ||
        !valid_fields(outputs, output_count, inputs, input_count)) {
        report_error("invalid operation field descriptors");
        return status;
    }
    event.function_name = function_name;
    event.inputs = inputs;
    event.input_count = input_count;
    event.outputs = outputs;
    event.output_count = output_count;
    event.output_is_null = 0;
    for (i = 0; i < output_count; ++i) {
        if (outputs[i].data == NULL) {
            event.output_is_null = 1;
        }
    }
    event.status = status;
    event.executed_at_us = start.executed_at_us;
#ifdef AMCORE_ENABLE_OPERATION_LOGGING
    amcore_operation_log_write(&event);
#endif
#ifdef AMCORE_ENABLE_TELEMETRY
    amcore_telemetry_write(&event);
#endif
    return status;
}
