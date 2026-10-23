#ifndef AMCORE_OPERATION_LOG_H
#define AMCORE_OPERATION_LOG_H

#include <amcore/utility.h>

#ifdef AMCORE_ENABLE_OPERATION_LOGGING
#include <stdint.h>

typedef struct {
    int64_t executed_at_us;
    int64_t ticks;
    int valid;
} amcore_operation_start;

amcore_operation_start amcore_operation_begin(void);
amcore_status amcore_operation_end(amcore_operation_start start,
                                  const char *function_name,
                                  double frequency_hz,
                                  double aperture_length_m,
                                  const double *output,
                                  amcore_status status);

#define AMCORE_OPERATION_BEGIN() \
    amcore_operation_start operation_start = amcore_operation_begin()
#define AMCORE_OPERATION_RETURN(name, frequency, aperture, output, status) \
    return amcore_operation_end(operation_start, name, frequency, aperture, \
                                output, status)
#else
#define AMCORE_OPERATION_BEGIN() ((void)0)
#define AMCORE_OPERATION_RETURN(name, frequency, aperture, output, status) \
    return (status)
#endif

#endif
