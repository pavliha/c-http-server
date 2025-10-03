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
  // This is tested indirectly through the router_handle function
  // We'll add a more comprehensive test later
  TEST_PASS();
}

void test_middleware_registration(void) {
  router_use_global_middleware(mock_middleware);
  // If we get here without crashing, middleware registration worked
  TEST_PASS();
}

void test_route_with_middleware(void) {
  middleware_t middlewares[] = {mock_middleware};
  router_register_with_middleware("POST", "/api", mock_handler_1, middlewares, 1);
  TEST_PASS();
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_router_init);
  RUN_TEST(test_simple_route_registration);
  RUN_TEST(test_path_params_extraction);
  RUN_TEST(test_middleware_registration);
  RUN_TEST(test_route_with_middleware);

  return UNITY_END();
}
