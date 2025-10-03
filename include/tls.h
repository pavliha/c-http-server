#ifndef TLS_H
#define TLS_H

#include <openssl/err.h>
#include <openssl/ssl.h>

// Initialize OpenSSL library
void tls_init(void);

// Create SSL context with certificate and key
SSL_CTX *tls_create_context(const char *cert_file, const char *key_file);

// Clean up SSL context
void tls_cleanup_context(SSL_CTX *ctx);

// Create SSL connection from socket
SSL *tls_accept_connection(SSL_CTX *ctx, int client_fd);

// Read from SSL connection
ssize_t tls_read(SSL *ssl, void *buffer, size_t length);

// Write to SSL connection
ssize_t tls_write(SSL *ssl, const void *buffer, size_t length);

// Close SSL connection
void tls_close(SSL *ssl);

#endif // TLS_H
