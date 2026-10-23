#ifndef AMCORE_OPERATION_H
#define AMCORE_OPERATION_H

#include "operation_log.h"

typedef struct {
    const char *function_name;
    const amcore_operation_field *inputs;
    size_t input_count;
    const amcore_operation_field *outputs;
    size_t output_count;
    int output_is_null;
    amcore_status status;
    int64_t executed_at_us;
    int64_t duration_ns;
} amcore_operation_event;

#ifdef AMCORE_ENABLE_OPERATION_LOGGING
void amcore_operation_log_write(const amcore_operation_event *event);
#endif

#ifdef AMCORE_ENABLE_TELEMETRY
void amcore_telemetry_write(const amcore_operation_event *event);
#endif

#endif
