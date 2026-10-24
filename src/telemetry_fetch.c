#include "telemetry_backend.h"

#include <emscripten/fetch.h>
#include <emscripten/threading.h>

#include <stdio.h>
#include <string.h>

#ifndef AMCORE_TELEMETRY_ENDPOINT
#error "AMCORE_TELEMETRY_ENDPOINT must be defined by the build system"
#endif

void amcore_telemetry_send(const char *json, size_t length)
{
    emscripten_fetch_attr_t attributes;
    emscripten_fetch_t *fetch;
    const char *headers[] = {"Content-Type", "application/json", NULL};

    if (emscripten_is_main_browser_thread()) {
        fputs("amcore telemetry: synchronous Fetch requires a pthread worker thread\n",
              stderr);
        return;
    }
    if (length > AMCORE_TELEMETRY_MAX_BODY_SIZE) {
        fputs("amcore telemetry: HTTP request body exceeds the 1 MiB JSON body limit\n",
              stderr);
        return;
    }
    emscripten_fetch_attr_init(&attributes);
    strcpy(attributes.requestMethod, "POST");
    attributes.attributes =
        EMSCRIPTEN_FETCH_SYNCHRONOUS | EMSCRIPTEN_FETCH_REPLACE;
    attributes.requestHeaders = headers;
    attributes.requestData = json;
    attributes.requestDataSize = length;
    attributes.timeoutMSecs = 5000;
    fetch = emscripten_fetch(&attributes, AMCORE_TELEMETRY_ENDPOINT);
    if (fetch == NULL) {
        fputs("amcore telemetry: Fetch request could not be started\n", stderr);
        return;
    }
    if (fetch->status < 200 || fetch->status >= 300) {
        fprintf(stderr, "amcore telemetry: endpoint returned HTTP status %d\n",
                fetch->status);
    }
    emscripten_fetch_close(fetch);
}
