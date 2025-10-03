#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#define MAX_HEADERS 32
#define MAX_HEADER_SIZE 1024

typedef struct {
  char method[16];
  char path[256];
  char version[16];
  char headers[MAX_HEADERS][2][MAX_HEADER_SIZE];
  int header_count;
  char *body;
  int body_length;
} http_request_t;

int http_parse_request(const char *raw_request, http_request_t *request);
void http_free_request(http_request_t *request);
const char *http_get_header(const http_request_t *request, const char *name);
int http_parse_post_data(const char *body, const char *key, char *value, size_t value_size);

#endif // HTTP_H
