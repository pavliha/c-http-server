cmake_minimum_required(VERSION 3.10)
project(c-http-server
    VERSION 1.0.0
    DESCRIPTION "Lightweight HTTP server built from scratch in C"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

# Find SQLite3
find_package(SQLite3 REQUIRED)
if(NOT SQLite3_FOUND)
    message(FATAL_ERROR "SQLite3 library not found")
endif()

# Find OpenSSL
find_package(OpenSSL REQUIRED)
if(NOT OPENSSL_FOUND)
    message(FATAL_ERROR "OpenSSL library not found")
endif()

# Read HTML files and convert to C strings
file(READ ${CMAKE_SOURCE_DIR}/static/index.html HTML_CONTENT)
string(REPLACE "\"" "\\\"" HTML_CONTENT "${HTML_CONTENT}")
string(REPLACE "\n" "\\n\"\n    \"" HTML_CONTENT "${HTML_CONTENT}")

file(READ ${CMAKE_SOURCE_DIR}/static/dashboard.html DASHBOARD_CONTENT)
string(REPLACE "\"" "\\\"" DASHBOARD_CONTENT "${DASHBOARD_CONTENT}")
string(REPLACE "\n" "\\n\"\n    \"" DASHBOARD_CONTENT "${DASHBOARD_CONTENT}")

# Configure the header file
configure_file(
    ${CMAKE_SOURCE_DIR}/src/static.h.in
    ${CMAKE_BINARY_DIR}/include/static.h
    @ONLY
)

# Add the executable
add_executable(${PROJECT_NAME}
    src/main.c
    src/db.c
    src/http.c
    src/router.c
    src/handlers.c
    src/security.c
    src/tls.c
    src/thread_pool.c
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
    ${SQLite3_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SQLite3_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    pthread
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Testing
enable_testing()

# Unity test framework
add_library(unity STATIC
    vendor/unity/src/unity.c
)
target_include_directories(unity PUBLIC
    vendor/unity/src
)

# Test executables
add_executable(test_http tests/test_http.c src/http.c)
target_include_directories(test_http PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
)
target_link_libraries(test_http PRIVATE unity ${OPENSSL_LIBRARIES})

add_executable(test_db tests/test_db.c src/db.c src/security.c)
target_include_directories(test_db PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${SQLite3_INCLUDE_DIRS}
)
target_link_libraries(test_db PRIVATE unity ${SQLite3_LIBRARIES})

add_executable(test_router tests/test_router.c src/router.c)
target_include_directories(test_router PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
)
target_link_libraries(test_router PRIVATE unity ${OPENSSL_LIBRARIES})

# Add tests
add_test(NAME HTTPParserTests COMMAND test_http)
add_test(NAME DatabaseTests COMMAND test_db)
add_test(NAME RouterTests COMMAND test_router)

# Test runner target
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_http test_db test_router
    COMMENT "Running all tests"
)

# Development server target
add_custom_target(dev
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/data
    COMMAND ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running development server"
)
