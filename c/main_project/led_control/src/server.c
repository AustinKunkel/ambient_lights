#include <libwebsockets.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include "../../cJSON/cJSON.h"
#include "led_functions.h"
#include "main.h"
#include "csv_control.h"

static const struct lws_protocols protocols[];

#define WEB_ROOT "./led_control/www"  // Path for static files
#define LED_SETTINGS_FILENAME "led_control/data/led_settings.csv"
#define LED_SETTINGS_HEADER "brightness, color, capture screen, sount react, fx num, count, id\n"
#define SC_SETTINGS_FILENAME "led_control/data/sc_settings.csv"
#define SC_SETTINGS_HEADER "V offset, H offset, avg color, left count, right count, top count, bottom count, res x, res y, blend depth, blend mode\n"
#define PORT 8080

struct lws_context *context;


int parse_led_settings_data_to_string(char *str) {
    return sprintf(str, "%d,#%06X,%d,%d,%d,%d,%d",
        led_settings.brightness,
        led_settings.color,
        led_settings.capture_screen,
        led_settings.sound_react,
        led_settings.fx_num,
        led_settings.count,
        led_settings.id
    );
}

int initialize_led_settings() {
    char data_line[512];
    printf("reading led_settings.csv...\n");
    if(read_one_line(LED_SETTINGS_FILENAME, data_line, sizeof(data_line)) == 0)
    {
        char *line_ptr = data_line;
        printf("Setting led settings variables...\n");
        led_settings.brightness = atoi(next_token(&line_ptr));
        printf("Brightness: %d\n", led_settings.brightness);
        char *hex = next_token(&line_ptr);
        //printf("Hex string: %s\n", hex);
        if(hex) {
            char *hex_ptr = (hex[0] == '#') ? hex + 1 : hex;
            //printf("Hex ptr: %s\n", hex_ptr);
            led_settings.color = (int)strtol(hex_ptr, NULL, 16);
        } else {
            led_settings.color = 0xDFC57B;
        }
        //printf("Color: #%06lX\n", led_settings.color);
        led_settings.capture_screen = atoi(next_token(&line_ptr));
        //printf("Capture screen: %d\n", led_settings.capture_screen);
        led_settings.sound_react = atoi(next_token(&line_ptr));
        //printf("sound react: %d\n", led_settings.sound_react);
        led_settings.fx_num = atoi(next_token(&line_ptr));
        //printf("fx num: %d\n", led_settings.fx_num);
        led_settings.count = atoi(next_token(&line_ptr));
        //printf("count: %d\n", led_settings.count);
        led_settings.id = atoi(next_token(&line_ptr));
        //printf("id: %d\n", led_settings.id);
        char buffer[256];
        parse_led_settings_data_to_string(buffer);
        printf("Current LED settings: %s\n", buffer);
        return 0;
    } else {
        perror("Unable to read from led_settings.csv!!\n");
        return 1;
    }
}

// WebSocket protocol callback function
static int websocket_callback(struct lws *wsi, enum lws_callback_reasons reason,
    void *user, void *in, size_t len)
{
    switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:
        printf("WebSocket connection established\n");
        lws_callback_on_writable(wsi);
        break;
    case LWS_CALLBACK_RECEIVE:
        printf("Received message: %s\n", (char *)in);
        lws_callback_on_writable(wsi);
        break;
    case LWS_CALLBACK_CLOSED:
        printf("WebSocket connection closed\n");
        break;
    case LWS_CALLBACK_SERVER_WRITEABLE: {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "status", "ok");
        cJSON_AddNumberToObject(root, "code", 200);

        cJSON *data = cJSON_CreateObject();
        cJSON_AddNumberToObject(data, "brightness", led_settings.brightness);
        
        char color_str[8]; // "#RRGGBB"
        snprintf(color_str, sizeof(color_str), "#%06X", led_settings.color);
        cJSON_AddStringToObject(data, "color", color_str);
        cJSON_AddNumberToObject(data, "capture_screen", led_settings.capture_screen);
        cJSON_AddNumberToObject(data, "sound_react", led_settings.sound_react);
        cJSON_AddNumberToObject(data, "fx_num", led_settings.fx_num);
        cJSON_AddNumberToObject(data, "count", led_settings.count);
        cJSON_AddNumberToObject(data, "id", led_settings.id);
        
        cJSON_AddItemToObject(root, "data", data);
        char *json_str = cJSON_PrintUnformatted(root); // You must free this
        
        // Prepare LWS buffer (with padding)
        unsigned char buffer[LWS_PRE + 1024];  // Adjust size as needed
        size_t json_len = strlen(json_str);
        if (json_len > 1024) json_len = 1024;  // Basic safety check
        
        memcpy(&buffer[LWS_PRE], json_str, json_len);
        
        // Send JSON over WebSocket
        lws_write(wsi, &buffer[LWS_PRE], json_len, LWS_WRITE_TEXT);
        
        // Cleanup
        free(json_str);
        cJSON_Delete(root);
        break;
        }
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
        case LWS_CALLBACK_HTTP: {
            printf("Received http request\n");
            // Get the requested URI
            const char *requested_uri = (const char *)in;

            if (strstr(requested_uri, "..")) {
                lws_return_http_status(wsi, HTTP_STATUS_FORBIDDEN, NULL);
                return -1;
            }

            // If the requested file is not specified, serve index.html
            if (strcmp(requested_uri, "/") == 0) {
            requested_uri = "/index.html";
            }


            // Build the full file path
            char file_path[512];
            snprintf(file_path, sizeof(file_path), "%s%s", WEB_ROOT, requested_uri);

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

            // lws_return_http_status(wsi, HTTP_STATUS_OK, "Content-Type: text/html\r\n\r\n");
            // const char *response = "<html><body><h1>Hello, world!</h1></body></html>";
            // lws_write(wsi, (unsigned char *)response, strlen(response), LWS_WRITE_HTTP);

            //Serve the file with NULL for mime_type and extra_headers

            printf("Serving file: %s\n", file_path);
            if (lws_serve_http_file(wsi, file_path, content_type, NULL, 0) < 0) {
                return -1;
            }
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
        "http",       // protocol name
        http_callback,  // callback function
        0,             // per session data size
        1024,          // maximum frame size
    }, {
        "websocket",
        websocket_callback,
        0,
        1024,
    },
    { NULL, NULL, 0, 0 }  // end of protocols list
};

// Create the server context for `libwebsockets`
static struct lws_context *create_server_context()
{
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = PORT;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;

    // Create the server context
    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "Error creating server context\n");
        return NULL;
    }

    return context;
}


void stop_server(int signo) {
    printf("\nStopping server...\n");
    lws_context_destroy(context);
    exit(0);
}

int main(void)
{
    signal(SIGINT, stop_server);
    signal(SIGTERM, stop_server);
    context = create_server_context();
    if (!context) {
        return -1;
    }

    if(initialize_led_settings()) {
        printf("Error initializing Led settings!\n");
        return 1;
    }

    printf("Server running on localhost:%d/\n", PORT);
    printf("Press CTRL+C to stop the server.\n");

    if(setup_strip(led_settings.count) != 0) {
        printf("Problem setting up strip...\n");
        return 1;
    }

    set_strip_32int_color(led_settings.color);
    set_brightness(led_settings.brightness);
    show_strip();

    // Main event loop to process connections
    while (1) {
        lws_service(context, 100);
    }

    lws_context_destroy(context);
    return 0;
}
