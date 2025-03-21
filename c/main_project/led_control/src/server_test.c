#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define PORT 8080  // Default port
#define WEB_ROOT "./www"  // Directory to serve files from

// Function to get the MIME type based on file extension
const char *get_mime_type(const char *path) {
    if (strstr(path, ".html")) return "text/html";
    if (strstr(path, ".css")) return "text/css";
    if (strstr(path, ".js")) return "application/javascript";
    return "application/octet-stream"; // Default binary type
}

// HTTP request handler
static enum MHD_Result ahc_echo(void *cls,
                                struct MHD_Connection *connection,
                                const char *url,
                                const char *method,
                                const char *version,
                                const char *upload_data,
                                size_t *upload_data_size,
                                void **ptr) {
    static int dummy;
    struct MHD_Response *response;
    int ret;

    if (0 != strcmp(method, "GET"))
        return MHD_NO; // Reject non-GET requests

    if (&dummy != *ptr) {
        *ptr = &dummy;
        return MHD_YES;
    }

    *ptr = NULL; // Clear context pointer

    // Construct full file path
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s%s", WEB_ROOT, url);

    // Default to index.html if requesting root "/"
    if (strcmp(url, "/") == 0) {
        snprintf(filepath, sizeof(filepath), "%s/index.html", WEB_ROOT);
    }

    // Open file
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        // File not found, return 404
        const char *not_found = "404 Not Found";
        response = MHD_create_response_from_buffer(strlen(not_found),
                                                   (void *)not_found,
                                                   MHD_RESPMEM_PERSISTENT);
        ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        MHD_destroy_response(response);
        return ret;
    }

    // Get file size
    struct stat st;
    fstat(fd, &st);

    // Read file content
    char *buffer = malloc(st.st_size);
    read(fd, buffer, st.st_size);
    close(fd);

    // Get correct MIME type
    const char *mime = get_mime_type(filepath);

    // Send response
    response = MHD_create_response_from_buffer(st.st_size, buffer, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", mime);
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}

int main(int argc, char **argv) {
    struct MHD_Daemon *d;

    // Start the HTTP daemon
    d = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD,
                         PORT, NULL, NULL,
                         &ahc_echo, NULL,
                         MHD_OPTION_END);

    if (d == NULL) {
        printf("Failed to start server\n");
        return 1;
    }

    printf("Server running on http://localhost:%d/\n", PORT);
    getchar(); // Wait for user input
    MHD_stop_daemon(d);
    return 0;
}
