import http from 'k6/http';
import { check } from 'k6';

// Smoke test - quick sanity check
export const options = {
  vus: 1,
  duration: '10s',
  thresholds: {
    http_req_duration: ['p(99)<1000'], // 99% of requests must complete below 1s
    http_req_failed: ['rate<0.01'],
  },
};

const BASE_URL = __ENV.BASE_URL || 'http://localhost:8080';

export default function () {
  // Test homepage
  const res = http.get(`${BASE_URL}/`);

  check(res, {
    'status is 200': (r) => r.status === 200,
    'has HTML content': (r) => r.body.includes('<!DOCTYPE html>'),
    'response time < 100ms': (r) => r.timings.duration < 100,
  });
}
