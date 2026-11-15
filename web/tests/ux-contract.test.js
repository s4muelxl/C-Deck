'use strict';

const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');

const webDir = path.resolve(__dirname, '..');
const html = fs.readFileSync(path.join(webDir, 'index.html'), 'utf8');
const css = fs.readFileSync(path.join(webDir, 'style.css'), 'utf8');
const script = fs.readFileSync(path.join(webDir, 'main.js'), 'utf8');

test('the simulator clearly discloses that it is local and illustrative', () => {
  assert.match(html, /aria-describedby="demo-disclaimer"/);
  assert.match(html, /Demonstração local e ilustrativa/);
  assert.match(html, /grade 3×3 é reduzida/);
  assert.match(html, /PRONTO · MODO DEMONSTRAÇÃO/);
  assert.doesNotMatch(html, /C\+\+ WIN32 NATIVO/);
});

test('demo copy does not claim to control Windows or launch real apps', () => {
  assert.match(script, /Simulação:/);
  assert.match(script, /Demonstração local/);
  assert.doesNotMatch(script, /Comando enviado:|Abrindo navegador padrão:|Iniciando aplicativo:|C\+\+ WIN32 NATIVO/);
});

test('interactive toggle and range controls expose accessible state', () => {
  assert.match(html, /id="key-mute"[^>]*aria-pressed="false"/);
  assert.match(html, /id="key-play"[^>]*aria-pressed="false"/);
  assert.match(html, /id="demo-sound-toggle"[^>]*aria-pressed="true"/);
  assert.match(html, /aria-valuetext="72%"/);
  assert.match(script, /aria-valuetext/);
});

test('keyboard focus, reduced motion and compact layouts are defined', () => {
  assert.match(css, /:focus-visible/);
  assert.match(css, /prefers-reduced-motion:\s*reduce/);
  assert.match(css, /max-width:\s*420px/);
});

test('the page allows browser zoom for low-vision users', () => {
  assert.doesNotMatch(html, /user-scalable=no|maximum-scale=1(?:\.0)?/);
});


test('the volume slider has a visible thumb and a keyboard focus treatment', () => {
  assert.match(css, /\.volume-slider::-webkit-slider-thumb/);
  assert.match(css, /\.volume-slider::-moz-range-thumb/);
  assert.match(css, /\.volume-slider:focus-visible/);
  assert.doesNotMatch(css, /\.volume-slider\s*\{[^}]*opacity:\s*0/s);
});

test('FAQ answers are linked to controls and hidden when collapsed', () => {
  assert.match(html, /aria-controls="faq-answer-1"/);
  assert.match(html, /id="faq-answer-1"[^>]*aria-labelledby="faq-question-1" hidden/);
  assert.match(script, /answer\.hidden = isOpen/);
  assert.match(script, /otherAnswer\.hidden = true/);
});

test('the page provides a skip link and a neutral interaction palette', () => {
  assert.match(html, /class="skip-link" href="#main-content"/);
  assert.match(html, /<main class="main-container" id="main-content">/);
  assert.match(css, /--accent: #eef1f5/);
  assert.match(css, /\.window-status \{ color: #dbe1e8; \}/);
});


test('landing page copy matches the verified C11 build and avoids unsupported performance promises', () => {
  assert.match(html, /desenvolvido em C11/);
  assert.match(html, /Win32 API/);
  assert.match(html, /até três atalhos/);
  assert.doesNotMatch(html, /C\\+\\+|3 MB RAM|0% de CPU|consumo quase zero de memória|mesmo enquanto joga em tela cheia/);
  assert.match(html, /O consumo de memória e CPU varia conforme o sistema e o uso/);
});

test('neutral iconography and high-contrast keyboard states are available', () => {
  assert.match(css, /--cyan: #b0b8c4/);
  assert.match(css, /:where\(a, button\):focus-visible/);
  assert.match(css, /prefers-contrast: more/);
  assert.match(css, /forced-colors: active/);
});


test('the neutral accent is the single source of truth without legacy lime remnants', () => {
  assert.match(css, /--accent:\s*#eef1f5/);
  assert.doesNotMatch(css, /#d7ff(?:63|85)|215,\s*255,\s*99|#e4ff91/i);
});

test('compact layouts keep the volume target usable and key labels legible', () => {
  assert.match(css, /\.volume-interactive-wrap\s*\{[^}]*height:\s*28px/s);
  assert.match(css, /\.volume-slider\s*\{[^}]*height:\s*28px/s);
  assert.match(css, /@media\s*\(max-width:\s*360px\)/);
  assert.match(css, /\.deck-key \.key-badge\s*\{\s*display:\s*none;/);
});

test('the simulator is exposed as a named accessibility region', () => {
  assert.match(html, /class="product-stage" role="region" aria-label="Simulador demonstrativo do C-Deck"/);
});


test('reset control restores simulator state while preserving the sound preference', () => {
  const vm = require('node:vm');

  class FakeElement {
    constructor(id) {
      this.id = id;
      this.listeners = {};
      this.attributes = {};
      this.style = {};
      this.textContent = '';
      this.innerHTML = '';
      this.value = '';
      const values = new Set();
      this.classList = {
        add: (...names) => names.forEach((name) => values.add(name)),
        remove: (...names) => names.forEach((name) => values.delete(name)),
        contains: (name) => values.has(name),
        toggle: (name, force) => {
          const enabled = force === undefined ? !values.has(name) : Boolean(force);
          if (enabled) values.add(name);
          else values.delete(name);
          return enabled;
        }
      };
    }
    addEventListener(name, callback) { this.listeners[name] = callback; }
    setAttribute(name, value) { this.attributes[name] = value; }
    dispatch(name, event = {}) { if (this.listeners[name]) this.listeners[name](event); }
  }

  const ids = [
    'demo-volume-value','demo-volume-track','demo-volume-slider','demo-status-text',
    'key-mute','key-play','play-glyph','play-label','demo-sound-toggle','demo-reset',
    'key-vol-down','key-vol-up','key-prev','key-next','key-app-1','key-app-2','key-app-3'
  ];
  const elements = Object.fromEntries(ids.map((id) => [id, new FakeElement(id)]));
  const document = {
    readyState: 'complete',
    activeElement: null,
    getElementById: (id) => elements[id] || null,
    querySelectorAll: () => [],
    querySelector: () => null
  };
  let timerId = 0;
  vm.runInNewContext(script, {
    document,
    window: {},
    setTimeout: () => ++timerId,
    clearTimeout: () => {},
    parseInt,
    Math
  });

  elements['key-vol-up'].dispatch('click');
  elements['key-mute'].dispatch('click');
  elements['key-play'].dispatch('click');
  elements['demo-sound-toggle'].dispatch('click');
  assert.equal(elements['demo-volume-slider'].value, 77);
  assert.equal(elements['key-mute'].attributes['aria-pressed'], 'true');
  assert.equal(elements['key-mute'].attributes['aria-label'], 'Ativar áudio (simulação)');
  assert.equal(elements['key-play'].attributes['aria-pressed'], 'true');
  assert.equal(elements['key-play'].attributes['aria-label'], 'Pausar reprodução simulada');
  assert.equal(elements['demo-sound-toggle'].textContent, 'SOM: OFF');

  elements['demo-reset'].dispatch('click');

  assert.equal(elements['demo-volume-value'].textContent, '72%');
  assert.equal(elements['demo-volume-track'].style.width, '72%');
  assert.equal(elements['demo-volume-slider'].value, 72);
  assert.equal(elements['demo-volume-slider'].attributes['aria-valuetext'], '72%');
  assert.equal(elements['key-mute'].attributes['aria-pressed'], 'false');
  assert.equal(elements['key-mute'].attributes['aria-label'], 'Silenciar áudio (simulação)');
  assert.equal(elements['key-mute'].classList.contains('key-active-mute'), false);
  assert.equal(elements['key-play'].attributes['aria-pressed'], 'false');
  assert.equal(elements['key-play'].attributes['aria-label'], 'Iniciar reprodução simulada');
  assert.equal(elements['key-play'].classList.contains('key-active-media'), false);
  assert.match(elements['play-glyph'].innerHTML, /M8 5v14/);
  assert.equal(elements['play-label'].textContent, 'PLAY / PAUSE');
  assert.equal(elements['demo-sound-toggle'].textContent, 'SOM: OFF');
  assert.match(elements['demo-status-text'].textContent, /Demonstração reiniciada/);
});

test('the reset control has an accessible name and a compact icon-only layout', () => {
  assert.match(html, /id="demo-reset"[^>]*aria-label="Reiniciar demonstração"/);
  assert.match(html, /title="Restaurar os valores iniciais da demonstração"/);
  assert.match(css, /\.demo-reset-btn:active/);
  assert.match(css, /@media\s*\(max-width:\s*360px\)[\s\S]*?\.demo-reset-btn span\s*\{\s*display:\s*none;/);
  assert.match(script, /demoResetBtn\.addEventListener\('click', resetDemo\)/);
});


test('rapid demo launches replace the toast timer and reset dismisses stale feedback', () => {
  const vm = require('node:vm');

  class FakeElement {
    constructor(id = '') {
      this.id = id;
      this.listeners = {};
      this.attributes = {};
      this.style = {};
      this.textContent = '';
      this.innerHTML = '';
      const values = new Set();
      this.classList = {
        add: (...names) => names.forEach((name) => values.add(name)),
        remove: (...names) => names.forEach((name) => values.delete(name)),
        contains: (name) => values.has(name),
        toggle: (name, force) => {
          const enabled = force === undefined ? !values.has(name) : Boolean(force);
          if (enabled) values.add(name);
          else values.delete(name);
          return enabled;
        }
      };
    }
    addEventListener(name, callback) { this.listeners[name] = callback; }
    setAttribute(name, value) { this.attributes[name] = value; }
    appendChild(child) { elements[child.id] = child; }
    dispatch(name, event = {}) { if (this.listeners[name]) this.listeners[name](event); }
  }

  const ids = [
    'demo-volume-value','demo-volume-track','demo-volume-slider','demo-status-text',
    'key-mute','key-play','play-glyph','play-label','demo-sound-toggle','demo-reset',
    'key-vol-down','key-vol-up','key-prev','key-next','key-app-1','key-app-2','key-app-3'
  ];
  const elements = Object.fromEntries(ids.map((id) => [id, new FakeElement(id)]));
  const appWindow = new FakeElement('app-window');
  const timers = new Map();
  const clearedTimers = new Set();
  let nextTimerId = 0;
  const document = {
    readyState: 'complete',
    activeElement: null,
    getElementById: (id) => elements[id] || null,
    createElement: () => new FakeElement(),
    querySelectorAll: () => [],
    querySelector: (selector) => selector === '.app-window' ? appWindow : null
  };

  vm.runInNewContext(script, {
    document,
    window: {},
    setTimeout: (callback) => {
      const id = ++nextTimerId;
      timers.set(id, callback);
      return id;
    },
    clearTimeout: (id) => {
      clearedTimers.add(id);
      timers.delete(id);
    },
    parseInt,
    Math
  });

  elements['key-app-1'].dispatch('click');
  const toast = elements['demo-toast'];
  assert.ok(toast);
  assert.equal(toast.textContent, 'Demonstração local · Bloco de Notas');
  assert.equal(toast.classList.contains('visible'), true);

  elements['key-app-2'].dispatch('click');
  assert.equal(toast.textContent, 'Demonstração local · YouTube');
  assert.equal(toast.classList.contains('visible'), true);
  assert.equal(clearedTimers.size, 2, 'the previous status and toast timers are replaced');

  elements['demo-reset'].dispatch('click');
  assert.equal(toast.classList.contains('visible'), false);
  assert.equal(clearedTimers.size, 4, 'reset clears the current status and toast timers');
});


test('simulator controls expose explicit names and a semantic status region', () => {
  assert.match(html, /id="key-vol-down"[^>]*aria-label="Reduzir volume em 5% \(simulação\)"/);
  assert.match(html, /id="key-mute"[^>]*aria-label="Silenciar áudio \(simulação\)"/);
  assert.match(html, /id="key-play"[^>]*aria-label="Iniciar reprodução simulada"/);
  assert.match(html, /id="demo-status-text"[^>]*role="status"[^>]*aria-live="polite"[^>]*aria-atomic="true"/);
  assert.match(html, /class="key-badge" aria-hidden="true"/);
  assert.match(html, /class="key-glyph key-icon glyph-app" aria-hidden="true"/);
  assert.match(html, /class="faq-icon" aria-hidden="true"/);
});

test('shortcut cards are explicitly described as local simulations', () => {
  assert.match(html, /title="Simular abertura do Bloco de Notas"/);
  assert.match(html, /title="Simular abertura do YouTube"/);
  assert.match(html, /title="Simular abertura da Calculadora"/);
  assert.match(script, /Demonstração local/);
});

test('touch devices do not retain hover-only elevation after tapping', () => {
  assert.match(css, /@media \(hover: none\)[\s\S]*?\.button-primary:hover[\s\S]*?transform: none/);
  assert.match(css, /@media \(hover: none\)[\s\S]*?\.deck-key:hover[\s\S]*?box-shadow: none/);
  assert.match(css, /\.essential-item:hover\s*\{[^}]*transform: none/s);
  assert.match(css, /\.faq-question:hover\s*\{[^}]*color: #e8ecf1/s);
});


test('simulator titlebar controls meet the 44px touch target contract', () => {
  const touchRules = css.slice(css.lastIndexOf('/* Touch accessibility:'));
  assert.match(touchRules, /\.window-titlebar\s*\{[^}]*height:\s*52px/s);
  assert.match(touchRules, /\.sound-toggle-btn\s*\{[^}]*min-width:\s*44px[^}]*min-height:\s*44px/s);
  assert.match(touchRules, /\.demo-reset-btn\s*\{[^}]*min-width:\s*44px[^}]*min-height:\s*44px/s);
  assert.match(touchRules, /@media\s*\(max-width:\s*360px\)[\s\S]*?\.demo-reset-btn\s*\{[^}]*width:\s*44px/);
});
