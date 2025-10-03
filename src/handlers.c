#include "handlers.h"
#include "db.h"
#include "http.h"
#include "security.h"
#include "static.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define MAX_RESPONSE_SIZE 65536

static void send_response(int client_fd, const char *status, const char *content_type,
                          const char *body) {
  // Send headers first
  char headers[512];
  int header_len = snprintf(headers, sizeof(headers),
                            "HTTP/1.1 %s\r\n"
                            "Content-Type: %s\r\n"
                            "Content-Length: %zu\r\n"
                            "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n"
                            "Pragma: no-cache\r\n"
                            "Expires: 0\r\n"
                            "Connection: close\r\n"
                            "\r\n",
                            status, content_type, strlen(body));

  // Send headers
  ssize_t written = write(client_fd, headers, header_len);
  if (written < 0) {
    perror("Failed to send headers");
    return;
  }

  // Send body
  written = write(client_fd, body, strlen(body));
  if (written < 0) {
    perror("Failed to send body");
  }
}

static int validate_credentials(const char *username, const char *password) {
  if (!username || !password) {
    return -1;
  }

  size_t username_len = strlen(username);
  size_t password_len = strlen(password);

  // Validate lengths
  if (username_len == 0 || username_len > 64) {
    return -1;
  }
  if (password_len == 0 || password_len > 128) {
    return -1;
  }

  // Validate username: alphanumeric and underscore only
  for (size_t i = 0; i < username_len; i++) {
    char c = username[i];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')) {
      return -1;
    }
  }

  return 0;
}

bool logging_middleware(int client_fd, const http_request_t *request) {
  (void) client_fd; // Unused
  printf("[%s] %s\n", request->method, request->path);
  return true; // Continue to next middleware/handler
}

// Helper: Extract session token from Authorization header
static const char *extract_session_token(const http_request_t *request, char *token_buffer,
                                         size_t buffer_size) {
  (void) buffer_size; // Unused - token size is fixed

  const char *auth_header = http_get_header(request, "Authorization");
  if (!auth_header)
    return NULL;

  if (sscanf(auth_header, "Bearer %32s", token_buffer) != 1)
    return NULL;

  return token_buffer;
}

bool auth_middleware(int client_fd, const http_request_t *request) {
  // Extract token from Authorization header
  char token[SESSION_TOKEN_LENGTH + 1] = {0};
  if (!extract_session_token(request, token, sizeof(token))) {
    const char *response = "HTTP/1.1 401 Unauthorized\r\n"
                           "Content-Type: application/json\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "{\"success\":false,\"message\":\"No authorization token provided\"}";
    write(client_fd, response, strlen(response));
    return false;
  }

  // Validate session
  char username[65];
  if (!session_validate(token, username, sizeof(username))) {
    const char *response = "HTTP/1.1 401 Unauthorized\r\n"
                           "Content-Type: application/json\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "{\"success\":false,\"message\":\"Invalid or expired session\"}";
    write(client_fd, response, strlen(response));
    return false;
  }

  return true; // Authenticated, continue
}

void handle_index(int client_fd, const http_request_t *request, const route_params_t *params) {
  (void) request; // Unused
  (void) params;  // Unused
  send_response(client_fd, "200 OK", "text/html", embedded_html);
}

void handle_dashboard(int client_fd, const http_request_t *request, const route_params_t *params) {
  (void) request; // Unused
  (void) params;  // Unused
  send_response(client_fd, "200 OK", "text/html", embedded_dashboard);
}

void handle_register(int client_fd, const http_request_t *request, const route_params_t *params) {
  (void) params; // Unused
  char username[256] = {0};
  char password[256] = {0};

  if (!request->body ||
      http_parse_post_data(request->body, "username", username, sizeof(username)) != 0 ||
      http_parse_post_data(request->body, "password", password, sizeof(password)) != 0) {
    send_response(client_fd, "400 Bad Request", "application/json",
                  "{\"success\":false,\"message\":\"Missing username or password\"}");
    return;
  }

  // Validate credentials
  if (validate_credentials(username, password) != 0) {
    send_response(
        client_fd, "400 Bad Request", "application/json",
        "{\"success\":false,\"message\":\"Invalid username or password format. Username must "
        "be alphanumeric (1-64 chars), password 1-128 chars\"}");
    return;
  }

  if (db_create_user(username, password) == 0) {
    send_response(client_fd, "200 OK", "application/json",
                  "{\"success\":true,\"message\":\"User registered successfully\"}");
  } else {
    send_response(client_fd, "400 Bad Request", "application/json",
                  "{\"success\":false,\"message\":\"Username already exists\"}");
  }
}

void handle_logout(int client_fd, const http_request_t *request, const route_params_t *params) {
  (void) params; // Unused

  // Extract session token
  char token[SESSION_TOKEN_LENGTH + 1] = {0};
  if (!extract_session_token(request, token, sizeof(token))) {
    send_response(client_fd, "400 Bad Request", "application/json",
                  "{\"success\":false,\"message\":\"No session token provided\"}");
    return;
  }

  // Destroy session
  session_destroy(token);

  send_response(client_fd, "200 OK", "application/json",
                "{\"success\":true,\"message\":\"Logged out successfully\"}");
}

void handle_login(int client_fd, const http_request_t *request, const route_params_t *params) {
  (void) params; // Unused

  // Rate limiting check (using a placeholder IP - in real implementation, extract from socket)
  if (!rate_limit_check("127.0.0.1")) {
    send_response(
        client_fd, "429 Too Many Requests", "application/json",
        "{\"success\":false,\"message\":\"Too many login attempts. Please try again later\"}");
    return;
  }

  char username[256] = {0};
  char password[256] = {0};

  if (!request->body ||
      http_parse_post_data(request->body, "username", username, sizeof(username)) != 0 ||
      http_parse_post_data(request->body, "password", password, sizeof(password)) != 0) {
    send_response(client_fd, "400 Bad Request", "application/json",
                  "{\"success\":false,\"message\":\"Missing username or password\"}");
    return;
  }

  // Validate credentials format
  if (validate_credentials(username, password) != 0) {
    send_response(client_fd, "400 Bad Request", "application/json",
                  "{\"success\":false,\"message\":\"Invalid username or password format\"}");
    return;
  }

  if (db_verify_user(username, password) == 0) {
    // Create session
    const char *token = session_create(username);
    if (token) {
      // Generate CSRF token
      const char *csrf_token = csrf_generate(token);
      if (csrf_token) {
        char response[768];
        snprintf(response, sizeof(response),
                 "{\"success\":true,\"message\":\"Login successful\",\"token\":\"%s\",\"csrf_"
                 "token\":\"%s\"}",
                 token, csrf_token);
        send_response(client_fd, "200 OK", "application/json", response);
      } else {
        send_response(client_fd, "500 Internal Server Error", "application/json",
                      "{\"success\":false,\"message\":\"Failed to create CSRF token\"}");
      }
    } else {
      send_response(client_fd, "500 Internal Server Error", "application/json",
                    "{\"success\":false,\"message\":\"Failed to create session\"}");
    }
  } else {
    send_response(client_fd, "401 Unauthorized", "application/json",
                  "{\"success\":false,\"message\":\"Invalid username or password\"}");
  }
}
