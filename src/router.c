#include "router.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ROUTES 64
#define MAX_GLOBAL_MIDDLEWARES 16

static route_t routes[MAX_ROUTES];
static size_t route_count = 0;

static middleware_t global_middlewares[MAX_GLOBAL_MIDDLEWARES];
static size_t global_middleware_count = 0;

void router_init(void) {
  route_count             = 0;
  global_middleware_count = 0;
}

// Check if path contains parameters (e.g., "/users/:id")
static bool path_has_params(const char *path) {
  return strchr(path, ':') != NULL;
}

// Match route path with request path and extract parameters
// Returns true if match, false otherwise
static bool match_route(const char *route_path, const char *request_path, route_params_t *params) {
  params->count = 0;

  const char *r = route_path;
  const char *p = request_path;

  while (*r && *p) {
    if (*r == ':') {
      // Found parameter
      r++; // Skip ':'

      // Extract parameter name
      char param_name[MAX_PARAM_NAME] = {0};
      size_t name_len                 = 0;
      while (*r && *r != '/' && name_len < MAX_PARAM_NAME - 1) {
        param_name[name_len++] = *r++;
      }

      // Extract parameter value from path
      char param_value[MAX_PARAM_VALUE] = {0};
      size_t value_len                  = 0;
      while (*p && *p != '/' && value_len < MAX_PARAM_VALUE - 1) {
        param_value[value_len++] = *p++;
      }

      // Store parameter
      if (params->count < MAX_PARAMS) {
        strncpy(params->params[params->count].name, param_name, MAX_PARAM_NAME - 1);
        strncpy(params->params[params->count].value, param_value, MAX_PARAM_VALUE - 1);
        params->count++;
      }
    } else if (*r == *p) {
      r++;
      p++;
    } else {
      return false;
    }
  }

  // Both should reach end for exact match
  return *r == *p;
}

void router_register(const char *method, const char *path, route_handler_t handler) {
  router_register_with_middleware(method, path, handler, NULL, 0);
}

void router_register_with_middleware(const char *method, const char *path, route_handler_t handler,
                                     middleware_t *middlewares, size_t middleware_count) {
  if (route_count >= MAX_ROUTES) {
    fprintf(stderr, "Maximum routes exceeded\n");
    return;
  }

  routes[route_count].method           = method;
  routes[route_count].path             = path;
  routes[route_count].handler          = handler;
  routes[route_count].middlewares      = middlewares;
  routes[route_count].middleware_count = middleware_count;
  routes[route_count].has_params       = path_has_params(path);
  route_count++;
}

void router_use_global_middleware(middleware_t middleware) {
  if (global_middleware_count >= MAX_GLOBAL_MIDDLEWARES) {
    fprintf(stderr, "Maximum global middlewares exceeded\n");
    return;
  }
  global_middlewares[global_middleware_count++] = middleware;
}

void router_handle(int client_fd, const http_request_t *request) {
  // Run global middlewares first
  for (size_t i = 0; i < global_middleware_count; i++) {
    if (!global_middlewares[i](client_fd, request)) {
      return; // Middleware stopped the request
    }
  }

  // Find matching route
  for (size_t i = 0; i < route_count; i++) {
    if (strcmp(request->method, routes[i].method) != 0) {
      continue;
    }

    route_params_t params = {0};
    bool matched          = false;

    if (routes[i].has_params) {
      matched = match_route(routes[i].path, request->path, &params);
    } else {
      matched = strcmp(request->path, routes[i].path) == 0;
    }

    if (matched) {
      // Run route-specific middlewares
      for (size_t j = 0; j < routes[i].middleware_count; j++) {
        if (!routes[i].middlewares[j](client_fd, request)) {
          return; // Middleware stopped the request
        }
      }

      // Call handler
      routes[i].handler(client_fd, request, &params);
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
