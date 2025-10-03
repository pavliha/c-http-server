import http from 'k6/http';
import { check, sleep } from 'k6';
import { Rate, Trend } from 'k6/metrics';

// Custom metrics
const errorRate = new Rate('errors');
const responseTrend = new Trend('response_time');

// Test configuration
export const options = {
  stages: [
    { duration: '30s', target: 10 },   // Ramp up to 10 users
    { duration: '1m', target: 10 },    // Stay at 10 users
    { duration: '30s', target: 50 },   // Ramp up to 50 users
    { duration: '1m', target: 50 },    // Stay at 50 users
    { duration: '30s', target: 0 },    // Ramp down to 0
  ],
  thresholds: {
    http_req_duration: ['p(95)<100'],  // 95% of requests must complete below 100ms
    http_req_failed: ['rate<0.01'],    // Error rate must be below 1%
    errors: ['rate<0.01'],
  },
};

const BASE_URL = __ENV.BASE_URL || 'http://localhost:8080';

export default function () {
  // Test 1: GET homepage
  const homeRes = http.get(`${BASE_URL}/`);

  check(homeRes, {
    'homepage status is 200': (r) => r.status === 200,
    'homepage has HTML': (r) => r.body.includes('<!DOCTYPE html>'),
  }) || errorRate.add(1);

  responseTrend.add(homeRes.timings.duration);

  sleep(0.5);

  // Test 2: POST to login (expect failure without valid user)
  const loginRes = http.post(
    `${BASE_URL}/login`,
    'username=testuser&password=testpass123',
    {
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    }
  );

  check(loginRes, {
    'login responds': (r) => r.status === 400 || r.status === 401 || r.status === 429,
    'login is JSON': (r) => r.headers['Content-Type'].includes('application/json'),
  }) || errorRate.add(1);

  sleep(1);
}

export function handleSummary(data) {
  return {
    'results/summary.json': JSON.stringify(data),
    stdout: textSummary(data, { indent: ' ', enableColors: true }),
  };
}

function textSummary(data, options) {
  const indent = options.indent || '';
  const enableColors = options.enableColors || false;

  return `
${indent}Test Summary
${indent}============
${indent}
${indent}Requests:      ${data.metrics.http_reqs.values.count}
${indent}Duration:      ${data.state.testRunDurationMs}ms
${indent}Req/sec:       ${(data.metrics.http_reqs.values.rate).toFixed(2)}
${indent}
${indent}Response Times (ms):
${indent}  min:         ${data.metrics.http_req_duration.values.min.toFixed(2)}
${indent}  avg:         ${data.metrics.http_req_duration.values.avg.toFixed(2)}
${indent}  p(95):       ${data.metrics.http_req_duration.values['p(95)'].toFixed(2)}
${indent}  max:         ${data.metrics.http_req_duration.values.max.toFixed(2)}
${indent}
${indent}Error Rate:    ${(data.metrics.http_req_failed.values.rate * 100).toFixed(2)}%
${indent}
${indent}Thresholds:    ${Object.keys(data.metrics).filter(m =>
    data.metrics[m].thresholds
  ).map(m => {
    const metric = data.metrics[m];
    const passed = Object.keys(metric.thresholds).every(t => !metric.thresholds[t].ok === false);
    return `${m}: ${passed ? 'PASS' : 'FAIL'}`;
  }).join(', ')}
`;
}
