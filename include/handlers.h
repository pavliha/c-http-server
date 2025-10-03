#ifndef HANDLERS_H
#define HANDLERS_H

#include "http.h"

// Route handlers
void handle_index(int client_fd, const http_request_t *request);
void handle_dashboard(int client_fd, const http_request_t *request);
void handle_register(int client_fd, const http_request_t *request);
void handle_login(int client_fd, const http_request_t *request);

#endif // HANDLERS_H
