#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "db.h"
#include "handlers.h"
#include "http.h"
#include "router.h"
#include "security.h"
#include "thread_pool.h"
#include "tls.h"

#define PORT 8080
#define TLS_PORT 8443
#define BUFFER_SIZE 4096
#define DB_PATH "data/server.db"
#define CERT_PATH "certs/cert.pem"
#define KEY_PATH "certs/key.pem"
#define THREAD_POOL_SIZE 8

// Client connection context
typedef struct {
  int client_fd;
  char client_ip[46];
  SSL_CTX *ssl_ctx; // NULL for HTTP, non-NULL for HTTPS
} client_context_t;

// Global state for cleanup
static int g_server_fd                            = -1;
static SSL_CTX *g_ssl_ctx                         = NULL;
static thread_pool_t *g_thread_pool               = NULL;
static volatile sig_atomic_t g_shutdown_requested = 0;

static void cleanup(void) {
  if (g_thread_pool) {
    thread_pool_destroy(g_thread_pool);
    g_thread_pool = NULL;
  }
  if (g_server_fd >= 0) {
    close(g_server_fd);
    g_server_fd = -1;
  }
  if (g_ssl_ctx) {
    tls_cleanup_context(g_ssl_ctx);
    g_ssl_ctx = NULL;
  }
  db_close();
  printf("\nServer shut down gracefully\n");
}

static void signal_handler(int signum) {
  (void) signum;
  g_shutdown_requested = 1;
}

static void setup_routes(void) {
  router_init();

  // Register global logging middleware
  router_use_global_middleware(logging_middleware);

  // Register routes
  router_register("GET", "/", handle_index);

  // Protected routes - require authentication
  middleware_t auth_middlewares[] = {auth_middleware};
  router_register_with_middleware("GET", "/dashboard", handle_dashboard, auth_middlewares, 1);
  router_register_with_middleware("POST", "/logout", handle_logout, auth_middlewares, 1);

  router_register("POST", "/register", handle_register);
  router_register("POST", "/login", handle_login);
}

static void handle_client_connection(void *arg) {
  client_context_t *ctx    = (client_context_t *) arg;
  char buffer[BUFFER_SIZE] = {0};
  SSL *ssl                 = NULL;

  // Handle TLS if needed
  if (ctx->ssl_ctx) {
    ssl = tls_accept_connection(ctx->ssl_ctx, ctx->client_fd);
    if (!ssl) {
      fprintf(stderr, "Failed to establish SSL connection\n");
      close(ctx->client_fd);
      free(ctx);
      return;
    }
  }

  // Read request
  ssize_t bytes_read;
  if (ssl) {
    bytes_read = tls_read(ssl, buffer, BUFFER_SIZE - 1);
  } else {
    bytes_read = read(ctx->client_fd, buffer, BUFFER_SIZE - 1);
  }

  if (bytes_read <= 0) {
    if (ssl)
      tls_close(ssl);
    close(ctx->client_fd);
    free(ctx);
    return;
  }
  buffer[bytes_read] = '\0';

  // Parse HTTP request
  http_request_t req;
  if (http_parse_request(buffer, &req) != 0) {
    const char *bad_request = "HTTP/1.1 400 Bad Request\r\n"
                              "Content-Type: text/plain\r\n"
                              "Connection: close\r\n\r\nBad Request";
    if (ssl) {
      tls_write(ssl, bad_request, strlen(bad_request));
      tls_close(ssl);
    } else {
      write(ctx->client_fd, bad_request, strlen(bad_request));
    }
    close(ctx->client_fd);
    free(ctx);
    return;
  }

  // Set client IP and SSL in request
  strncpy(req.client_ip, ctx->client_ip, sizeof(req.client_ip) - 1);
  req.ssl = ssl;

  // Handle route
  router_handle(ctx->client_fd, &req);
  http_free_request(&req);

  // Cleanup
  if (ssl) {
    tls_close(ssl);
  } else {
    shutdown(ctx->client_fd, SHUT_WR);
  }
  close(ctx->client_fd);
  free(ctx);
}

int main(int argc, char *argv[]) {
  int server_fd, client_fd;
  struct sockaddr_in address;
  char buffer[BUFFER_SIZE] = {0};
  bool use_tls             = false;
  int port                 = PORT;

  // Check for --tls flag
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--tls") == 0) {
      use_tls = true;
      port    = TLS_PORT;
      break;
    }
  }

  // Setup signal handlers
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // Register cleanup function
  atexit(cleanup);

  // Initialize database
  if (db_init(DB_PATH) != 0) {
    fprintf(stderr, "Failed to initialize database\n");
    exit(EXIT_FAILURE);
  }

  // Initialize security modules
  session_init();
  rate_limit_init();
  csrf_init();

  // Initialize TLS if requested
  if (use_tls) {
    tls_init();
    g_ssl_ctx = tls_create_context(CERT_PATH, KEY_PATH);
    if (!g_ssl_ctx) {
      fprintf(stderr, "Failed to create SSL context\n");
      fprintf(stderr, "Make sure certificates exist at: %s and %s\n", CERT_PATH, KEY_PATH);
      exit(EXIT_FAILURE);
    }
    printf("TLS/SSL initialized successfully\n");
  }

  // Create thread pool
  g_thread_pool = thread_pool_create(THREAD_POOL_SIZE);
  if (!g_thread_pool) {
    fprintf(stderr, "Failed to create thread pool\n");
    exit(EXIT_FAILURE);
  }
  printf("Thread pool created with %d threads\n", THREAD_POOL_SIZE);

  // Setup routes
  setup_routes();

  // Create socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("Socket failed");
    exit(EXIT_FAILURE);
  }
  g_server_fd = server_fd;

  // Allow socket reuse
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    perror("Setsockopt failed");
    exit(EXIT_FAILURE);
  }

  // Bind to port
  address.sin_family      = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port        = htons(port);

  if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
    perror("Bind failed");
    exit(EXIT_FAILURE);
  }

  // Listen for connections
  if (listen(server_fd, 3) < 0) {
    perror("Listen failed");
    exit(EXIT_FAILURE);
  }

  printf("Server listening on %s://localhost:%d...\n", use_tls ? "https" : "http", port);
  printf("Press Ctrl+C to stop\n");

  // Main server loop
  while (!g_shutdown_requested) {
    // Accept connection
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    if ((client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addrlen)) < 0) {
      if (g_shutdown_requested) {
        break;
      }
      perror("Accept failed");
      continue;
    }

    // Create client context
    client_context_t *ctx = (client_context_t *) malloc(sizeof(client_context_t));
    if (!ctx) {
      close(client_fd);
      continue;
    }

    ctx->client_fd = client_fd;
    ctx->ssl_ctx   = use_tls ? g_ssl_ctx : NULL;
    inet_ntop(AF_INET, &client_addr.sin_addr, ctx->client_ip, sizeof(ctx->client_ip));

    // Add task to thread pool
    if (!thread_pool_add_task(g_thread_pool, handle_client_connection, ctx)) {
      fprintf(stderr, "Failed to add task to thread pool\n");
      close(client_fd);
      free(ctx);
    }
  }

  return 0;
}
