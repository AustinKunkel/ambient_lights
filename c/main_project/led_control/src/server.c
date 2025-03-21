#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define PORT 8080
#define WEB_ROOT "./www" // Serve files from the "www" directory

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

// Function to serve files from the "www" directory
static int request_handler(void *cls, struct MHD_Connection *connection,
                           const char *url, const char *method,
                           const char *version, const char *upload_data,
                           size_t *upload_data_size, void **con_cls) {
    char file_path[512];
    struct stat file_stat;
    int fd;
    
    // Default to "index.html" if root URL is requested
    if (strcmp(url, "/") == 0) 
        snprintf(file_path, sizeof(file_path), "%s/index.html", WEB_ROOT);
    else 
        snprintf(file_path, sizeof(file_path), "%s%s", WEB_ROOT, url);
    
    // Check if file exists
    if (stat(file_path, &file_stat) != 0) {
        const char *not_found = "404 Not Found";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(not_found),
                                                                        (void *)not_found, 
                                                                        MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        MHD_destroy_response(response);
        return ret;
    }

    // Open the file for reading
    fd = open(file_path, O_RDONLY);
    if (fd < 0) return MHD_NO;

    // Create an HTTP response from the file
    struct MHD_Response *response = MHD_create_response_from_fd(file_stat.st_size, fd);
    if (!response) {
        close(fd);
        return MHD_NO;
    }

    // Set the appropriate Content-Type
    MHD_add_response_header(response, "Content-Type", get_content_type(file_path));

    // Send the response
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}

int main() {
    struct MHD_Daemon *server;

    // Start the HTTP server
    server = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, PORT, NULL, NULL,
                              &request_handler, NULL, MHD_OPTION_END);
    if (!server) {
        printf("Failed to start server\n");
        return 1;
    }

    printf("Server running on http://localhost:%d/\n", PORT);
    getchar(); // Wait for user input to stop the server

    MHD_stop_daemon(server);
    return 0;
}
