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
