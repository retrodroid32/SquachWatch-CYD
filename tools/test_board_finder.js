#!/usr/bin/env node
'use strict';
// Exercise the actual browser finder function, extracted from index.html,
// against the board profiles that will be deployed. No mock algorithm.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const root = path.resolve(__dirname, '..');
const html = fs.readFileSync(path.join(root, 'web-flasher/index.html'), 'utf8');
const boards = JSON.parse(fs.readFileSync(path.join(root, 'web-flasher/boards.json'), 'utf8')).boards;
const matrix = Object.fromEntries(boards.map(b => [b.id, b]));
const start = html.indexOf('    function classifyBoardCode(input, matrix) {');
const end = html.indexOf('\n    loadBoardMatrix().then(function () {', start);
assert(start >= 0 && end > start, 'inline board finder must be present in index.html');
const fn = vm.runInNewContext(html.slice(start, end) + '\nclassifyBoardCode;');

const cases = [
  ['ESP32-2432S028R', 'match', 'cyd'],
  ['JC2432W328C', 'match', 'cyd'],
  ['E32R28T', 'match', 'cyd-ili9341'],
  ['ESP32-32E 2.8 inch', 'match', 'cyd-ili9341'],
  ['ESP32-2432S032R', 'match', 'cyd32-st7798'],
  ['E32R32P', 'match', 'cyd32-st7798'],
  ['FNK0103L', 'match', 'freenove32'],
  ['ESP32-2432S032C', 'unpublished', null],
  ['ESP32-32E', 'ambiguous', null],
  ['E32R35T', 'unsupported35', null],
  ['ESP32-32E 3.5 inch', 'unsupported35', null],
  ['UNRECOGNIZED', 'unknown', null],
  ['', 'empty', null],
];
for (const [input, kind, id] of cases) {
  const result = fn(input, matrix);
  assert.equal(result.kind, kind, input);
  if (id) assert.equal(result.id, id, input);
  else assert.equal(result.id, undefined, input);
}
assert.equal(matrix.cyd32c.visible, false, 'unverified CYD32C must stay hidden');
assert.equal(matrix.cyd32c.release, false, 'unverified CYD32C cannot be released');
assert.equal(matrix.cyd32c.sign, false, 'unverified CYD32C cannot enter production signing');
console.log(`Board finder: ${cases.length} match/safety cases PASS; CYD32C unpublished`);
