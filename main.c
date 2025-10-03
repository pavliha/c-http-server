#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "static.h"

#define PORT 8080
#define BUFFER_SIZE 1024
#define HTTP_HEADER "HTTP/1.1 200 OK\r\n" \
                    "Content-Type: text/html\r\n" \
                    "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n" \
                    "Pragma: no-cache\r\n" \
                    "Expires: 0\r\n" \
                    "Connection: close\r\n" \
                    "\r\n"

int main() {
  int server_fd, client_fd;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  char buffer[BUFFER_SIZE] = {0};

  // Create socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("Socket failed");
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
    read(client_fd, buffer, BUFFER_SIZE);
    printf("Request received:\n%s\n", buffer);

    // Send HTTP header
    write(client_fd, HTTP_HEADER, strlen(HTTP_HEADER));

    // Send HTML body from embedded file
    ssize_t bytes_written = write(client_fd, embedded_html, embedded_html_len);
    printf("Sent %zd bytes\n", bytes_written);

    // Shutdown write side to signal we're done sending
    shutdown(client_fd, SHUT_WR);

    // Close connection
    close(client_fd);

    // Clear buffer for next request
    memset(buffer, 0, BUFFER_SIZE);
  }

  close(server_fd);
  return 0;
}