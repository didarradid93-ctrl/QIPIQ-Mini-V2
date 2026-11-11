import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import test from 'node:test';

const root = new URL('..', import.meta.url);
const read = (path) => readFileSync(new URL(path, root), 'utf8');

test('backend serves the browser demo from the correct directory', () => {
  const source = read('backend/app.py');
  assert.match(source, /web-demo[\\/].*index\.html/);
  assert.doesNotMatch(source, /web[\\/].*index\.html/);
});

test('web demo clearly labels the mock AI simulation', () => {
  const source = read('web-demo/index.html');
  assert.match(source, /MOCK AI/i);
  assert.match(source, /browser.*simulation|simulation.*browser/i);
});

test('github actions runs the smoke tests and demo safety checks', () => {
  const workflow = read('.github/workflows/prototype-validation.yml');
  assert.match(workflow, /node --test tests\/\*\.test\.mjs/);
  assert.match(workflow, /grep -qi 'MOCK AI'/);
});
