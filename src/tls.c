#include "tls.h"
#include <stdio.h>
#include <stdlib.h>

void tls_init(void) {
  SSL_load_error_strings();
  OpenSSL_add_ssl_algorithms();
}

SSL_CTX *tls_create_context(const char *cert_file, const char *key_file) {
  const SSL_METHOD *method = TLS_server_method();
  SSL_CTX *ctx             = SSL_CTX_new(method);

  if (!ctx) {
    ERR_print_errors_fp(stderr);
    return NULL;
  }

  // Set minimum TLS version to 1.2
  SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

  // Load certificate
  if (SSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(stderr);
    SSL_CTX_free(ctx);
    return NULL;
  }

  // Load private key
  if (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(stderr);
    SSL_CTX_free(ctx);
    return NULL;
  }

  // Verify private key matches certificate
  if (!SSL_CTX_check_private_key(ctx)) {
    fprintf(stderr, "Private key does not match certificate\n");
    SSL_CTX_free(ctx);
    return NULL;
  }

  return ctx;
}

void tls_cleanup_context(SSL_CTX *ctx) {
  if (ctx) {
    SSL_CTX_free(ctx);
  }
  EVP_cleanup();
}

SSL *tls_accept_connection(SSL_CTX *ctx, int client_fd) {
  SSL *ssl = SSL_new(ctx);
  if (!ssl) {
    ERR_print_errors_fp(stderr);
    return NULL;
  }

  SSL_set_fd(ssl, client_fd);

  if (SSL_accept(ssl) <= 0) {
    ERR_print_errors_fp(stderr);
    SSL_free(ssl);
    return NULL;
  }

  return ssl;
}

ssize_t tls_read(SSL *ssl, void *buffer, size_t length) {
  int bytes_read = SSL_read(ssl, buffer, length);
  if (bytes_read <= 0) {
    int error = SSL_get_error(ssl, bytes_read);
    if (error != SSL_ERROR_ZERO_RETURN) {
      ERR_print_errors_fp(stderr);
    }
    return -1;
  }
  return bytes_read;
}

ssize_t tls_write(SSL *ssl, const void *buffer, size_t length) {
  int bytes_written = SSL_write(ssl, buffer, length);
  if (bytes_written <= 0) {
    ERR_print_errors_fp(stderr);
    return -1;
  }
  return bytes_written;
}

void tls_close(SSL *ssl) {
  if (ssl) {
    SSL_shutdown(ssl);
    SSL_free(ssl);
  }
}
