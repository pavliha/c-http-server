#ifndef HANDLERS_H
#define HANDLERS_H

#include "http.h"
#include "router.h"

// Route handlers
void handle_index(int client_fd, const http_request_t *request, const route_params_t *params);
void handle_dashboard(int client_fd, const http_request_t *request, const route_params_t *params);
void handle_register(int client_fd, const http_request_t *request, const route_params_t *params);
void handle_login(int client_fd, const http_request_t *request, const route_params_t *params);

// Middleware
bool logging_middleware(int client_fd, const http_request_t *request);
bool auth_middleware(int client_fd, const http_request_t *request);

#endif // HANDLERS_H
