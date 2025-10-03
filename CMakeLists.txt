cmake_minimum_required(VERSION 3.10)
project(c-http-server
    VERSION 1.0.0
    DESCRIPTION "Lightweight HTTP server built from scratch in C"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

# Read HTML file and convert to C string
file(READ ${CMAKE_SOURCE_DIR}/static/index.html HTML_CONTENT)
string(REPLACE "\"" "\\\"" HTML_CONTENT "${HTML_CONTENT}")
string(REPLACE "\n" "\\n\"\n    \"" HTML_CONTENT "${HTML_CONTENT}")

# Configure the header file
configure_file(
    ${CMAKE_SOURCE_DIR}/src/static.h.in
    ${CMAKE_BINARY_DIR}/include/static.h
    @ONLY
)

# Add the executable
add_executable(${PROJECT_NAME}
    src/main.c
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
