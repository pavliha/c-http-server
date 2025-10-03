#!/bin/bash
set -e

echo "=== Static Analysis Tools ==="
echo ""

# Check if tools are installed
command -v cppcheck >/dev/null 2>&1 || { echo "⚠️  cppcheck not installed. Run: brew install cppcheck"; MISSING=1; }
command -v scan-build >/dev/null 2>&1 || { echo "⚠️  scan-build not installed. Install via Xcode Command Line Tools"; MISSING=1; }

if [ "$MISSING" = "1" ]; then
    echo ""
    echo "Install missing tools and re-run this script."
    exit 1
fi

echo "--- cppcheck: Static code analysis ---"
cppcheck --enable=all \
         --inconclusive \
         --std=c11 \
         --suppress=missingIncludeSystem \
         --quiet \
         src/ include/

echo "✅ cppcheck passed"
echo ""

echo "--- Clang Static Analyzer ---"
rm -rf build-analyze
scan-build -o build-analyze cmake -B build-analyze -DCMAKE_BUILD_TYPE=Debug
scan-build -o build-analyze cmake --build build-analyze

echo ""
echo "✅ Static analysis complete!"
echo "Check build-analyze/ for detailed reports"
