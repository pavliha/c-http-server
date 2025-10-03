cmake_minimum_required(VERSION 3.10)
project(c-http-server C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

# Read HTML file and convert to C string
file(READ ${CMAKE_SOURCE_DIR}/static/index.html HTML_CONTENT)
string(REPLACE "\"" "\\\"" HTML_CONTENT "${HTML_CONTENT}")
string(REPLACE "\n" "\\n\"\n    \"" HTML_CONTENT "${HTML_CONTENT}")

# Configure the header file
configure_file(
    ${CMAKE_SOURCE_DIR}/static.h.in
    ${CMAKE_BINARY_DIR}/static.h
    @ONLY
)

# Add the executable
add_executable(main main.c)

# Include the build directory for generated headers
target_include_directories(main PRIVATE ${CMAKE_BINARY_DIR})
