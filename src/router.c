#include "router.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ROUTES 64
#define BUFFER_SIZE 4096

static route_t routes[MAX_ROUTES];
static size_t route_count = 0;

void router_init(void) {
  route_count = 0;
}

void router_register(const char *method, const char *path, route_handler_t handler) {
  if (route_count >= MAX_ROUTES) {
    fprintf(stderr, "Maximum routes exceeded\n");
    return;
  }

  routes[route_count].method  = method;
  routes[route_count].path    = path;
  routes[route_count].handler = handler;
  route_count++;
}

void router_handle(int client_fd, const http_request_t *request) {
  // Find matching route
  for (size_t i = 0; i < route_count; i++) {
    if (strcmp(request->method, routes[i].method) == 0 &&
        strcmp(request->path, routes[i].path) == 0) {
      routes[i].handler(client_fd, request);
      return;
    }
  }

  // No route found - 404
  const char *response = "HTTP/1.1 404 Not Found\r\n"
                         "Content-Type: text/plain\r\n"
                         "Connection: close\r\n"
                         "\r\n"
                         "Not Found";
  write(client_fd, response, strlen(response));
}
