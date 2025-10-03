// @ts-check
const { test, expect } = require('@playwright/test');

test.describe('Authentication UI', () => {
  test('should load login page', async ({ page }) => {
    await page.goto('/');
    await expect(page.locator('h1')).toContainText('Login');
    await expect(page.locator('input[type="text"]')).toBeVisible();
    await expect(page.locator('input[type="password"]')).toBeVisible();
  });

  test('should toggle between login and register', async ({ page }) => {
    await page.goto('/');

    // Initial state should be Login
    await expect(page.locator('#formTitle')).toHaveText('Login');
    await expect(page.locator('#submitBtn')).toHaveText('Login');
    await expect(page.locator('#toggleLink')).toHaveText('Register');

    // Click Register link
    await page.locator('#toggleLink').click();

    // Should switch to Register
    await expect(page.locator('#formTitle')).toHaveText('Register');
    await expect(page.locator('#submitBtn')).toHaveText('Register');
    await expect(page.locator('#toggleLink')).toHaveText('Login');

    // Click Login link
    await page.locator('#toggleLink').click();

    // Should switch back to Login
    await expect(page.locator('#formTitle')).toHaveText('Login');
    await expect(page.locator('#submitBtn')).toHaveText('Login');
  });

  test('should register a new user', async ({ page }) => {
    await page.goto('/');

    // Switch to Register
    await page.locator('#toggleLink').click();
    await expect(page.locator('#formTitle')).toHaveText('Register');

    // Fill form
    const timestamp = Date.now();
    await page.locator('#username').fill(`testuser${timestamp}`);
    await page.locator('#password').fill('testpass123');

    // Submit
    await page.locator('#submitBtn').click();

    // Should show success message
    await expect(page.locator('#message')).toBeVisible();
    await expect(page.locator('#message')).toContainText('registered successfully');
  });

  test('should login with valid credentials', async ({ page }) => {
    await page.goto('/');

    // First register a user
    await page.locator('#toggleLink').click();
    const timestamp = Date.now();
    const username = `loginuser${timestamp}`;
    await page.locator('#username').fill(username);
    await page.locator('#password').fill('testpass123');
    await page.locator('#submitBtn').click();
    await page.waitForTimeout(500);

    // Switch to Login
    await page.locator('#toggleLink').click();
    await expect(page.locator('#formTitle')).toHaveText('Login');

    // Login with same credentials
    await page.locator('#username').fill(username);
    await page.locator('#password').fill('testpass123');
    await page.locator('#submitBtn').click();

    // Should redirect to dashboard
    await expect(page).toHaveURL('/dashboard');
    await expect(page.locator('h1')).toContainText('Welcome!');
  });

  test('should reject invalid username format', async ({ page }) => {
    await page.goto('/');

    // Switch to Register
    await page.locator('#toggleLink').click();

    // Try invalid username
    await page.locator('#username').fill('invalid@user');
    await page.locator('#password').fill('testpass123');
    await page.locator('#submitBtn').click();

    // Should show error
    await expect(page.locator('#message')).toBeVisible();
    await expect(page.locator('#message')).toContainText('Invalid username');
  });

  test('should reject empty credentials', async ({ page }) => {
    await page.goto('/');

    // Try to submit empty form
    await page.locator('#submitBtn').click();

    // HTML5 validation should prevent submission
    const username = page.locator('#username');
    await expect(username).toHaveAttribute('required', '');
  });
});
