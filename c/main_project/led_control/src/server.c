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

static int callback_http(struct lws *wsi,
                         enum lws_callback_reasons reason,
                         void *user, void *in, size_t len) {
    return lws_callback_http_dummy(wsi, reason, user, in, len);
} 

static int callback_ws(struct lws *wsi,
                       enum lws_callback_reasons reason,
                       void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            lwsl_user("Websocket Connected\n");
            break;
        case LWS_CALLBACK_RECEIVE:
            lws_write(wsi, (unsigned char *)in, len, LWS_WRITE_TEXT);
            break;
        default:
            break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    {
        .name = "http",
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
    LWS_PROTOCOL_LIST_TERM
};

int main() {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    

    info.port = 8080;
    info.protocols = protocols;
    info.mounts = &(struct lws_http_mount) {
        .mount_next = NULL,
        .mountpoint = "/",
        .origin = "/home/controller/ambient_lights/c/main_project/led_control/www",  // your static files folder
        .def = "index.html",
        .origin_protocol = LWSMPRO_FILE,
        .protocol = NULL,
        .cgienv = NULL,
        .extra_mimetypes = NULL,
        .interpret = NULL,
        .cgi_timeout = 0,
        .cache_max_age = 0,
        .auth_mask = 0,
        .cache_reusable = 0,
        .cache_revalidate = 0,
        .cache_intermediaries = 0,
        .basic_auth_login_file = NULL
    };
    info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        lwsl_err("lws init failed\n");
        return -1;
    }

    while (1)
        lws_service(context, 1000);

    lws_context_destroy(context);
    return 0;
}