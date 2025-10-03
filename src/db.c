#include "db.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static sqlite3 *db = NULL;

int db_init(const char *db_path) {
  int rc = sqlite3_open(db_path, &db);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
    return -1;
  }

  // Create users table
  const char *sql =
    "CREATE TABLE IF NOT EXISTS users ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  username TEXT UNIQUE NOT NULL,"
    "  password TEXT NOT NULL,"
    "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
    ");";

  char *err_msg = NULL;
  rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", err_msg);
    sqlite3_free(err_msg);
    return -1;
  }

  printf("Database initialized successfully\n");
  return 0;
}

void db_close(void) {
  if (db) {
    sqlite3_close(db);
    db = NULL;
  }
}

int db_create_user(const char *username, const char *password) {
  const char *sql = "INSERT INTO users (username, password) VALUES (?, ?);";
  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return -1;
  }

  sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc != SQLITE_DONE) {
    fprintf(stderr, "Failed to create user: %s\n", sqlite3_errmsg(db));
    return -1;
  }

  return 0;
}

int db_verify_user(const char *username, const char *password) {
  const char *sql = "SELECT password FROM users WHERE username = ?;";
  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return -1;
  }

  sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    const unsigned char *stored_password = sqlite3_column_text(stmt, 0);
    int match = strcmp(password, (const char *)stored_password) == 0;
    sqlite3_finalize(stmt);
    return match ? 0 : -1;
  }

  sqlite3_finalize(stmt);
  return -1;
}

sqlite3 *db_get_connection(void) {
  return db;
}
