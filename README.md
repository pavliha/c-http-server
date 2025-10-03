# C HTTP Server

A lightweight HTTP server built from scratch in C using TCP sockets.

## Quick Start

**Development build:**
```bash
cmake --preset dev
cmake --build --preset dev
./build/bin/c-http-server
```

**Run tests:**
```bash
cd build && ctest --output-on-failure
```

**Release build:**
```bash
cmake --preset release
cmake --build --preset release
./build-release/bin/c-http-server
```

Then open http://localhost:8080 in your browser.

## CMake Presets

This project uses CMake Presets (CMake 3.21+) for professional workflow:

- **dev** - Debug build with compile commands for IDEs
- **release** - Optimized production build

### Available Commands

```bash
# List all presets
cmake --list-presets

# Configure
cmake --preset dev

# Build
cmake --build --preset dev

# Clean and rebuild
rm -rf build && cmake --preset dev && cmake --build --preset dev
```

## IDE Support

Modern IDEs automatically detect `CMakePresets.json`:

- **VS Code**: Install CMake Tools extension
- **CLion**: Open folder, select preset
- **Visual Studio**: Native support (2019+)

## Features

- **TCP socket server** on port 8080
- **HTTP/1.1** request parsing with POST support
- **SQLite authentication** - user registration and login
- **Modern auth UI** with client-side JavaScript
- **Embedded HTML** resources via CMake
- **Professional structure** - src/include separation
- **Route handling** - /register, /login, / endpoints
- **JSON responses** for API endpoints

## API Endpoints

- `GET /` - Auth UI (login/register form)
- `POST /register` - Create new user
  - Body: `username=USER&password=PASS`
  - Returns: JSON with success/error
- `POST /login` - Authenticate user
  - Body: `username=USER&password=PASS`
  - Returns: JSON with success/error

## Testing

The project uses **Unity** testing framework with comprehensive test coverage:

- **HTTP Parser Tests** (5 tests)
  - GET/POST request parsing
  - URL-encoded POST data parsing
  - Header retrieval (case-insensitive)

- **Database Tests** (7 tests)
  - User creation and validation
  - Duplicate user handling
  - Password verification
  - Multi-user scenarios

```bash
# Run all tests
cd build && ctest

# Run specific test
./build/test_http
./build/test_db

# Run with verbose output
ctest --output-on-failure
```

## Project Structure

```
.
├── CMakeLists.txt       # Build configuration with SQLite + Testing
├── CMakePresets.json    # Professional presets (dev/release)
├── src/
│   ├── main.c           # Server and routing
│   ├── db.c             # SQLite database operations
│   ├── http.c           # HTTP request parser
│   └── static.h.in      # CMake template for embedding HTML
├── include/
│   ├── db.h             # Database API
│   └── http.h           # HTTP parser API
├── tests/
│   ├── test_http.c      # HTTP parser unit tests
│   └── test_db.c        # Database unit tests
├── vendor/
│   └── unity/           # Unity test framework (submodule)
├── static/
│   ├── index.html       # Auth UI
│   └── dashboard.html   # Protected dashboard
├── build/
│   ├── bin/             # c-http-server executable
│   ├── test_http        # HTTP test runner
│   ├── test_db          # Database test runner
│   └── include/         # Generated headers
└── server.db            # SQLite database (auto-created)
```
