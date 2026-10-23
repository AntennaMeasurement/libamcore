#ifndef AMCORE_OPERATION_LOG_H
#define AMCORE_OPERATION_LOG_H

#include <amcore/utility.h>

#if defined(AMCORE_ENABLE_OPERATION_LOGGING) || defined(AMCORE_ENABLE_TELEMETRY)
#include <stddef.h>
#include <stdint.h>

typedef enum {
    AMCORE_OPERATION_DOUBLE,
    AMCORE_OPERATION_INT64,
    AMCORE_OPERATION_BOOL,
    AMCORE_OPERATION_TEXT
} amcore_operation_type;

/* data points to a double, int64_t, int (boolean), or NUL-terminated text. */
typedef struct {
    const char *name;
    amcore_operation_type type;
    const void *data;
} amcore_operation_field;

typedef struct {
    int64_t executed_at_us;
    int64_t ticks;
    int valid;
} amcore_operation_start;

amcore_operation_start amcore_operation_begin(void);
amcore_status amcore_operation_end(amcore_operation_start start,
                                  const char *function_name,
                                  const amcore_operation_field *inputs,
                                  size_t input_count,
                                  const amcore_operation_field *outputs,
                                  size_t output_count,
                                  amcore_status status);

#define AMCORE_OPERATION_FIELDS(name, ...) \
    const amcore_operation_field name[] = {__VA_ARGS__}
#define AMCORE_OPERATION_BEGIN(inputs, input_count, outputs, output_count) \
    const amcore_operation_field *operation_inputs = (inputs); \
    const size_t operation_input_count = (input_count); \
    const amcore_operation_field *operation_outputs = (outputs); \
    const size_t operation_output_count = (output_count); \
    amcore_operation_start operation_start = amcore_operation_begin()
#define AMCORE_OPERATION_RETURN(status) \
    return amcore_operation_end(operation_start, __func__, operation_inputs, \
        operation_input_count, operation_outputs, operation_output_count, status)
#else
#define AMCORE_OPERATION_FIELDS(name, ...) ((void)0)
#define AMCORE_OPERATION_BEGIN(inputs, input_count, outputs, output_count) ((void)0)
#define AMCORE_OPERATION_RETURN(status) return (status)
#endif

#endif
