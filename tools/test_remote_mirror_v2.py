#!/usr/bin/env python3
"""Guard USB mirror provenance, incremental framing, PIN and LOG isolation.

This is source-level integration coverage, not a claim of physical USB or
LCD hardware validation. Both physical classic CYDs remain required.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding="utf-8")
mirror = read("src/remote_mirror.cpp")
head = read("include/remote_mirror.h")
main = read("src/main.cpp")
clock = read("src/clock.cpp")
sim = read("sim/Makefile")
doc = read("docs/USB_MIRROR_WORKBENCH_V2.md")
ci = read(".github/workflows/build.yml")

for token in (
    'MIRROR CAPS 2 RGB332 FULL DELTA ROWS TOUCH PAUSE NO_FAKE_USB',
    'MIRROR READY RGB332 320x240/240x320 TOUCH DELTA V2',
    'MIRROR BEGIN %lu %d %d RGB332',
    'MIRROR DELTA %lu %lu %d %d RGB332',
    'MIRROR ROW %lu %d ',
    'MIRROR END %lu',
    'MIRROR ERROR %s',
    'MIRROR PAUSE', 'MIRROR RESUME', 'MIRROR OFF',
    'MIRROR TOUCH %d %d %7s',
    'MIRROR FRAME', 'MIRROR FRAME ',
    's.baseRequested==s.committed',
    'memcpy(baselineHash,pendingHash,',
    's.committed=s.sequence; s.baselineValid=true;',
    'Security::locked()', 'void tick()',
    'NO_FRAME_REFRESH_OR_TX',
    'if(s.nextRow==s.height)',
    'if(s.nextRow<y || s.nextRow>=y+bandH) return;',
):
    assert token in mirror, token

assert 'constexpr int MAX_SENT_PER_PASS = 4;' in mirror
assert 'constexpr int MAX_SCANNED_PER_PASS = 32;' in mirror
assert 'Serial.availableForWrite()<n+32' in mirror
assert 'Serial.write((const uint8_t*)p,(size_t)n)' in mirror
assert 'uint32_t baselineHash[MAX_H]' in mirror
assert 'uint32_t pendingHash[MAX_H]' in mirror
assert 'TFT_eSPI::read' not in mirror
assert 'readRect(' not in mirror
assert 'malloc(' not in mirror and 'new uint8_t' not in mirror
assert 'MIRROR ERROR' not in head or 'void unavailable()' in head

# Nothing sneaks past the existing serial PIN gate.
assert clock.index('if (Security::locked()) {') < clock.index('RemoteMirror::command(line)')
assert 'RemoteMirror::pauseEvidence();\n            logDump();' in clock

# Same native touch hit test (no second touch-mapping implementation).
touch = main[main.index('static TouchPoint pollTouch()'):main.index('// ---- Raw readers ----')]
assert touch.index('if (!readTouchRaw(a, b))') < touch.index('RemoteMirror::touch(')
assert 'TouchFit::toScreen(s_touchFit' in touch

# Capture follows the actual LCD push; idle full-frame snapshots are bounded.
push = main[main.index('static inline void pushFrame('):main.index('// Draw a whole screen', main.index('static inline void pushFrame('))]
assert push.index('frame.pushSprite(x, y)') < push.index('RemoteMirror::capture(')
assert 'frame.bufW(),frame.bufH(),tft.height(),y' in push
assert 's_recoveryBands==1 && canvas==&frame' in main
assert 'if(!frameBufferOk) RemoteMirror::unavailable();' in main
assert 'Serial.setTxBufferSize(2048);' in main
assert '$(SRC)/remote_mirror.cpp' in sim

# Keep the two independent device protocols, never silently change SquachEmit.
assert 'SquachEmit PR #37 remain unchanged' in doc
assert 'MIRROR FRAME 12' in doc
assert 'MIRROR PAUSE' in doc and 'LOG' in doc
assert 'python3 tools/test_remote_mirror_v2.py' in ci
print("PASS: SquachWatch USB authentic row-delta mirror, native touch, LOG/PIN gates")
