#include "../vendor/unity/src/unity.h"
#include "../include/http.h"
#include <string.h>

void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
}

void test_http_parse_get_request(void) {
    const char *request =
        "GET /test HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "User-Agent: TestAgent\r\n"
        "\r\n";

    http_request_t req;
    int result = http_parse_request(request, &req);

    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("GET", req.method);
    TEST_ASSERT_EQUAL_STRING("/test", req.path);
    TEST_ASSERT_EQUAL_STRING("HTTP/1.1", req.version);
    TEST_ASSERT_EQUAL(2, req.header_count);

    const char *host = http_get_header(&req, "Host");
    TEST_ASSERT_EQUAL_STRING("localhost", host);

    http_free_request(&req);
}

void test_http_parse_post_request(void) {
    const char *request =
        "POST /login HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "\r\n"
        "username=testuser&password=testpass";

    http_request_t req;
    int result = http_parse_request(request, &req);

    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("POST", req.method);
    TEST_ASSERT_EQUAL_STRING("/login", req.path);
    TEST_ASSERT_NOT_NULL(req.body);
    TEST_ASSERT_EQUAL_STRING("username=testuser&password=testpass", req.body);

    http_free_request(&req);
}

void test_http_parse_post_data(void) {
    const char *body = "username=john&password=secret123&email=john@example.com";
    char username[256];
    char password[256];
    char email[256];

    TEST_ASSERT_EQUAL(0, http_parse_post_data(body, "username", username, sizeof(username)));
    TEST_ASSERT_EQUAL_STRING("john", username);

    TEST_ASSERT_EQUAL(0, http_parse_post_data(body, "password", password, sizeof(password)));
    TEST_ASSERT_EQUAL_STRING("secret123", password);

    TEST_ASSERT_EQUAL(0, http_parse_post_data(body, "email", email, sizeof(email)));
    TEST_ASSERT_EQUAL_STRING("john@example.com", email);

    // Test missing key
    char missing[256];
    TEST_ASSERT_EQUAL(-1, http_parse_post_data(body, "nonexistent", missing, sizeof(missing)));
}

void test_http_parse_post_data_url_encoded(void) {
    const char *body = "name=John+Doe&message=Hello%20World";
    char name[256];
    char message[256];

    TEST_ASSERT_EQUAL(0, http_parse_post_data(body, "name", name, sizeof(name)));
    TEST_ASSERT_EQUAL_STRING("John Doe", name);

    TEST_ASSERT_EQUAL(0, http_parse_post_data(body, "message", message, sizeof(message)));
    TEST_ASSERT_EQUAL_STRING("Hello World", message);
}

void test_http_get_header_case_insensitive(void) {
    const char *request =
        "GET / HTTP/1.1\r\n"
        "Content-Type: text/html\r\n"
        "\r\n";

    http_request_t req;
    http_parse_request(request, &req);

    const char *ct1 = http_get_header(&req, "Content-Type");
    const char *ct2 = http_get_header(&req, "content-type");
    const char *ct3 = http_get_header(&req, "CONTENT-TYPE");

    TEST_ASSERT_EQUAL_STRING("text/html", ct1);
    TEST_ASSERT_EQUAL_STRING("text/html", ct2);
    TEST_ASSERT_EQUAL_STRING("text/html", ct3);

    http_free_request(&req);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_http_parse_get_request);
    RUN_TEST(test_http_parse_post_request);
    RUN_TEST(test_http_parse_post_data);
    RUN_TEST(test_http_parse_post_data_url_encoded);
    RUN_TEST(test_http_get_header_case_insensitive);

    return UNITY_END();
}
