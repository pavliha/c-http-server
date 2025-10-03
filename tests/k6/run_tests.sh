#!/bin/bash

# k6 Performance Test Runner
# Runs performance tests and tracks metrics over time

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Configuration
RESULTS_DIR="tests/k6/results"
METRICS_DB="$RESULTS_DIR/metrics.jsonl"
BASE_URL="${BASE_URL:-http://localhost:8080}"

# Check if k6 is installed
if ! command -v k6 &> /dev/null; then
    echo -e "${RED}Error: k6 is not installed${NC}"
    echo "Install with: brew install k6 (macOS) or https://k6.io/docs/get-started/installation/"
    exit 1
fi

# Create results directory
mkdir -p "$RESULTS_DIR"

# Start server if not already running
SERVER_STARTED=0
if ! curl -s "$BASE_URL" > /dev/null 2>&1; then
    echo "Starting server..."
    cd build 2>/dev/null || (echo "Build directory not found. Run cmake build first." && exit 1)
    mkdir -p data
    ./bin/c-http-server &
    SERVER_PID=$!
    SERVER_STARTED=1
    cd ..
    sleep 2
fi

# Cleanup function
cleanup() {
    if [ $SERVER_STARTED -eq 1 ] && [ -n "$SERVER_PID" ]; then
        echo "Stopping server..."
        kill -9 $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
    fi
}
trap cleanup EXIT

# Run test based on argument
TEST_TYPE="${1:-smoke}"

echo "========================================"
echo "Running k6 ${TEST_TYPE} test"
echo "========================================"
echo ""

case "$TEST_TYPE" in
    smoke)
        k6 run --env BASE_URL="$BASE_URL" --out json="$RESULTS_DIR/smoke_$(date +%Y%m%d_%H%M%S).json" tests/k6/smoke_test.js
        ;;
    load)
        k6 run --env BASE_URL="$BASE_URL" --out json="$RESULTS_DIR/load_$(date +%Y%m%d_%H%M%S).json" tests/k6/load_test.js
        ;;
    stress)
        k6 run --env BASE_URL="$BASE_URL" --out json="$RESULTS_DIR/stress_$(date +%Y%m%d_%H%M%S).json" tests/k6/stress_test.js
        ;;
    *)
        echo -e "${RED}Unknown test type: $TEST_TYPE${NC}"
        echo "Usage: $0 [smoke|load|stress]"
        exit 1
        ;;
esac

TEST_EXIT_CODE=$?

# Store metrics in JSONL database
if [ -f "$RESULTS_DIR/summary.json" ]; then
    TIMESTAMP=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    RPS=$(jq -r '.metrics.http_reqs.values.rate' "$RESULTS_DIR/summary.json" 2>/dev/null || echo "0")
    AVG_DURATION=$(jq -r '.metrics.http_req_duration.values.avg' "$RESULTS_DIR/summary.json" 2>/dev/null || echo "0")
    P95_DURATION=$(jq -r '.metrics.http_req_duration.values["p(95)"]' "$RESULTS_DIR/summary.json" 2>/dev/null || echo "0")
    ERROR_RATE=$(jq -r '.metrics.http_req_failed.values.rate' "$RESULTS_DIR/summary.json" 2>/dev/null || echo "0")

    # Append to metrics database
    echo "{\"timestamp\":\"$TIMESTAMP\",\"test_type\":\"$TEST_TYPE\",\"rps\":$RPS,\"avg_duration\":$AVG_DURATION,\"p95_duration\":$P95_DURATION,\"error_rate\":$ERROR_RATE}" >> "$METRICS_DB"

    echo ""
    echo "========================================"
    echo "Metrics saved to database"
    echo "========================================"
    echo "Timestamp: $TIMESTAMP"
    echo "RPS: $RPS"
    echo "Avg Duration: ${AVG_DURATION}ms"
    echo "P95 Duration: ${P95_DURATION}ms"
    echo "Error Rate: ${ERROR_RATE}"
fi

# Compare with baseline if it exists
BASELINE_FILE="$RESULTS_DIR/baseline.json"
if [ -f "$BASELINE_FILE" ]; then
    echo ""
    echo "========================================"
    echo "Comparing with baseline"
    echo "========================================"

    BASELINE_RPS=$(jq -r '.rps' "$BASELINE_FILE")
    CURRENT_RPS="$RPS"

    DEGRADATION=$(echo "scale=2; (($BASELINE_RPS - $CURRENT_RPS) / $BASELINE_RPS) * 100" | bc)

    echo "Baseline RPS: $BASELINE_RPS"
    echo "Current RPS: $CURRENT_RPS"

    if (( $(echo "$DEGRADATION > 10" | bc -l) )); then
        echo -e "${RED}WARNING: Performance degraded by ${DEGRADATION}%${NC}"
        TEST_EXIT_CODE=1
    elif (( $(echo "$DEGRADATION > 5" | bc -l) )); then
        echo -e "${YELLOW}NOTICE: Performance degraded by ${DEGRADATION}%${NC}"
    else
        echo -e "${GREEN}Performance within acceptable range${NC}"
    fi
fi

exit $TEST_EXIT_CODE
