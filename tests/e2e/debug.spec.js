// @ts-check
const { test, expect } = require('@playwright/test');

test('debug toggle issue', async ({ page }) => {
  const messages = [];
  page.on('console', msg => messages.push(`${msg.type()}: ${msg.text()}`));
  page.on('pageerror', error => messages.push(`ERROR: ${error.message}`));

  await page.goto('/');

  // Check if toggleForm function exists
  const hasToggleForm = await page.evaluate(() => {
    return typeof window.toggleForm === 'function';
  });
  console.log('toggleForm function exists:', hasToggleForm);

  // Check if button exists
  const button = await page.locator('#toggleLink');
  const buttonExists = await button.count();
  console.log('Toggle button count:', buttonExists);

  // Get button attributes
  const onclick = await button.getAttribute('onclick');
  console.log('Button onclick attribute:', onclick);

  // Try clicking
  console.log('Clicking button...');
  await button.click();

  // Wait a bit
  await page.waitForTimeout(500);

  // Check title
  const title = await page.locator('#formTitle').textContent();
  console.log('Form title after click:', title);

  // Print all console messages
  console.log('\nBrowser console messages:');
  messages.forEach(msg => console.log(msg));

  // Try calling function directly
  console.log('\nTrying to call toggleForm() directly...');
  const result = await page.evaluate(() => {
    try {
      if (typeof window.toggleForm === 'function') {
        window.toggleForm();
        return { success: true, title: document.getElementById('formTitle').textContent };
      }
      return { success: false, error: 'Function not found' };
    } catch (e) {
      return { success: false, error: e.message };
    }
  });
  console.log('Direct call result:', result);
});
