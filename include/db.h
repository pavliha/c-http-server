#ifndef DB_H
#define DB_H

#include <sqlite3.h>

int db_init(const char *db_path);
void db_close(void);
int db_create_user(const char *username, const char *password);
int db_verify_user(const char *username, const char *password);
sqlite3 *db_get_connection(void);

#endif // DB_H
