#include "../include/router.h"
#include "../vendor/unity/src/unity.h"
#include <string.h>

// Mock handlers and middleware
static int last_handler_called         = 0;
static int middleware_call_count       = 0;
static bool middleware_should_continue = true;

void mock_handler_1(int client_fd, const http_request_t *request, const route_params_t *params) {
  (void) client_fd;
  (void) request;
  (void) params;
  last_handler_called = 1;
}

void mock_handler_2(int client_fd, const http_request_t *request, const route_params_t *params) {
  (void) client_fd;
  (void) request;
  (void) params;
  last_handler_called = 2;
}

bool mock_middleware(int client_fd, const http_request_t *request) {
  (void) client_fd;
  (void) request;
  middleware_call_count++;
  return middleware_should_continue;
}

void setUp(void) {
  last_handler_called        = 0;
  middleware_call_count      = 0;
  middleware_should_continue = true;
  router_init();
}

void tearDown(void) {
  // Nothing to clean up
}

void test_router_init(void) {
  router_init();
  // If we get here without crashing, init worked
  TEST_PASS();
}

void test_simple_route_registration(void) {
  router_register("GET", "/test", mock_handler_1);
  // If we get here without crashing, registration worked
  TEST_PASS();
}

void test_path_params_extraction(void) {
  // Register route with path parameters
  router_register("GET", "/users/:id", mock_handler_1);

  // Create a mock request
  http_request_t request = {0};
  strncpy(request.method, "GET", sizeof(request.method) - 1);
  strncpy(request.path, "/users/123", sizeof(request.path) - 1);
  request.body = NULL;

  // Route should match and handler should be called
  router_handle(-1, &request); // -1 as dummy fd

  TEST_ASSERT_EQUAL(1, last_handler_called);
}

void test_middleware_registration(void) {
  router_use_global_middleware(mock_middleware);
  router_register("GET", "/test", mock_handler_1);

  http_request_t request = {0};
  strncpy(request.method, "GET", sizeof(request.method) - 1);
  strncpy(request.path, "/test", sizeof(request.path) - 1);
  request.body = NULL;

  router_handle(-1, &request);

  // Middleware should have been called
  TEST_ASSERT_EQUAL(1, middleware_call_count);
  // Handler should have been called
  TEST_ASSERT_EQUAL(1, last_handler_called);
}

void test_route_with_middleware(void) {
  middleware_t middlewares[] = {mock_middleware};
  router_register_with_middleware("POST", "/api", mock_handler_1, middlewares, 1);

  http_request_t request = {0};
  strncpy(request.method, "POST", sizeof(request.method) - 1);
  strncpy(request.path, "/api", sizeof(request.path) - 1);
  request.body = NULL;

  router_handle(-1, &request);

  // Route-specific middleware should have been called
  TEST_ASSERT_EQUAL(1, middleware_call_count);
  // Handler should have been called
  TEST_ASSERT_EQUAL(1, last_handler_called);
}

void test_middleware_stops_request(void) {
  // Set middleware to stop the request
  middleware_should_continue = false;

  router_use_global_middleware(mock_middleware);
  router_register("GET", "/blocked", mock_handler_1);

  http_request_t request = {0};
  strncpy(request.method, "GET", sizeof(request.method) - 1);
  strncpy(request.path, "/blocked", sizeof(request.path) - 1);
  request.body = NULL;

  router_handle(-1, &request);

  // Middleware should have been called
  TEST_ASSERT_EQUAL(1, middleware_call_count);
  // Handler should NOT have been called
  TEST_ASSERT_EQUAL(0, last_handler_called);
}

void test_multiple_routes(void) {
  router_register("GET", "/route1", mock_handler_1);
  router_register("GET", "/route2", mock_handler_2);

  http_request_t request1 = {0};
  strncpy(request1.method, "GET", sizeof(request1.method) - 1);
  strncpy(request1.path, "/route1", sizeof(request1.path) - 1);
  request1.body = NULL;

  router_handle(-1, &request1);
  TEST_ASSERT_EQUAL(1, last_handler_called);

  // Reset for second test
  last_handler_called = 0;

  http_request_t request2 = {0};
  strncpy(request2.method, "GET", sizeof(request2.method) - 1);
  strncpy(request2.path, "/route2", sizeof(request2.path) - 1);
  request2.body = NULL;

  router_handle(-1, &request2);
  TEST_ASSERT_EQUAL(2, last_handler_called);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_router_init);
  RUN_TEST(test_simple_route_registration);
  RUN_TEST(test_path_params_extraction);
  RUN_TEST(test_middleware_registration);
  RUN_TEST(test_route_with_middleware);
  RUN_TEST(test_middleware_stops_request);
  RUN_TEST(test_multiple_routes);

  return UNITY_END();
}
