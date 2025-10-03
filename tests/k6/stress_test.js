import http from 'k6/http';
import { check, sleep } from 'k6';

// Stress test - push server to its limits
export const options = {
  stages: [
    { duration: '2m', target: 100 },  // Ramp up to 100 users
    { duration: '5m', target: 100 },  // Stay at 100 users
    { duration: '2m', target: 200 },  // Ramp up to 200 users
    { duration: '5m', target: 200 },  // Stay at 200 users
    { duration: '2m', target: 300 },  // Beyond breaking point
    { duration: '5m', target: 300 },
    { duration: '2m', target: 0 },    // Ramp down
  ],
  thresholds: {
    http_req_duration: ['p(95)<500'],  // Looser threshold for stress test
    http_req_failed: ['rate<0.05'],    // Allow up to 5% errors under stress
  },
};

const BASE_URL = __ENV.BASE_URL || 'http://localhost:8080';

export default function () {
  const res = http.get(`${BASE_URL}/`);

  check(res, {
    'status is 200': (r) => r.status === 200,
  });

  sleep(0.1);
}
