#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "db.h"
#include "handlers.h"
#include "http.h"
#include "router.h"

#define PORT 8080
#define BUFFER_SIZE 4096
#define DB_PATH "data/server.db"

static void setup_routes(void) {
  router_init();
  router_register("GET", "/", handle_index);
  router_register("GET", "/dashboard", handle_dashboard);
  router_register("POST", "/register", handle_register);
  router_register("POST", "/login", handle_login);
}

int main() {
  int server_fd, client_fd;
  struct sockaddr_in address;
  int addrlen              = sizeof(address);
  char buffer[BUFFER_SIZE] = {0};

  // Initialize database
  if (db_init(DB_PATH) != 0) {
    fprintf(stderr, "Failed to initialize database\n");
    exit(EXIT_FAILURE);
  }

  // Setup routes
  setup_routes();

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
  address.sin_family      = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port        = htons(PORT);

  if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
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
    if ((client_fd = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
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
      const char *bad_request = "HTTP/1.1 400 Bad Request\r\n"
                                "Content-Type: text/plain\r\n"
                                "Connection: close\r\n"
                                "\r\n"
                                "Bad Request";
      write(client_fd, bad_request, strlen(bad_request));
      close(client_fd);
      memset(buffer, 0, BUFFER_SIZE);
      continue;
    }

    // Handle route
    router_handle(client_fd, &req);
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
