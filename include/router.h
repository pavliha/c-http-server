#ifndef ROUTER_H
#define ROUTER_H

#include "http.h"

// Route handler function type
typedef void (*route_handler_t)(int client_fd, const http_request_t *request);

// Route structure
typedef struct {
  const char *method;
  const char *path;
  route_handler_t handler;
} route_t;

// Router functions
void router_init(void);
void router_register(const char *method, const char *path, route_handler_t handler);
void router_handle(int client_fd, const http_request_t *request);

#endif // ROUTER_H
