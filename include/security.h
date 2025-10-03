#ifndef SECURITY_H
#define SECURITY_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// Session management
#define MAX_SESSIONS 100
#define SESSION_TOKEN_LENGTH 32
#define SESSION_TIMEOUT 3600 // 1 hour in seconds

typedef struct {
  char token[SESSION_TOKEN_LENGTH + 1];
  char username[65];
  time_t created_at;
  time_t last_accessed;
  bool active;
} session_t;

// Session functions
void session_init(void);
const char *session_create(const char *username);
bool session_validate(const char *token, char *username_out, size_t username_size);
void session_destroy(const char *token);
void session_cleanup_expired(void);

// Security utilities
int secure_compare(const char *a, const char *b, size_t len);
void generate_token(char *buffer, size_t length);

// CSRF protection
#define CSRF_TOKEN_LENGTH 32

typedef struct {
  char token[CSRF_TOKEN_LENGTH + 1];
  char session_token[SESSION_TOKEN_LENGTH + 1];
  time_t created_at;
  bool active;
} csrf_token_t;

void csrf_init(void);
const char *csrf_generate(const char *session_token);
bool csrf_validate(const char *csrf_token, const char *session_token);
void csrf_cleanup_expired(void);

// Rate limiting
#define MAX_RATE_LIMIT_ENTRIES 100
#define RATE_LIMIT_WINDOW 60 // 1 minute
#define RATE_LIMIT_MAX_ATTEMPTS 5

typedef struct {
  char ip[46]; // IPv6 max length
  int attempts;
  time_t window_start;
} rate_limit_entry_t;

void rate_limit_init(void);
bool rate_limit_check(const char *ip);
void rate_limit_cleanup(void);

#endif // SECURITY_H
