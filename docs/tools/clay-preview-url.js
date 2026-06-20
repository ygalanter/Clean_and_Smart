#!/usr/bin/env node
'use strict';

/**
 * Build a Clay settings preview from src/pkjs/config.json.
 *
 * Run from repo root after `rebble build` (needs build/js/message_keys.json).
 * Writes build/clay-preview.html — open that file in Chrome for a visual check.
 */

const fs = require('fs');
const path = require('path');
const Module = require('module');

const repoRoot = path.resolve(__dirname, '../..');
const keysPath = path.join(repoRoot, 'build/js/message_keys.json');
const configPath = path.join(repoRoot, 'src/pkjs/config.json');
const outPath = path.join(repoRoot, 'build/clay-preview.html');

if (!fs.existsSync(keysPath)) {
  console.error('Missing ' + keysPath + ' — run `rebble build` first.');
  process.exit(1);
}

const origRequire = Module.prototype.require;
Module.prototype.require = function (id) {
  if (id === 'message_keys') {
    return require(keysPath);
  }
  return origRequire.apply(this, arguments);
};

global.Pebble = { platform: 'pypkjs', addEventListener: function () {} };
global.localStorage = {
  getItem: function () { return null; },
  setItem: function () {}
};

const Clay = require('@rebble/clay/dist/js/index.js');
const clayConfig = require(configPath);
const clay = new Clay(clayConfig, null, { autoHandleEvents: false });
const url = clay.generateUrl();

const prefix = 'http://clay.pebble.com.s3-website-us-west-2.amazonaws.com/#';
let html;
if (url.startsWith(prefix)) {
  html = decodeURIComponent(url.slice(prefix.length));
} else if (url.startsWith('data:text/html;charset=utf-8,')) {
  html = decodeURIComponent(url.slice('data:text/html;charset=utf-8,'.length));
} else {
  console.error('Unexpected Clay URL format; writing URL only to build/clay-preview.url');
  fs.writeFileSync(path.join(repoRoot, 'build/clay-preview.url'), url, 'utf8');
  process.exit(0);
}

fs.mkdirSync(path.dirname(outPath), { recursive: true });
fs.writeFileSync(outPath, html, 'utf8');
console.log(outPath);
