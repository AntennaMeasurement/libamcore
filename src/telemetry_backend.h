#ifndef AMCORE_TELEMETRY_BACKEND_H
#define AMCORE_TELEMETRY_BACKEND_H

#include <stddef.h>

#ifndef AMCORE_TELEMETRY_MAX_BODY_SIZE
#define AMCORE_TELEMETRY_MAX_BODY_SIZE ((size_t)1048576U)
#endif

void amcore_telemetry_send(const char *json, size_t length);

#endif
