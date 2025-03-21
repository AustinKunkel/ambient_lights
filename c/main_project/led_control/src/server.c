#include <microhttpd.h>
#include <stdio.h>
#include <string.h>

#define PORT 8080  // Define the port number

// Request handler function
static int request_handler(void *cls, struct MHD_Connection *connection, 
                           const char *url, const char *method, 
                           const char *version, const char *upload_data, 
                           size_t *upload_data_size, void **con_cls) {
    
    const char *response_text = "Hello, World!";  // Response message
    struct MHD_Response *response;
    int ret;

    // Create an HTTP response with the message
    response = MHD_create_response_from_buffer(strlen(response_text), 
                                               (void *)response_text, 
                                               MHD_RESPMEM_PERSISTENT);
    if (!response)
        return MHD_NO;

    // Queue response for sending
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}

int main() {
    struct MHD_Daemon *server;

    // Start the HTTP server (use correct flag: MHD_USE_THREAD_PER_CONNECTION)
    server = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, PORT, 
                              NULL, NULL, &request_handler, NULL, 
                              MHD_OPTION_END);
    
    if (!server) {
        printf("Failed to start server\n");
        return 1;
    }

    printf("Server running on port %d...\n", PORT);
    getchar();  // Wait for user input to stop the server

    // Stop the server when done
    MHD_stop_daemon(server);
    
    return 0;
}
