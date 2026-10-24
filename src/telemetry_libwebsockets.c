#include "telemetry_backend.h"

#include <libwebsockets.h>

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define AMCORE_TELEMETRY_TIMEOUT_US 5000000

typedef struct {
    struct lws_context *context;
    lws_sorted_usec_list_t timeout;
    const char *json;
    size_t length;
    int done;
    int sent;
    int status;
    char error[256];
} telemetry_request;

static void report_error(const char *message)
{
    fprintf(stderr, "amcore telemetry: %s\n", message);
}

static void emit_log(int level, const char *line)
{
    (void)level;
    fprintf(stderr, "amcore telemetry: libwebsockets: %s", line);
}

static void request_timeout(lws_sorted_usec_list_t *timer)
{
    telemetry_request *request =
        lws_container_of(timer, telemetry_request, timeout);

    snprintf(request->error, sizeof(request->error), "request timed out");
    request->done = 1;
    lws_cancel_service(request->context);
}

static int callback_http(struct lws *connection,
                          enum lws_callback_reasons reason,
                          void *user, void *input, size_t length)
{
    telemetry_request *request;

    (void)user;
    (void)length;
    switch (reason) {
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
    case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
    case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
    case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
    case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
        request = lws_context_user(lws_get_context(connection));
        break;
    default:
        return 0;
    }

    switch (reason) {
    case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER: {
        unsigned char **cursor = input;
        unsigned char *end = *cursor + length;
        char content_length[32];

        snprintf(content_length, sizeof(content_length), "%lu",
                 (unsigned long)request->length);
        if (lws_add_http_header_by_token(connection, WSI_TOKEN_HTTP_CONTENT_TYPE,
                (const unsigned char *)"application/json", 16, cursor, end) ||
            lws_add_http_header_by_token(connection, WSI_TOKEN_HTTP_CONTENT_LENGTH,
                (const unsigned char *)content_length,
                (int)strlen(content_length), cursor, end)) {
            snprintf(request->error, sizeof(request->error),
                     "cannot construct HTTP headers");
            request->done = 1;
            return -1;
        }
        lws_client_http_body_pending(connection, 1);
        lws_callback_on_writable(connection);
        return 0;
    }
    case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
        if (request->sent) {
            return 0;
        }
        {
            unsigned char *buffer = malloc(LWS_PRE + request->length);
            int written;
            if (buffer == NULL) {
                snprintf(request->error, sizeof(request->error),
                         "cannot allocate HTTP request body");
                request->done = 1;
                return -1;
            }
            memcpy(buffer + LWS_PRE, request->json, request->length);
            written = lws_write(connection, buffer + LWS_PRE, request->length,
                                LWS_WRITE_HTTP_FINAL);
            free(buffer);
            if (written != (int)request->length) {
                snprintf(request->error, sizeof(request->error),
                         "cannot write HTTP request body");
                request->done = 1;
                return -1;
            }
        }
        request->sent = 1;
        lws_client_http_body_pending(connection, 0);
        return 0;
    case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
        request->status = (int)lws_http_client_http_response(connection);
        request->done = 1;
        lws_cancel_service(request->context);
        return 0;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        if (!request->done) {
            snprintf(request->error, sizeof(request->error), "%s",
                     input != NULL ? (const char *)input : "connection failed");
            request->done = 1;
        }
        return 0;
    case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
        if (!request->done) {
            snprintf(request->error, sizeof(request->error),
                     "connection closed before response completed");
            request->done = 1;
        }
        return 0;
    default:
        return 0;
    }
}

static const struct lws_protocols protocols[] = {
    {"amcore-telemetry", callback_http, 0, 0, 0, NULL, 0},
    {NULL, NULL, 0, 0, 0, NULL, 0}
};

void amcore_telemetry_send(const char *json, size_t length)
{
    telemetry_request request;
    struct lws_context_creation_info context_info;
    struct lws_client_connect_info connect_info;
    struct lws_log_cx log_context;
    char url[] = AMCORE_TELEMETRY_ENDPOINT;
    char path[sizeof(url) + 1];
    const char *scheme;
    const char *address;
    const char *uri;
    int port;

    memset(&request, 0, sizeof(request));
    request.json = json;
    request.length = length;
    if (request.length > AMCORE_TELEMETRY_MAX_BODY_SIZE ||
        request.length > INT_MAX || request.length > SIZE_MAX - LWS_PRE) {
        report_error("HTTP request body exceeds transport limits");
        return;
    }
    if (lws_parse_uri(url, &scheme, &address, &port, &uri) != 0 ||
        port <= 0 || port > 65535) {
        report_error("invalid configured endpoint");
        return;
    }
    snprintf(path, sizeof(path), "/%s", uri);
    memset(&log_context, 0, sizeof(log_context));
    log_context.u.emit = emit_log;
    log_context.lll_flags = LLL_ERR | LLL_WARN;
    memset(&context_info, 0, sizeof(context_info));
    context_info.port = CONTEXT_PORT_NO_LISTEN;
    context_info.protocols = protocols;
    context_info.user = &request;
    context_info.log_cx = &log_context;
    context_info.fd_limit_per_thread = 4;
    context_info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
#ifdef AMCORE_TELEMETRY_CA_FILE
    context_info.client_ssl_ca_filepath = AMCORE_TELEMETRY_CA_FILE;
#endif
    request.context = lws_create_context(&context_info);
    if (request.context == NULL) {
        report_error("cannot create HTTP client context");
        return;
    }
    memset(&connect_info, 0, sizeof(connect_info));
    connect_info.context = request.context;
    connect_info.address = address;
    connect_info.port = port;
    connect_info.path = path;
    connect_info.host = address;
    connect_info.method = "POST";
    connect_info.protocol = protocols[0].name;
    connect_info.alpn = "http/1.1";
    connect_info.ssl_connection = LCCSCF_HTTP_NO_FOLLOW_REDIRECT;
    if (strcmp(scheme, "https") == 0) {
#ifndef LWS_WITH_TLS
        report_error("libwebsockets was built without TLS support");
        lws_context_destroy(request.context);
        return;
#else
        connect_info.ssl_connection |= LCCSCF_USE_SSL;
#endif
    }
    lws_sul_schedule(request.context, 0, &request.timeout, request_timeout,
                     AMCORE_TELEMETRY_TIMEOUT_US);
    if (lws_client_connect_via_info(&connect_info) == NULL && !request.done) {
        snprintf(request.error, sizeof(request.error),
                 "cannot start HTTP connection");
        request.done = 1;
    }
    while (!request.done) {
        if (lws_service(request.context, 0) < 0) {
            snprintf(request.error, sizeof(request.error),
                     "HTTP service failed");
            break;
        }
    }
    lws_sul_cancel(&request.timeout);
    lws_context_destroy(request.context);
    if (request.error[0] != '\0') {
        report_error(request.error);
    } else if (request.status < 200 || request.status >= 300) {
        fprintf(stderr, "amcore telemetry: endpoint returned HTTP status %d\n",
                request.status);
    }
}
