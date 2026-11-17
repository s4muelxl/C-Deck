'use strict';

const assert = require('node:assert/strict');
const { chromium } = require('playwright');

(async () => {
  const browser = await chromium.launch({ headless: true });
  const page = await browser.newPage({ viewport: { width: 1440, height: 1000 }, deviceScaleFactor: 1 });
  const pageErrors = [];
  const failedRequests = [];
  page.on('pageerror', (error) => pageErrors.push(error.message));
  page.on('requestfailed', (request) => {
    if (new URL(request.url()).origin === 'http://127.0.0.1:4173') {
      failedRequests.push(request.url());
    }
  });

  try {
    const response = await page.goto('http://127.0.0.1:4173/', { waitUntil: 'networkidle' });
    assert.equal(response.status(), 200, 'landing page must respond with HTTP 200');
    assert.match(await page.title(), /C-Deck/);
    assert.equal(await page.locator('.deck-key').count(), 9, 'the reduced simulator must expose nine keys');

    const canonical = await page.locator('link[rel="canonical"]').getAttribute('href');
    assert.equal(canonical, 'https://c-deck.vercel.app/');
    const jsonLd = await page.locator('script[type="application/ld+json"]').textContent();
    const software = JSON.parse(jsonLd);
    assert.equal(software.softwareVersion, '1.4.0');
    assert.equal(software.operatingSystem, 'Windows 10, Windows 11');
    assert.match(await page.locator('.hero .description').textContent(), /até 15 teclas/);
    assert.match(await page.locator('.feature-grid').textContent(), /quatro perfis independentes/i);

    const anchors = await page.locator('a[href^="#"]').evaluateAll((items) =>
      items.map((item) => item.getAttribute('href').slice(1))
    );
    const missingAnchors = await page.evaluate((targets) =>
      targets.filter((target) => !document.getElementById(target)), anchors);
    assert.deepEqual(missingAnchors, [], 'every in-page link must resolve to a section');

    for (const width of [1440, 1024, 821, 820, 768, 650, 420, 390, 360, 320]) {
      await page.setViewportSize({ width, height: 900 });
      const dimensions = await page.evaluate(() => ({
        viewport: window.innerWidth,
        document: document.documentElement.scrollWidth,
        body: document.body.scrollWidth
      }));
      assert.ok(
        Math.max(dimensions.document, dimensions.body) <= dimensions.viewport,
        'horizontal overflow at ' + width + 'px: ' + JSON.stringify(dimensions)
      );
    }

    await page.setViewportSize({ width: 1440, height: 1000 });
    const desktopHeading = await page.locator('.hero h1').evaluate((element) => ({
      height: element.getBoundingClientRect().height,
      fontSize: Number.parseFloat(getComputedStyle(element).fontSize)
    }));
    assert.ok(desktopHeading.height <= desktopHeading.fontSize * 2.3,
      'desktop hero headline should remain in two intentional lines');
    await page.screenshot({ path: 'artifacts/c-deck-hero-1440.png' });
    await page.screenshot({ path: 'artifacts/c-deck-desktop-1440.png', fullPage: true });

    await page.setViewportSize({ width: 390, height: 844 });
    await page.screenshot({ path: 'artifacts/c-deck-mobile-390.png', fullPage: true });

    await page.setViewportSize({ width: 320, height: 780 });
    const compactHeading = await page.locator('.hero h1').evaluate((element) => ({
      height: element.getBoundingClientRect().height,
      fontSize: Number.parseFloat(getComputedStyle(element).fontSize)
    }));
    assert.ok(compactHeading.height <= compactHeading.fontSize * 2.3,
      '320px hero headline should remain in two intentional lines');
    await page.screenshot({ path: 'artifacts/c-deck-mobile-320.png', fullPage: true });

    const menu = page.locator('.mobile-nav');
    const trigger = page.locator('.mobile-nav summary');
    assert.equal(await trigger.isVisible(), true, 'mobile menu trigger must be visible');
    await trigger.click();
    assert.equal(await menu.evaluate((element) => element.open), true, 'menu opens with native details');
    await page.keyboard.press('Escape');
    assert.equal(await menu.evaluate((element) => element.open), false, 'Escape closes the menu');
    assert.equal(await trigger.evaluate((element) => element === document.activeElement), true,
      'Escape returns focus to the menu trigger');

    await trigger.click();
    await page.locator('.mobile-nav a[href="#recursos"]').click();
    assert.equal(await menu.evaluate((element) => element.open), false,
      'selecting an in-page destination closes the menu');

    const robots = await page.request.get('http://127.0.0.1:4173/robots.txt');
    const sitemap = await page.request.get('http://127.0.0.1:4173/sitemap.xml');
    assert.equal(robots.status(), 200);
    assert.equal(sitemap.status(), 200);
    assert.match(await robots.text(), /Sitemap:/);
    assert.match(await sitemap.text(), /https:\/\/c-deck\.vercel\.app\//);

    assert.deepEqual(failedRequests, [], 'local page assets must load without request failures');
    assert.deepEqual(pageErrors, [], 'the page must not emit uncaught JavaScript errors');
    process.stdout.write('Visual checks passed: desktop, tablet, mobile (320–1440px), navigation, SEO and local assets.\n');
  } finally {
    await browser.close();
  }
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
