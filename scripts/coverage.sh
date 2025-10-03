#!/bin/bash
set -e

echo "=== Building with coverage instrumentation ==="
cmake --preset coverage
cmake --build --preset coverage

echo ""
echo "=== Running tests ==="
cd build-coverage
ctest --output-on-failure

echo ""
echo "=== Generating coverage report ==="
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/vendor/*' '*/tests/*' --output-file coverage.info
lcov --list coverage.info

echo ""
echo "=== Generating HTML report ==="
genhtml coverage.info --output-directory coverage-html

echo ""
echo "✅ Coverage report generated!"
echo "Open: build-coverage/coverage-html/index.html"
