#include <microhttpd.h>
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

#define WEB_ROOT "./led_control/www"  // Directory containing HTML, CSS, JS files
#define LED_SETTINGS_FILENAME "led_control/data/led_settings.csv"
#define LED_SETTINGS_HEADER "brightness, color, capture screen, sount react, fx num, count, id\n"
#define PORT 8080

static struct MHD_Daemon *server;  // Declare server globally

int handle_get_request(struct MHD_Connection*, const char*);
int handle_post_request(struct MHD_Connection*, const char*, const char*, size_t*);
int handle_delete_request(struct MHD_Connection*, const char*);

// Function to determine the Content-Type based on file extension
const char *get_content_type(const char *path) {
    if (strstr(path, ".html")) return "text/html";
    if (strstr(path, ".css")) return "text/css";
    if (strstr(path, ".js")) return "application/javascript";
    if (strstr(path, ".png")) return "image/png";
    if (strstr(path, ".jpg")) return "image/jpeg";
    if (strstr(path, ".gif")) return "image/gif";
    if (strstr(path, ".ico")) return "image/x-icon";
    return "text/plain";
}

// Function to check if a file exists
int file_exists(const char *filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

// Function to serve files from the "www" directory
enum MHD_Result request_handler(void *cls, struct MHD_Connection *connection,
    const char *url, const char *method,
    const char *version, const char *upload_data,
    long unsigned int *upload_data_size, void **con_cls) {


  if (strcmp(method, "GET") == 0) {
  return handle_get_request(connection, url);
  } 
  else if (strcmp(method, "POST") == 0) {
  return handle_post_request(connection, url, upload_data, upload_data_size);
  } 
  else if (strcmp(method, "DELETE") == 0) {
  return handle_delete_request(connection, url);
  }

  return MHD_NO;
}

void stop_server(int signo) {
    printf("\nStopping server...\n");
    if (server) {
        MHD_stop_daemon(server);
    }
    exit(0);
}

/**
 * Helper function to get the next token in a csv line
 */
char* next_token(char **line) {
    if (*line == NULL || **line == '\0') return NULL; // No more tokens or empty string

    // Skip any leading spaces or tabs
    while (**line == ' ' || **line == '\t') {
        (*line)++;
    }

    // If we reached the end of the string, return NULL
    if (**line == '\0') return NULL;

    // Find the next delimiter (comma or end of line)
    char *token_start = *line;
    while (**line && **line != ',' && **line != '\n') {
        (*line)++;
    }
    // If we reached a comma, replace it with null terminator
    if (**line == ',' || **line == '\n') {
        **line = '\0';
        (*line)++; // Move past the delimiter
    }
    return token_start;
}
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

int main(int argc, char **argv) {
    // Register signal handler for CTRL+C
    signal(SIGINT, stop_server);
    signal(SIGTERM, stop_server);

    // Start the server
    server = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD,  // Use polling mode
        PORT, NULL, NULL,
        &request_handler, NULL,
        MHD_OPTION_END
    );

    if (!server) {
        printf("Failed to start server\n");
        return 1;
    }

    if(initialize_led_settings()) {
        printf("Error initializing Led settings!\n");
        return 1;
    }

    printf("Server running on http://localhost:%d/\n", PORT);
    printf("Press CTRL+C to stop the server.\n");

    if(setup_strip(led_settings.count) != 0) {
        printf("Problem setting up strip...\n");
        return 1;
    }

    set_strip_32int_color(led_settings.color);
    set_brightness(led_settings.brightness);
    show_strip();

    // Keep the server running until stopped
    while (1) {
        sleep(1);
    }
    
    return 0;
}

int handle_get_api_request(struct MHD_Connection *connection, const char *url) {
    led_settings.brightness = 125;
    led_settings.color = 0xCC6CE7;
    led_settings.capture_screen = 0;
    const char *json_response = update_leds();

    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(json_response), (void *)json_response, MHD_RESPMEM_PERSISTENT);
    
    MHD_add_response_header(response, "Content-Type", "application/json");
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    
    MHD_destroy_response(response);
    return ret;
}

int handle_serve_static_files(struct MHD_Connection *connection, const char *url) {
    char file_path[512];
    struct stat file_stat;
    int fd;

    if (strcmp(url, "/") == 0) 
        snprintf(file_path, sizeof(file_path), "%s/index.html", WEB_ROOT);
    else 
        snprintf(file_path, sizeof(file_path), "%s%s", WEB_ROOT, url);

    if (stat(file_path, &file_stat) != 0) {
        const char *not_found = "404 Not Found";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(not_found),
                                                                        (void *)not_found, 
                                                                        MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        MHD_destroy_response(response);
        return ret;
    }

    fd = open(file_path, O_RDONLY);
    if (fd < 0) return MHD_NO;

    struct MHD_Response *response = MHD_create_response_from_fd(file_stat.st_size, fd);
    MHD_add_response_header(response, "Content-Type", get_content_type(file_path));
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}

int handle_get_request(struct MHD_Connection *connection, const char *url) {
    char file_path[512];
    struct stat file_stat;
    int fd;
   
    if(strncmp(url, "/api", 4) == 0) {
      return handle_get_api_request(connection, url);
    } else {
      return handle_serve_static_files(connection, url);
    }
  }

int handle_post_led_settings(struct MHD_Connection *connection, const char *upload_data) {
    //printf("%s\n", upload_data);
    printf("Upload data length: %zu\n", strlen(upload_data)); // Check length
    //const char *test = "{\"brightness\":\"71\",\"color\":\"#FFFFFF\",\"capture_screen\":0,\"sound_react\":0,\"fx_num\":0,\"count\":206,\"id\":0}";
    cJSON *json = cJSON_Parse(upload_data);
    if(!json) {
        fprintf(stderr, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        return 1;
    }
    cJSON *brightness = cJSON_GetObjectItemCaseSensitive(json, "brightness");
    cJSON *color = cJSON_GetObjectItemCaseSensitive(json, "color");
    cJSON *capture_screen = cJSON_GetObjectItemCaseSensitive(json, "capture_screen");
    cJSON *sound_react = cJSON_GetObjectItemCaseSensitive(json, "sound_react");
    cJSON *fx_num = cJSON_GetObjectItemCaseSensitive(json, "fx_num");
    cJSON *count = cJSON_GetObjectItemCaseSensitive(json, "count");
    cJSON *id = cJSON_GetObjectItemCaseSensitive(json, "id");

    if (cJSON_IsNumber(brightness)) led_settings.brightness = brightness->valueint;
    if (cJSON_IsString(color) && color->valuestring) {
        const char *hex = color->valuestring;
        led_settings.color = (int)strtol(hex[0] == '#' ? hex + 1 : hex, NULL, 16);
    }
    if (cJSON_IsNumber(capture_screen)) led_settings.capture_screen = capture_screen->valueint;
    if (cJSON_IsNumber(sound_react)) led_settings.sound_react = sound_react->valueint;
    if (cJSON_IsNumber(fx_num)) led_settings.fx_num = fx_num->valueint;
    if (cJSON_IsNumber(count)) led_settings.count = count->valueint;
    if (cJSON_IsNumber(id)) led_settings.id = id->valueint;

    cJSON_Delete(json);

    const char *response_text;
    int response_code;

    char led_settings_str[256];
    parse_led_settings_data_to_string(led_settings_str);
    printf("current led settings: %s\n", led_settings_str);
    if(write_data(LED_SETTINGS_FILENAME, LED_SETTINGS_HEADER, led_settings_str)) {
        printf("Failed to write led_settings\n");
        response_text = "{\"Error\":\"Failed to write led settings\"}";
        response_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
    } else { // success
        response_text = update_leds();
        response_code = MHD_HTTP_OK;
    }
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(response_text),
                                                  (void *)response_text, 
                                                  MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, response_code, response);
    MHD_destroy_response(response);

    //update_leds();
    return ret;
}
  
int handle_post_request(struct MHD_Connection *connection, const char *url,
  const char *upload_data, size_t *upload_data_size) {
  
  static char post_data[1024]; // Buffer to store received data
  
  // Check if this is the first call or subsequent call
  if (*upload_data_size > 0) {
    size_t len = *upload_data_size;
    if (len >= sizeof(post_data)) len = sizeof(post_data) - 1;

    memcpy(post_data, upload_data, len);
    post_data[len] = '\0'; // Ensure null-terminated string

    printf("UPLOAD DATA (%zu): %s\n", len, post_data); // Optional debug log

    *upload_data_size = 0; // Reset to tell MHD data is processed
    return MHD_YES;        // Continue processing (calls this function again with size 0)
  }

  if(strncmp(url, "/led-settings", 14) == 0) {
    if(*upload_data_size == 0) {
        return handle_post_led_settings(connection, post_data);
    }
  } else {
    // Send a response back to the client
    led_settings.brightness = 125;
    led_settings.capture_screen = 1;
    const char *response_text =  update_leds();
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(response_text),
                                                    (void *)response_text, 
                                                    MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    
    return ret; // Ensure a response is returned
  }
}
  
int handle_delete_request(struct MHD_Connection *connection, const char *url) {
    if (strncmp(url, "/api", 4) == 0) {
        led_settings.capture_screen = 0;
        led_settings.brightness = 0;
        const char *response_text = update_leds();
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(response_text),
                                                                        (void *)response_text, 
                                                                        MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    } else {
        const char *error_text = "Incorrect URL";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_text),
                                                                        (void *)error_text, 
                                                                        MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
        return ret;
    }
}
