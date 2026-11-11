import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import test from 'node:test';

const root = new URL('..', import.meta.url);
const read = (path) => readFileSync(new URL(path, root), 'utf8');

test('backend serves the browser demo from the correct directory', () => {
  const source = read('backend/app.py');
  assert.match(source, /web-demo[\\/].*index\.html/);
  assert.doesNotMatch(source, /APP_DIR\s*\/\s*["']web["']\s*\/\s*["']index\.html["']/);
});

test('web demo clearly labels the mock AI simulation', () => {
  const source = read('web-demo/index.html');
  assert.match(source, /MOCK AI/i);
  assert.match(source, /браузерлік демо/i);
  assert.match(source, /микрофон.*ESP32.*live AI/i);
});

test('current CI retains its Python syntax and demo safety checks', () => {
  const workflow = read('.github/workflows/prototype-validation.yml');
  assert.match(workflow, /python -m compileall -q backend/);
  assert.match(workflow, /test -s web-demo\/index\.html/);
  assert.match(workflow, /test ! -f backend\/\.env/);
});
