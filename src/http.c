#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int http_parse_request(const char *raw_request, http_request_t *request) {
  memset(request, 0, sizeof(http_request_t));

  // Parse request line
  if (sscanf(raw_request, "%15s %255s %15s", request->method, request->path, request->version) != 3) {
    return -1;
  }

  // Find end of request line
  const char *header_start = strstr(raw_request, "\r\n");
  if (!header_start) return -1;
  header_start += 2;

  // Parse headers
  const char *line = header_start;
  while (*line != '\0' && request->header_count < MAX_HEADERS) {
    if (strncmp(line, "\r\n", 2) == 0) {
      // End of headers
      line += 2;
      break;
    }

    const char *colon = strchr(line, ':');
    const char *line_end = strstr(line, "\r\n");

    if (!colon || !line_end) break;

    int name_len = colon - line;
    if (name_len >= MAX_HEADER_SIZE) break;

    strncpy(request->headers[request->header_count][0], line, name_len);
    request->headers[request->header_count][0][name_len] = '\0';

    // Skip colon and spaces
    const char *value_start = colon + 1;
    while (*value_start == ' ') value_start++;

    int value_len = line_end - value_start;
    if (value_len >= MAX_HEADER_SIZE) break;

    strncpy(request->headers[request->header_count][1], value_start, value_len);
    request->headers[request->header_count][1][value_len] = '\0';

    request->header_count++;
    line = line_end + 2;
  }

  // Body starts after headers
  if (*line != '\0') {
    request->body_length = strlen(line);
    request->body = malloc(request->body_length + 1);
    if (request->body) {
      strcpy(request->body, line);
    }
  }

  return 0;
}

void http_free_request(http_request_t *request) {
  if (request->body) {
    free(request->body);
    request->body = NULL;
  }
}

const char *http_get_header(const http_request_t *request, const char *name) {
  for (int i = 0; i < request->header_count; i++) {
    if (strcasecmp(request->headers[i][0], name) == 0) {
      return request->headers[i][1];
    }
  }
  return NULL;
}

int http_parse_post_data(const char *body, const char *key, char *value, size_t value_size) {
  if (!body || !key || !value) return -1;

  char search_key[256];
  snprintf(search_key, sizeof(search_key), "%s=", key);

  const char *start = strstr(body, search_key);
  if (!start) return -1;

  start += strlen(search_key);
  const char *end = strchr(start, '&');

  size_t len;
  if (end) {
    len = end - start;
  } else {
    len = strlen(start);
  }

  if (len >= value_size) len = value_size - 1;

  strncpy(value, start, len);
  value[len] = '\0';

  // URL decode (simple version - just handle %20 for spaces)
  char *dst = value;
  const char *src = value;
  while (*src) {
    if (*src == '+') {
      *dst++ = ' ';
      src++;
    } else if (*src == '%' && isxdigit(src[1]) && isxdigit(src[2])) {
      char hex[3] = {src[1], src[2], '\0'};
      *dst++ = (char)strtol(hex, NULL, 16);
      src += 3;
    } else {
      *dst++ = *src++;
    }
  }
  *dst = '\0';

  return 0;
}
