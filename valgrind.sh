#!/bin/bash
set -e

echo "=== Building tests ==="
cmake --preset dev
cmake --build --preset dev

echo ""
echo "=== Running tests with Valgrind ==="
echo ""

echo "--- HTTP Parser Tests ---"
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --error-exitcode=1 \
         ./build/test_http

echo ""
echo "--- Database Tests ---"
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --error-exitcode=1 \
         ./build/test_db

echo ""
echo "✅ Valgrind checks passed - No memory leaks detected!"
