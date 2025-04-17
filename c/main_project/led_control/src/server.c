#include <libwebsockets.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "../../cJSON/cJSON.h"
#include "led_functions.h"
#include "main.h"
#include "csv_control.h"

static int callback_http(struct lws *wsi, enum lws_callback_reasons reason,
    void *user, void *in, size_t len) {
switch (reason) {
    case LWS_CALLBACK_HTTP:
        if (lws_serve_http_file(wsi, "./led_control/www/index.html", "text/html", NULL, 0))
            return -1;
        break;
    default:
        break;
    }
    return 0;
}

static int callback_ws(struct lws *wsi, enum lws_callback_reasons reason,
 void *user, void *in, size_t len) {
switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:
        // Handle new WebSocket connection
        break;
    case LWS_CALLBACK_RECEIVE:
        // Handle received WebSocket message
        break;
    case LWS_CALLBACK_CLOSED:
        // Handle WebSocket closure
        break;
    default:
        break;
}
return 0;
}


static struct lws_protocols protocols[] = {
    {
        .name = "http-only",
        .callback = lws_callback_http_dummy,  // built-in HTTP handler
        .per_session_data_size = 0,
        .rx_buffer_size = 0,
    },
    {
        .name = "my-websocket-protocol",
        .callback = callback_ws,
        .per_session_data_size = 0,          // adjust if needed
        .rx_buffer_size = 4096,
    },
    { NULL, NULL, 0, 0 }
};

int main() {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    info.port = 8080;
    info.protocols = protocols;
    struct lws_context *context = lws_create_context(&info);
    if (context == NULL) {
        fprintf(stderr, "libwebsockets init failed\n");
        return -1;
    }
    
    while (1) {
        lws_service(context, 0);
    }
    
    lws_context_destroy(context);
    return 0;
}