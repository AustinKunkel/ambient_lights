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
                printf("Received request: %s\n", lws_get_ssl(wsi));  // Log the full request path
                if (lws_serve_http_file(wsi, "./led_control/www/index.html", "text/html", NULL, 0))
                    return -1;
                break;
    
            case LWS_CALLBACK_HTTP_FILE_COMPLETION:
                // Optional: called after serving a file
                return -1;  // close connection after serving
    
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
        .name = "http",
        .callback = lws_callback_http_dummy(),  // built-in HTTP handler
        .per_session_data_size = 0,
        .rx_buffer_size = 0,
    },
    // {
    //     .name = "my-websocket-protocol",
    //     .callback = callback_ws,
    //     .per_session_data_size = 0,          // adjust if needed
    //     .rx_buffer_size = 4096,
    // },
    { NULL, NULL, 0, 0 }
};

int main() {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    char resolved_path[PATH_MAX];  // Buffer to store the resolved path
    if (realpath("./led_control/www", resolved_path) != NULL) {
        printf("Serving from path: %s\n", resolved_path);
    } else {
        perror("Error resolving path");
    }

    static const struct lws_http_mount mount = {
        .mount_next = NULL,           // Linked-list of mounts
        .mountpoint = "/",            // URL mount point
        .origin = "./led_control/www", // Local path
        .def = "index.html",          // Default file
        .protocol = NULL,             // Protocol for serving the files
        .cgienv = NULL,
        .extra_mimetypes = NULL,
        .interpret = NULL,
        .cgi_timeout = 0,
        .cache_max_age = 0,
        .auth_mask = 0,
        .cache_reusable = 0,
        .cache_revalidate = 0,
        .cache_intermediaries = 0,
        .origin_protocol = LWSMPRO_FILE, // Serve from filesystem
        .basic_auth_login_file = NULL,
    };

    lws_set_log_level(LLL_NOTICE | LLL_INFO | LLL_WARN | LLL_ERR, NULL);

    info.port = 8080;
    info.mounts = &mount;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT; // or other options
    struct lws_context *context = lws_create_context(&info);
    if (context == NULL) {
        fprintf(stderr, "libwebsockets init failed\n");
        return -1;
    }
    
    while (1) {
        lws_service(context, 100);
    }
    
    lws_context_destroy(context);
    return 0;
}