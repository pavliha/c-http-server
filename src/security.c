#include "security.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Session storage
static session_t sessions[MAX_SESSIONS];
static bool sessions_initialized = false;

// Rate limiting storage
static rate_limit_entry_t rate_limits[MAX_RATE_LIMIT_ENTRIES];
static bool rate_limit_initialized = false;

// CSRF token storage
#define MAX_CSRF_TOKENS 100
#define CSRF_TOKEN_TIMEOUT 3600 // 1 hour
static csrf_token_t csrf_tokens[MAX_CSRF_TOKENS];
static bool csrf_initialized = false;

// Constant-time string comparison (timing-attack resistant)
int secure_compare(const char *a, const char *b, size_t len) {
  if (!a || !b)
    return -1;

  size_t a_len = strlen(a);
  size_t b_len = strlen(b);

  // Always compare the same number of bytes
  size_t compare_len = (a_len < b_len) ? b_len : a_len;
  if (len > 0 && len < compare_len) {
    compare_len = len;
  }

  volatile unsigned char result = 0;

  // Compare all bytes, accumulating differences
  for (size_t i = 0; i < compare_len; i++) {
    unsigned char a_byte = (i < a_len) ? (unsigned char) a[i] : 0;
    unsigned char b_byte = (i < b_len) ? (unsigned char) b[i] : 0;
    result |= a_byte ^ b_byte;
  }

  // Also check lengths
  result |= (unsigned char) (a_len ^ b_len);

  return (result == 0) ? 0 : -1;
}

// Generate cryptographically random token
void generate_token(char *buffer, size_t length) {
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  FILE *urandom        = fopen("/dev/urandom", "r");

  if (urandom) {
    unsigned char random_bytes[length];
    fread(random_bytes, 1, length, urandom);
    fclose(urandom);

    for (size_t i = 0; i < length; i++) {
      buffer[i] = charset[random_bytes[i] % (sizeof(charset) - 1)];
    }
  } else {
    // Fallback to rand() if /dev/urandom unavailable
    srand((unsigned int) time(NULL));
    for (size_t i = 0; i < length; i++) {
      buffer[i] = charset[rand() % (sizeof(charset) - 1)];
    }
  }
  buffer[length] = '\0';
}

// Session management
void session_init(void) {
  if (sessions_initialized)
    return;

  memset(sessions, 0, sizeof(sessions));
  sessions_initialized = true;
}

const char *session_create(const char *username) {
  session_cleanup_expired();

  // Find free slot
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (!sessions[i].active) {
      generate_token(sessions[i].token, SESSION_TOKEN_LENGTH);
      strncpy(sessions[i].username, username, sizeof(sessions[i].username) - 1);
      sessions[i].created_at    = time(NULL);
      sessions[i].last_accessed = time(NULL);
      sessions[i].active        = true;
      return sessions[i].token;
    }
  }

  return NULL; // No free slots
}

bool session_validate(const char *token, char *username_out, size_t username_size) {
  if (!token)
    return false;

  time_t now = time(NULL);

  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i].active && secure_compare(sessions[i].token, token, 0) == 0) {
      // Check if expired
      if (now - sessions[i].last_accessed > SESSION_TIMEOUT) {
        sessions[i].active = false;
        return false;
      }

      // Update last accessed
      sessions[i].last_accessed = now;

      if (username_out) {
        strncpy(username_out, sessions[i].username, username_size - 1);
        username_out[username_size - 1] = '\0';
      }

      return true;
    }
  }

  return false;
}

void session_destroy(const char *token) {
  if (!token)
    return;

  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i].active && strcmp(sessions[i].token, token) == 0) {
      sessions[i].active = false;
      memset(&sessions[i], 0, sizeof(session_t));
      return;
    }
  }
}

void session_cleanup_expired(void) {
  time_t now = time(NULL);

  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i].active && (now - sessions[i].last_accessed > SESSION_TIMEOUT)) {
      sessions[i].active = false;
      memset(&sessions[i], 0, sizeof(session_t));
    }
  }
}

// Rate limiting
void rate_limit_init(void) {
  if (rate_limit_initialized)
    return;

  memset(rate_limits, 0, sizeof(rate_limits));
  rate_limit_initialized = true;
}

bool rate_limit_check(const char *ip) {
  if (!ip)
    return false;

  time_t now = time(NULL);

  // Find or create entry for this IP
  int free_slot   = -1;
  int oldest_slot = 0;

  for (int i = 0; i < MAX_RATE_LIMIT_ENTRIES; i++) {
    // Found existing entry
    if (strcmp(rate_limits[i].ip, ip) == 0) {
      // Check if window expired
      if (now - rate_limits[i].window_start > RATE_LIMIT_WINDOW) {
        // Reset window
        rate_limits[i].attempts     = 1;
        rate_limits[i].window_start = now;
        return true;
      }

      // Within window
      rate_limits[i].attempts++;
      return rate_limits[i].attempts <= RATE_LIMIT_MAX_ATTEMPTS;
    }

    // Track free slot
    if (rate_limits[i].ip[0] == '\0' && free_slot == -1) {
      free_slot = i;
    }

    // Track oldest entry for eviction
    if (rate_limits[i].window_start < rate_limits[oldest_slot].window_start) {
      oldest_slot = i;
    }
  }

  // Use free slot or evict oldest
  int slot = (free_slot != -1) ? free_slot : oldest_slot;

  strncpy(rate_limits[slot].ip, ip, sizeof(rate_limits[slot].ip) - 1);
  rate_limits[slot].attempts     = 1;
  rate_limits[slot].window_start = now;

  return true;
}

void rate_limit_cleanup(void) {
  time_t now = time(NULL);

  for (int i = 0; i < MAX_RATE_LIMIT_ENTRIES; i++) {
    if (now - rate_limits[i].window_start > RATE_LIMIT_WINDOW) {
      memset(&rate_limits[i], 0, sizeof(rate_limit_entry_t));
    }
  }
}

// CSRF protection
void csrf_init(void) {
  if (csrf_initialized)
    return;

  memset(csrf_tokens, 0, sizeof(csrf_tokens));
  csrf_initialized = true;
}

const char *csrf_generate(const char *session_token) {
  if (!session_token)
    return NULL;

  csrf_cleanup_expired();

  // Find free slot
  for (int i = 0; i < MAX_CSRF_TOKENS; i++) {
    if (!csrf_tokens[i].active) {
      generate_token(csrf_tokens[i].token, CSRF_TOKEN_LENGTH);
      strncpy(csrf_tokens[i].session_token, session_token,
              sizeof(csrf_tokens[i].session_token) - 1);
      csrf_tokens[i].created_at = time(NULL);
      csrf_tokens[i].active     = true;
      return csrf_tokens[i].token;
    }
  }

  return NULL;
}

bool csrf_validate(const char *csrf_token, const char *session_token) {
  if (!csrf_token || !session_token)
    return false;

  time_t now = time(NULL);

  for (int i = 0; i < MAX_CSRF_TOKENS; i++) {
    if (csrf_tokens[i].active && secure_compare(csrf_tokens[i].token, csrf_token, 0) == 0) {
      // Check if expired
      if (now - csrf_tokens[i].created_at > CSRF_TOKEN_TIMEOUT) {
        csrf_tokens[i].active = false;
        return false;
      }

      // Verify it belongs to this session
      if (secure_compare(csrf_tokens[i].session_token, session_token, 0) != 0) {
        return false;
      }

      // One-time use: invalidate after validation
      csrf_tokens[i].active = false;
      return true;
    }
  }

  return false;
}

void csrf_cleanup_expired(void) {
  time_t now = time(NULL);

  for (int i = 0; i < MAX_CSRF_TOKENS; i++) {
    if (csrf_tokens[i].active && (now - csrf_tokens[i].created_at > CSRF_TOKEN_TIMEOUT)) {
      csrf_tokens[i].active = false;
      memset(&csrf_tokens[i], 0, sizeof(csrf_token_t));
    }
  }
}
