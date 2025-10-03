# C HTTP Server

A lightweight HTTP server built from scratch in C using TCP sockets.

## Quick Start

**Development build:**
```bash
cmake --preset dev
cmake --build --preset dev
./build/bin/c-http-server
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

- TCP socket server (port 8080)
- HTTP/1.1 support
- Embedded HTML resources via CMake
- No-cache headers for development
- No external dependencies

## Project Structure

```
.
├── CMakeLists.txt       # Build configuration
├── CMakePresets.json    # Professional presets
├── src/
│   ├── main.c           # Server implementation
│   └── static.h.in      # CMake template for embedding resources
├── include/             # Public headers (currently empty)
├── static/
│   └── index.html       # HTML landing page
└── build/
    ├── bin/             # Compiled executables
    └── include/         # Generated headers
```
