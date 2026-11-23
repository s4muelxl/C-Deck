'use strict';

const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');

const root = path.resolve(__dirname, '../..');
const read = (file) => fs.readFileSync(path.join(root, file), 'utf8');
const html = read('web/index.html');
const readme = read('README.md');
const version = read('VERSION').trim();
const installer = read('installer/C-Deck.iss');
const build = read('scripts/build.ps1');

test('version is consistent across the app website, README and installer', () => {
  assert.equal(version, '1.5.0');
  assert.match(html, new RegExp('C-Deck Official Website v' + version.replaceAll('.', '\\.')));
  assert.ok(html.includes('VERSÃO ' + version));
  assert.ok(readme.includes('Versão ' + version + '.'));
  assert.ok(installer.includes('#define AppVersion "' + version + '"'));
});

test('desktop app contains exactly the six agreed actions', () => {
  const ui = read('src/interface.c');
  for (const label of ['Volume +', 'Volume -', 'Mudo', 'Bloco de Notas', 'Calculadora', 'Paint']) {
    assert.ok(ui.includes(label), 'missing button: ' + label);
  }
  assert.equal((ui.match(/add_button\(window, instance,/g) || []).length, 6);
  assert.match(ui, /BUTTON_WIDTH 155/);
  assert.match(ui, /BUTTON_HEIGHT 42/);
  assert.match(ui, /LEFT_COLUMN 18/);
  assert.match(ui, /RIGHT_COLUMN 187/);
});

test('C code is separated into small interface, audio and app modules', () => {
  assert.match(read('src/main.c'), /cdeck_run\(instance, show_command\)/);
  assert.match(read('src/interface.c'), /window_procedure/);
  assert.match(read('src/audio.c'), /IAudioEndpointVolume/);
  assert.match(read('src/apps.c'), /ShellExecuteW/);
  assert.match(read('src/interface.h'), /cdeck_run/);
  assert.match(read('src/audio.h'), /cdeck_change_volume/);
  assert.match(read('src/apps.h'), /cdeck_open_application/);
});

test('local build and CI compile the same four C source files', () => {
  for (const source of ['src/main.c', 'src/interface.c', 'src/audio.c', 'src/apps.c']) {
    assert.ok(build.includes(source), 'local build missing ' + source);
  }
  const ci = read('.github/workflows/ci.yml');
  for (const source of ['src/main.c', 'src/interface.c', 'src/audio.c', 'src/apps.c']) {
    assert.ok(ci.includes(source), 'CI build missing ' + source);
  }
  const release = read('.github/workflows/release.yml');
  for (const source of ['src/main.c', 'src/interface.c', 'src/audio.c', 'src/apps.c']) {
    assert.ok(release.includes(source), 'release build missing ' + source);
  }
});

test('installer ships the single desktop executable and the short README', () => {
  assert.match(installer, /C-Deck\.exe/);
  assert.match(installer, /README\.md/);
  assert.doesNotMatch(installer, /database|plugin|server/i);
});

test('website identifies the six-action native app and the demo as illustrative', () => {
  assert.match(html, /C11/);
  assert.match(html, /Win32 API/);
  assert.match(html, /seis ações/);
  assert.match(html, /A demonstração do site é apenas visual/);
  assert.doesNotMatch(html, /15 Teclas Configuráveis|grade 5×3|configure até 15 teclas/);
});
