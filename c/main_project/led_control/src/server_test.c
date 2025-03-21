#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#define WEB_ROOT "./www"  // Directory containing HTML, CSS, JS files

// Function to check if a file exists
int file_exists(const char *filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

// Request handler function
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
    char filepath[512];

    // First time only headers are received, do nothing
    if (&dummy != *ptr) {
        *ptr = &dummy;
        return MHD_YES;
    }

    if (*upload_data_size != 0) {
        *upload_data_size = 0;  // Reset data size
        return MHD_YES;
    }

    *ptr = NULL;  // Clear context pointer

    // **Serve Static Files (HTML, CSS, JS)**
    if (strcmp(url, "/") == 0) {
        snprintf(filepath, sizeof(filepath), "%s/index.html", WEB_ROOT);
    } else {
        snprintf(filepath, sizeof(filepath), "%s%s", WEB_ROOT, url);
    }

    if (file_exists(filepath)) {
        FILE *file = fopen(filepath, "rb");
        if (file) {
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            rewind(file);

            char *file_content = malloc(file_size);
            fread(file_content, 1, file_size, file);
            fclose(file);

            response = MHD_create_response_from_buffer(file_size, file_content, MHD_RESPMEM_MUST_FREE);
            ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            return ret;
        }
    }

    // **API Handling (GET, POST, DELETE)**
    if (strncmp(url, "/api", 4) == 0) {
        const char *response_text;
        
        if (strcmp(method, "GET") == 0) {
            response_text = "{\"message\": \"GET request received!\"}";
        } else if (strcmp(method, "POST") == 0) {
            response_text = "{\"message\": \"POST request received!\"}";
        } else if (strcmp(method, "DELETE") == 0) {
            response_text = "{\"message\": \"DELETE request received!\"}";
        } else {
            return MHD_NO;  // Unsupported method
        }

        response = MHD_create_response_from_buffer(strlen(response_text),
                                                   (void *)response_text,
                                                   MHD_RESPMEM_PERSISTENT);
        ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    // **Return 404 Not Found**
    response = MHD_create_response_from_buffer(strlen("404 Not Found"),
                                               (void *)"404 Not Found",
                                               MHD_RESPMEM_PERSISTENT);
    ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    return ret;
}

int main(int argc, char **argv) {
    struct MHD_Daemon *daemon;
    if (argc != 2) {
        printf("Usage: %s PORT\n", argv[0]);
        return 1;
    }

    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD,
                              atoi(argv[1]),
                              NULL,
                              NULL,
                              &ahc_echo,
                              NULL,
                              MHD_OPTION_END);
    if (daemon == NULL)
        return 1;

    (void)getc(stdin);  // Wait for user input before exiting
    MHD_stop_daemon(daemon);
    
    return 0;
}
