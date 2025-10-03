#include "../vendor/unity/src/unity.h"
#include "../include/db.h"
#include <stdio.h>
#include <unistd.h>

#define TEST_DB "test.db"

void setUp(void) {
    // Remove test database if exists
    unlink(TEST_DB);
    db_init(TEST_DB);
}

void tearDown(void) {
    db_close();
    unlink(TEST_DB);
}

void test_db_init_creates_table(void) {
    sqlite3 *conn = db_get_connection();
    TEST_ASSERT_NOT_NULL(conn);

    // Check if users table exists
    sqlite3_stmt *stmt;
    const char *sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='users';";
    int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, NULL);
    TEST_ASSERT_EQUAL(SQLITE_OK, rc);

    rc = sqlite3_step(stmt);
    TEST_ASSERT_EQUAL(SQLITE_ROW, rc);

    const unsigned char *table_name = sqlite3_column_text(stmt, 0);
    TEST_ASSERT_EQUAL_STRING("users", (const char *)table_name);

    sqlite3_finalize(stmt);
}

void test_db_create_user_success(void) {
    int result = db_create_user("testuser", "testpass");
    TEST_ASSERT_EQUAL(0, result);
}

void test_db_create_user_duplicate(void) {
    db_create_user("testuser", "testpass");
    int result = db_create_user("testuser", "anotherpass");
    TEST_ASSERT_EQUAL(-1, result);
}

void test_db_verify_user_correct_password(void) {
    db_create_user("john", "secret123");
    int result = db_verify_user("john", "secret123");
    TEST_ASSERT_EQUAL(0, result);
}

void test_db_verify_user_wrong_password(void) {
    db_create_user("john", "secret123");
    int result = db_verify_user("john", "wrongpass");
    TEST_ASSERT_EQUAL(-1, result);
}

void test_db_verify_user_nonexistent(void) {
    int result = db_verify_user("nonexistent", "anypass");
    TEST_ASSERT_EQUAL(-1, result);
}

void test_db_multiple_users(void) {
    TEST_ASSERT_EQUAL(0, db_create_user("user1", "pass1"));
    TEST_ASSERT_EQUAL(0, db_create_user("user2", "pass2"));
    TEST_ASSERT_EQUAL(0, db_create_user("user3", "pass3"));

    TEST_ASSERT_EQUAL(0, db_verify_user("user1", "pass1"));
    TEST_ASSERT_EQUAL(0, db_verify_user("user2", "pass2"));
    TEST_ASSERT_EQUAL(0, db_verify_user("user3", "pass3"));

    TEST_ASSERT_EQUAL(-1, db_verify_user("user1", "pass2"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_db_init_creates_table);
    RUN_TEST(test_db_create_user_success);
    RUN_TEST(test_db_create_user_duplicate);
    RUN_TEST(test_db_verify_user_correct_password);
    RUN_TEST(test_db_verify_user_wrong_password);
    RUN_TEST(test_db_verify_user_nonexistent);
    RUN_TEST(test_db_multiple_users);

    return UNITY_END();
}
