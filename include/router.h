#ifndef ROUTER_H
#define ROUTER_H

#include "http.h"
#include <stdbool.h>

#define MAX_PARAMS 8
#define MAX_PARAM_NAME 32
#define MAX_PARAM_VALUE 256

// Route parameters extracted from URL
typedef struct {
  char name[MAX_PARAM_NAME];
  char value[MAX_PARAM_VALUE];
} route_param_t;

typedef struct {
  route_param_t params[MAX_PARAMS];
  size_t count;
} route_params_t;

// Middleware function type - returns true to continue, false to stop
typedef bool (*middleware_t)(int client_fd, const http_request_t *request);

// Route handler function type with params
typedef void (*route_handler_t)(int client_fd, const http_request_t *request,
                                const route_params_t *params);

// Route structure
typedef struct {
  const char *method;
  const char *path; // Can contain :params like "/users/:id"
  route_handler_t handler;
  middleware_t *middlewares; // Array of middleware functions
  size_t middleware_count;
  bool has_params; // True if path contains :params
} route_t;

// Router functions
void router_init(void);
void router_register(const char *method, const char *path, route_handler_t handler);
void router_register_with_middleware(const char *method, const char *path, route_handler_t handler,
                                     middleware_t *middlewares, size_t middleware_count);
void router_handle(int client_fd, const http_request_t *request);

// Middleware registration
void router_use_global_middleware(middleware_t middleware);

#endif // ROUTER_H
