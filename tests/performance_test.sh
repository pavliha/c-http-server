#!/bin/bash

# Performance test script for C HTTP Server
# Tests throughput and latency under various loads

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
SERVER_PORT=8080
TEST_URL="http://127.0.0.1:${SERVER_PORT}/"
MIN_REQUESTS_PER_SEC=10000
MAX_LATENCY_MS=5

# Check if Apache Bench is installed
if ! command -v ab &> /dev/null; then
    echo -e "${RED}Error: Apache Bench (ab) is not installed${NC}"
    echo "Install with: brew install apache-bench (macOS) or apt-get install apache2-utils (Linux)"
    exit 1
fi

# Start server in background
echo "Starting server..."
./bin/c-http-server &
SERVER_PID=$!

# Wait for server to start
sleep 2

# Function to cleanup on exit
cleanup() {
    echo "Cleaning up..."
    kill -9 $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
}
trap cleanup EXIT

# Check if server is running
if ! curl -s "$TEST_URL" > /dev/null; then
    echo -e "${RED}Error: Server failed to start${NC}"
    exit 1
fi

echo -e "${GREEN}Server started successfully${NC}"
echo ""

# Run performance tests
echo "========================================"
echo "Performance Tests"
echo "========================================"
echo ""

# Test 1: Single client throughput
echo "Test 1: Single client baseline (1000 requests)"
echo "----------------------------------------"
OUTPUT=$(ab -n 1000 -c 1 -q "$TEST_URL" 2>&1)
RPS=$(echo "$OUTPUT" | grep "Requests per second" | awk '{print $4}' | cut -d. -f1)
LATENCY=$(echo "$OUTPUT" | grep "Time per request" | head -1 | awk '{print $4}' | cut -d. -f1)

echo "Requests/sec: $RPS"
echo "Latency: ${LATENCY}ms"

if [ "$RPS" -lt "$MIN_REQUESTS_PER_SEC" ]; then
    echo -e "${RED}FAIL: Throughput too low (${RPS} < ${MIN_REQUESTS_PER_SEC})${NC}"
    exit 1
fi

if [ "$LATENCY" -gt "$MAX_LATENCY_MS" ]; then
    echo -e "${YELLOW}WARNING: Latency higher than expected (${LATENCY}ms > ${MAX_LATENCY_MS}ms)${NC}"
fi

echo -e "${GREEN}PASS${NC}"
echo ""

# Test 2: Concurrent clients
echo "Test 2: Concurrent clients (1000 requests, 10 concurrent)"
echo "----------------------------------------"
OUTPUT=$(ab -n 1000 -c 10 -q "$TEST_URL" 2>&1)
RPS=$(echo "$OUTPUT" | grep "Requests per second" | awk '{print $4}' | cut -d. -f1)
FAILED=$(echo "$OUTPUT" | grep "Failed requests" | awk '{print $3}')

echo "Requests/sec: $RPS"
echo "Failed requests: $FAILED"

if [ "$FAILED" != "0" ]; then
    echo -e "${RED}FAIL: ${FAILED} requests failed${NC}"
    exit 1
fi

if [ "$RPS" -lt "$MIN_REQUESTS_PER_SEC" ]; then
    echo -e "${RED}FAIL: Throughput too low (${RPS} < ${MIN_REQUESTS_PER_SEC})${NC}"
    exit 1
fi

echo -e "${GREEN}PASS${NC}"
echo ""

# Test 3: High concurrency
echo "Test 3: High concurrency (1000 requests, 50 concurrent)"
echo "----------------------------------------"
OUTPUT=$(ab -n 1000 -c 50 -q "$TEST_URL" 2>&1)
RPS=$(echo "$OUTPUT" | grep "Requests per second" | awk '{print $4}' | cut -d. -f1)
FAILED=$(echo "$OUTPUT" | grep "Failed requests" | awk '{print $3}')

echo "Requests/sec: $RPS"
echo "Failed requests: $FAILED"

if [ "$FAILED" != "0" ]; then
    echo -e "${RED}FAIL: ${FAILED} requests failed${NC}"
    exit 1
fi

if [ "$RPS" -lt "$MIN_REQUESTS_PER_SEC" ]; then
    echo -e "${RED}FAIL: Throughput too low (${RPS} < ${MIN_REQUESTS_PER_SEC})${NC}"
    exit 1
fi

echo -e "${GREEN}PASS${NC}"
echo ""

# Test 4: POST endpoint
echo "Test 4: POST endpoint (100 requests, 10 concurrent)"
echo "----------------------------------------"
echo "username=testuser&password=testpass123" > /tmp/post_data.txt
OUTPUT=$(ab -n 100 -c 10 -q -p /tmp/post_data.txt -T 'application/x-www-form-urlencoded' "http://127.0.0.1:${SERVER_PORT}/login" 2>&1)
RPS=$(echo "$OUTPUT" | grep "Requests per second" | awk '{print $4}' | cut -d. -f1)
rm /tmp/post_data.txt

echo "Requests/sec: $RPS"

if [ "$RPS" -lt 5000 ]; then
    echo -e "${YELLOW}WARNING: POST throughput lower than expected (${RPS} < 5000)${NC}"
fi

echo -e "${GREEN}PASS${NC}"
echo ""

echo "========================================"
echo -e "${GREEN}All performance tests passed!${NC}"
echo "========================================"

exit 0
