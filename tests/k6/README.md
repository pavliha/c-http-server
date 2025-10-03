# Performance Testing with k6

This directory contains professional load testing setup using [k6](https://k6.io/).

## Installation

### macOS
```bash
brew install k6
```

### Linux
```bash
sudo gpg -k
sudo gpg --no-default-keyring --keyring /usr/share/keyrings/k6-archive-keyring.gpg --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys C5AD17C747E3415A3642D57D77C6C491D6AC1D69
echo "deb [signed-by=/usr/share/keyrings/k6-archive-keyring.gpg] https://dl.k6.io/deb stable main" | sudo tee /etc/apt/sources.list.d/k6.list
sudo apt-get update
sudo apt-get install k6
```

### Windows
```powershell
choco install k6
```

## Test Types

### 1. Smoke Test (`smoke_test.js`)
**Purpose**: Quick sanity check that server works correctly

**Duration**: 10 seconds
**Load**: 1 virtual user
**When to run**: After every build, in CI/CD

```bash
./tests/k6/run_tests.sh smoke
```

### 2. Load Test (`load_test.js`)
**Purpose**: Verify server performs well under expected load

**Duration**: 3.5 minutes
**Load**: Ramps from 10 → 50 users
**When to run**: Before releases, nightly

```bash
./tests/k6/run_tests.sh load
```

### 3. Stress Test (`stress_test.js`)
**Purpose**: Find server breaking point

**Duration**: 23 minutes
**Load**: Ramps up to 300 users
**When to run**: Before major releases, monthly

```bash
./tests/k6/run_tests.sh stress
```

## Metrics Tracking

Test results are automatically saved to `results/metrics.jsonl` as a time-series database:

```json
{"timestamp":"2025-10-03T18:00:00Z","test_type":"smoke","rps":20000,"avg_duration":2.5,"p95_duration":5.2,"error_rate":0}
{"timestamp":"2025-10-04T02:00:00Z","test_type":"load","rps":19500,"avg_duration":3.1,"p95_duration":8.4,"error_rate":0.001}
```

## Baseline & Regression Detection

### Set Baseline
After running a test, set it as the performance baseline:

```bash
./tests/k6/set_baseline.sh
```

This saves current metrics to `results/baseline.json`.

### Automatic Regression Detection
The test runner automatically compares against baseline:
- **>10% degradation**: Test fails (CI fails)
- **5-10% degradation**: Warning issued
- **<5% degradation**: Test passes

## CI/CD Integration

Performance tests run automatically:
- **Nightly**: Every day at 2 AM UTC (scheduled)
- **Manual**: Via GitHub Actions workflow_dispatch
- **Not on push/PR**: Too slow for quick feedback

To trigger manually:
1. Go to GitHub Actions
2. Select "CI" workflow
3. Click "Run workflow"

## Example Output

```
========================================
Test Summary
============

Requests:      12450
Duration:      10234ms
Req/sec:       21082.37

Response Times (ms):
  min:         0.52
  avg:         2.84
  p(95):       5.23
  max:         42.11

Error Rate:    0.00%

Thresholds:    http_req_duration: PASS, http_req_failed: PASS
========================================
```

## Performance Targets

Current baselines (single-threaded server):

| Metric | Target | Warning Threshold |
|--------|--------|-------------------|
| Requests/sec | >10,000 | <9,000 |
| P95 Latency | <100ms | >100ms |
| Error Rate | 0% | >1% |

## Troubleshooting

**Server not starting**: Make sure you've built the project first:
```bash
cmake --preset dev && cmake --build --preset dev
```

**k6 not found**: Install k6 using instructions above

**Port already in use**: Kill any running servers:
```bash
pkill -9 c-http-server
```

## Advanced Usage

### Custom test duration
Edit the test files in `tests/k6/*.js` to adjust `stages` array.

### Test different endpoints
Modify the test scripts to hit your custom endpoints.

### Export to cloud
k6 supports exporting metrics to various backends:
- InfluxDB
- Grafana Cloud
- Datadog
- New Relic

See: https://k6.io/docs/results-output/
