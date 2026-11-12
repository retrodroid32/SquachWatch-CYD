#!/usr/bin/env python3
"""v1.41 classic CYD web workbench: static source/deployment/JS invariants."""
from pathlib import Path
import importlib.util
import re
import shutil
import subprocess
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
PAGE = ROOT / 'web-flasher/emulator/index.html'
LOCAL = ROOT / 'sim/web/index.html'
HTML = PAGE.read_text(encoding='utf-8')

assert HTML == LOCAL.read_text(encoding='utf-8'), 'sim and deployed HTML differ'
assert HTML.count('<!-- BOARDS:BEGIN -->') == 1
assert HTML.count('<!-- BOARDS:END -->') == 1
assert HTML.count('<template id="board-cyd28">') == 1
assert HTML.count('<template id="board-') == 1, 'unsupported boards enabled'
assert 'const BUILDS = {}; // only classic CYD WASM' in HTML
assert not re.search(r'\./squachsim-(?:tdeck|twatch|cardputer)\.js', HTML)
assert "if (board.pocket === 'boot' && E.boot)" in HTML
assert "if (board.pocket === 'boot') return E && E.boot" in HTML
assert "if (E.boot) E.boot()" in HTML
assert "if (/^[1-5]$/.test(e.key))" not in HTML, 'missing shortcut board'
assert all(f'data-size="{name}"' in HTML for name in ('fit','same','actual'))
assert 'requestAnimationFrame(tick)' in HTML
assert "cv.addEventListener('pointerdown'" in HTML
assert 'sw_det_catalog' in HTML and 'meshCatalog' in HTML

spec = importlib.util.spec_from_file_location(
    'embed_cyd_board', ROOT / 'sim/web/boards/embed.py')
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
assert mod.BOARDS == ['cyd28']
assert mod.drawing('cyd28') in HTML, 'embedded SVG source drift'
drawing = re.search(r'<template id="board-cyd28">\s*(<svg.*?</svg>)\s*</template>', HTML, re.S)
assert drawing, 'board SVG missing or broken'
svg = ET.fromstring(drawing.group(1))
ids = {e.attrib.get('id') for e in svg.iter()}
assert {'cyd28-screen','cyd28-usb','cyd28-btn-boot'} <= ids
panel = next(e for e in svg.iter() if e.attrib.get('id') == 'cyd28-screen')
assert all(float(panel.attrib[v]) >= 0 for v in ('x','y','width','height'))

module = re.search(r'<script type="module">\s*(.*?)\s*</script>', HTML, re.S)
assert module
assert (ROOT / 'tools/test_v141_wasm_exports.cjs').is_file()
if shutil.which('node'):
    verifier = subprocess.run(['node', '--check',
       str(ROOT / 'tools/test_v141_wasm_exports.cjs')],
       capture_output=True, text=True, check=False)
    assert verifier.returncode == 0, verifier.stderr
ci = (ROOT / '.github/workflows/build.yml').read_text()
assert 'mymindstorm/setup-emsdk@v14' in ci
assert 'version: 6.0.9' in ci
assert 'lab-pr-${{ github.event.pull_request.number }}-v141-fresh-wasm-workbench' in ci
assert 'node tools/test_v141_wasm_exports.cjs' in ci
if shutil.which('node'):
    result = subprocess.run(['node','--check','--input-type=module'],
                            input=module.group(1), text=True,
                            capture_output=True, check=False)
    assert result.returncode == 0, result.stderr
print('PASS v1.41 classic CYD workbench: board source, JS, controls, deployment')
