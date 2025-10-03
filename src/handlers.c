#include "handlers.h"
#include "db.h"
#include "http.h"
#include "static.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

static void send_response(int client_fd, const char *status, const char *content_type,
                          const char *body) {
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

void handle_index(int client_fd, const http_request_t *request) {
  (void) request; // Unused
  send_response(client_fd, "200 OK", "text/html", embedded_html);
}

void handle_dashboard(int client_fd, const http_request_t *request) {
  (void) request; // Unused
  send_response(client_fd, "200 OK", "text/html", embedded_dashboard);
}

void handle_register(int client_fd, const http_request_t *request) {
  char username[256] = {0};
  char password[256] = {0};

  if (request->body &&
      http_parse_post_data(request->body, "username", username, sizeof(username)) == 0 &&
      http_parse_post_data(request->body, "password", password, sizeof(password)) == 0) {

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

void handle_login(int client_fd, const http_request_t *request) {
  char username[256] = {0};
  char password[256] = {0};

  if (request->body &&
      http_parse_post_data(request->body, "username", username, sizeof(username)) == 0 &&
      http_parse_post_data(request->body, "password", password, sizeof(password)) == 0) {

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
