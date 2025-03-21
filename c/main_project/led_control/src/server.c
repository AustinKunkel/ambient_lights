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

int handle_get_request(struct MHD_Connection *connection, const char *url) {
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

int handle_post_request(struct MHD_Connection *connection, const char *url,
  const char *upload_data, size_t *upload_data_size) {
if (*upload_data_size == 0) {
return MHD_NO;
}

printf("Received POST data: %s\n", upload_data);

// Example: Save to a file
FILE *file = fopen("post_data.txt", "w");
if (file) {
fwrite(upload_data, 1, *upload_data_size, file);
fclose(file);
}

const char *response_text = "POST request received!";
struct MHD_Response *response = MHD_create_response_from_buffer(strlen(response_text),
                                              (void *)response_text, 
                                              MHD_RESPMEM_PERSISTENT);
int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
MHD_destroy_response(response);

return ret;
}

int handle_delete_request(struct MHD_Connection *connection, const char *url) {
  char file_path[512];
  snprintf(file_path, sizeof(file_path), "%s%s", WEB_ROOT, url);

  if (remove(file_path) == 0) {
      const char *response_text = "File deleted successfully";
      struct MHD_Response *response = MHD_create_response_from_buffer(strlen(response_text),
                                                                      (void *)response_text, 
                                                                      MHD_RESPMEM_PERSISTENT);
      int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
      MHD_destroy_response(response);
      return ret;
  } else {
      const char *error_text = "Failed to delete file";
      struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_text),
                                                                      (void *)error_text, 
                                                                      MHD_RESPMEM_PERSISTENT);
      int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
      MHD_destroy_response(response);
      return ret;
  }
}

