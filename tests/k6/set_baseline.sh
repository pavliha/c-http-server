#!/bin/bash

# Set performance baseline from current test run

set -e

RESULTS_DIR="tests/k6/results"
BASELINE_FILE="$RESULTS_DIR/baseline.json"

if [ ! -f "$RESULTS_DIR/summary.json" ]; then
    echo "Error: No test results found. Run a test first with ./tests/k6/run_tests.sh"
    exit 1
fi

# Extract key metrics
RPS=$(jq -r '.metrics.http_reqs.values.rate' "$RESULTS_DIR/summary.json")
AVG_DURATION=$(jq -r '.metrics.http_req_duration.values.avg' "$RESULTS_DIR/summary.json")
P95_DURATION=$(jq -r '.metrics.http_req_duration.values["p(95)"]' "$RESULTS_DIR/summary.json")
ERROR_RATE=$(jq -r '.metrics.http_req_failed.values.rate' "$RESULTS_DIR/summary.json")

# Save baseline
cat > "$BASELINE_FILE" << EOF
{
  "rps": $RPS,
  "avg_duration": $AVG_DURATION,
  "p95_duration": $P95_DURATION,
  "error_rate": $ERROR_RATE,
  "timestamp": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
}
EOF

echo "Baseline set successfully:"
cat "$BASELINE_FILE"
