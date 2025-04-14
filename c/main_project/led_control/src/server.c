#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "led_functions.h"
#include "main.h"
#include "csv_control.h"

#define WEB_ROOT "./led_control/www"  // Directory containing HTML, CSS, JS files
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
    if(*line == NULL) return NULL;

    while (**line == ' ' || **line == '\t') (*line)++;

    char *token = strtok(*line, ",");
    if(token) {
        *line = NULL;
    }
    return token;
}

int parse_led_settings_data_to_string(char *str) {
    return sprintf(str, "%d,#%06X,%s,%s,%s,%s,%s",
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
    if(read_one_line("led_control/data/led_settings.csv", data_line, sizeof(data_line)) == 0)
    {
        char *line_ptr = data_line;
        printf("Setting led settings variables...\n");
        led_settings.brightness = atoi(next_token(&line_ptr));
        printf("Brightness: %d\n", led_settings.brightness);
        char *hex = next_token(&line_ptr);
        printf("Hex string: %s", hex);
        if(hex) {
            char *hex_ptr = (hex[0] == '#') ? hex + 1 : hex;
            printf("Hex ptr: %s", hex_ptr);
            led_settings.color = (int)strtol(hex_ptr, NULL, 16);
        } else {
            led_settings.color = 0xDFC57B;
        }
        //printf("Color: #%06X\n", led_settings.color);
        led_settings.capture_screen = atoi(next_token(&line_ptr));
        printf("Capture screen: %d\n", led_settings.capture_screen);
        led_settings.sound_react = atoi(next_token(&line_ptr));
        printf("sound react: %d\n", led_settings.sound_react);
        led_settings.fx_num = atoi(next_token(&line_ptr));
        printf("fx num: %d\n", led_settings.fx_num);
        led_settings.count = atoi(next_token(&line_ptr));
        printf("count: %d\n", led_settings.count);
        led_settings.id = atoi(next_token(&line_ptr));
        printf("id: %d\n", led_settings.id);
        char buffer[256];
        parse_led_settings_data_to_string(buffer);
        printf("Current LED settings: %s", buffer);
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
  
  int handle_post_request(struct MHD_Connection *connection, const char *url,
      const char *upload_data, size_t *upload_data_size) {
  
  static char post_data[1024]; // Buffer to store received data
  
  // Check if this is the first call or subsequent call
  if (*upload_data_size > 0) {
  strncpy(post_data, upload_data, *upload_data_size);
  post_data[*upload_data_size] = '\0'; // Ensure null termination
  
  printf("Received POST data: %s\n", post_data);
  
  *upload_data_size = 0; // Reset to indicate data has been processed
  return MHD_YES; // Return YES to indicate more data may come
  }
  
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
