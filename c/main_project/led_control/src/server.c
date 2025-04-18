#include <libwebsockets.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

static const struct lws_protocols protocols[];

#define STATIC_PATH "./led_functions/www"  // Path for static files
#define WEBSOCKET_PORT 8080

// WebSocket protocol callback function
static int websocket_callback(struct lws *wsi, enum lws_callback_reasons reason,
                              void *user, void *in, size_t len)
{
    switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:
        printf("WebSocket connection established\n");
        break;
    case LWS_CALLBACK_RECEIVE:
        printf("Received message: %s\n", (char *)in);
        lws_write(wsi, in, len, LWS_WRITE_TEXT);  // Echo the received message back
        break;
    case LWS_CALLBACK_CLOSED:
        printf("WebSocket connection closed\n");
        break;
    default:
        break;
    }

    return 0;
}

// HTTP callback function for serving static files
static int http_callback(struct lws *wsi, enum lws_callback_reasons reason,
    void *user, void *in, size_t len)
{
    switch (reason) {
    case LWS_CALLBACK_HTTP:
    {
    // Get the requested URI
    const char *requested_uri = (const char *)in;

    // If the requested file is not specified, serve index.html
    if (strcmp(requested_uri, "/") == 0) {
    requested_uri = "/index.html";
    }

    // Build the full file path
    char file_path[1024];
    snprintf(file_path, sizeof(file_path), "%s%s", STATIC_PATH, requested_uri);

    // Check if the requested file exists
    struct stat file_stat;
    if (stat(file_path, &file_stat) == -1) {
    // If file doesn't exist, return 404
    lws_return_http_status(wsi, HTTP_STATUS_NOT_FOUND, NULL);
    return -1;
    }

    // Serve the requested file
    const char *content_type;
    if (strstr(requested_uri, ".html")) {
    content_type = "text/html";
    } else if (strstr(requested_uri, ".css")) {
    content_type = "text/css";
    } else if (strstr(requested_uri, ".js")) {
    content_type = "application/javascript";
    } else if (strstr(requested_uri, ".jpg") || strstr(requested_uri, ".jpeg")) {
    content_type = "image/jpeg";
    } else if (strstr(requested_uri, ".png")) {
    content_type = "image/png";
    } else if (strstr(requested_uri, ".gif")) {
    content_type = "image/gif";
    } else {
    content_type = "application/octet-stream";
    }

    lws_return_http_status(wsi, HTTP_STATUS_OK, "Content-Type: text/html\r\n\r\n");
    const char *response = "<html><body><h1>Hello, world!</h1></body></html>";
    lws_write(wsi, (unsigned char *)response, strlen(response), LWS_WRITE_HTTP);


    // Serve the file with NULL for mime_type and extra_headers
    // if (lws_serve_http_file(wsi, file_path, content_type, NULL, 0) < 0) {
    // return -1;
    // }
    break;
    }
    default:
    break;
    }

    return 0;
}

// Define the WebSocket protocol
static const struct lws_protocols protocols[] = {
    {
        "http-only",       // protocol name
        http_callback,  // callback function
        0,             // per session data size
        1024,          // maximum frame size
    },
    { NULL, NULL, 0, 0 }  // end of protocols list
};

// Create the server context for `libwebsockets`
static struct lws_context *create_server_context()
{
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = WEBSOCKET_PORT;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    // Create the server context
    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "Error creating server context\n");
        return NULL;
    }

    return context;
}

int main(void)
{
    struct lws_context *context = create_server_context();
    if (!context) {
        return -1;
    }

    // Main event loop to process connections
    while (1) {
        lws_service(context, 100);
    }

    lws_context_destroy(context);
    return 0;
}
