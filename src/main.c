#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "static.h"
#include "db.h"
#include "http.h"

#define PORT 8080
#define BUFFER_SIZE 4096
#define DB_PATH "server.db"

void send_response(int client_fd, const char *status, const char *content_type, const char *body) {
  char response[BUFFER_SIZE];
  snprintf(response, sizeof(response),
    "HTTP/1.1 %s\r\n"
    "Content-Type: %s\r\n"
    "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n"
    "Pragma: no-cache\r\n"
    "Expires: 0\r\n"
    "Connection: close\r\n"
    "\r\n"
    "%s",
    status, content_type, body);

  write(client_fd, response, strlen(response));
}

int main() {
  int server_fd, client_fd;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  char buffer[BUFFER_SIZE] = {0};

  // Initialize database
  if (db_init(DB_PATH) != 0) {
    fprintf(stderr, "Failed to initialize database\n");
    exit(EXIT_FAILURE);
  }

  // Create socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("Socket failed");
    db_close();
    exit(EXIT_FAILURE);
  }

  // Allow socket reuse
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    perror("Setsockopt failed");
    exit(EXIT_FAILURE);
  }

  // Bind to port
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("Bind failed");
    exit(EXIT_FAILURE);
  }

  // Listen for connections
  if (listen(server_fd, 3) < 0) {
    perror("Listen failed");
    exit(EXIT_FAILURE);
  }

  printf("Server listening on port %d...\n", PORT);

  // Main server loop
  while (1) {
    // Accept connection
    if ((client_fd = accept(server_fd, (struct sockaddr *)&address,
                            (socklen_t *)&addrlen)) < 0) {
      perror("Accept failed");
      continue;
    }

    // Read request
    ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read <= 0) {
      close(client_fd);
      continue;
    }
    buffer[bytes_read] = '\0';

    printf("Request received:\n%s\n", buffer);

    // Parse HTTP request
    http_request_t req;
    if (http_parse_request(buffer, &req) != 0) {
      send_response(client_fd, "400 Bad Request", "text/plain", "Bad Request");
      close(client_fd);
      memset(buffer, 0, BUFFER_SIZE);
      continue;
    }

    // Route handling
    if (strcmp(req.method, "POST") == 0 && strcmp(req.path, "/register") == 0) {
      char username[256] = {0};
      char password[256] = {0};

      if (req.body &&
          http_parse_post_data(req.body, "username", username, sizeof(username)) == 0 &&
          http_parse_post_data(req.body, "password", password, sizeof(password)) == 0) {

        if (db_create_user(username, password) == 0) {
          send_response(client_fd, "200 OK", "application/json",
            "{\"success\":true,\"message\":\"User registered successfully\"}");
        } else {
          send_response(client_fd, "400 Bad Request", "application/json",
            "{\"success\":false,\"message\":\"Username already exists\"}");
        }
      } else {
        send_response(client_fd, "400 Bad Request", "application/json",
          "{\"success\":false,\"message\":\"Missing username or password\"}");
      }
    }
    else if (strcmp(req.method, "POST") == 0 && strcmp(req.path, "/login") == 0) {
      char username[256] = {0};
      char password[256] = {0};

      if (req.body &&
          http_parse_post_data(req.body, "username", username, sizeof(username)) == 0 &&
          http_parse_post_data(req.body, "password", password, sizeof(password)) == 0) {

        if (db_verify_user(username, password) == 0) {
          send_response(client_fd, "200 OK", "application/json",
            "{\"success\":true,\"message\":\"Login successful\"}");
        } else {
          send_response(client_fd, "401 Unauthorized", "application/json",
            "{\"success\":false,\"message\":\"Invalid username or password\"}");
        }
      } else {
        send_response(client_fd, "400 Bad Request", "application/json",
          "{\"success\":false,\"message\":\"Missing username or password\"}");
      }
    }
    else if (strcmp(req.method, "GET") == 0 && strcmp(req.path, "/") == 0) {
      send_response(client_fd, "200 OK", "text/html", embedded_html);
    }
    else if (strcmp(req.method, "GET") == 0 && strcmp(req.path, "/dashboard") == 0) {
      send_response(client_fd, "200 OK", "text/html", embedded_dashboard);
    }
    else {
      send_response(client_fd, "404 Not Found", "text/plain", "Not Found");
    }

    http_free_request(&req);

    // Shutdown write side to signal we're done sending
    shutdown(client_fd, SHUT_WR);

    // Close connection
    close(client_fd);

    // Clear buffer for next request
    memset(buffer, 0, BUFFER_SIZE);
  }

  close(server_fd);
  db_close();
  return 0;
}