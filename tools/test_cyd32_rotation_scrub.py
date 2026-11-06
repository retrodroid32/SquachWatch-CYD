#!/usr/bin/env python3
"""CYD32 rotation-stride scrub regression guards (static, not physical validation)."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
main = (root / "src/main.cpp").read_text(encoding="utf-8")
rotate = main[main.index("if (g_consoleRotate ||"):main.index("#endif  // !AWOK", main.index("if (g_consoleRotate ||"))]
post = main[main.index("if (frameBufferOk && s_recoveryBands == 1 && renderedThisLoop"):]

checks = {
  "display rotation and cached-panel invalidation precede sprite repack":
    rotate.index("tft.setRotation(screenRotation);") <
    rotate.index("FramePush::invalidate();") <
    rotate.index("frame.resizeInPlace("),
  "same stride-safe repack for CYD32 full and recovery sprites":
    "frame.resizeInPlace(tft.width(), tft.height() / s_recoveryBands);" in rotate,
  "scrub starts only after new dimensions are installed":
    rotate.index("frame.resizeInPlace(tft.width(), tft.height() / s_recoveryBands);") <
    rotate.index("frame.fillRect(0, 0, frame.bufW(), frame.bufH(), Theme::BG);"),
  "erase only the allocated sprite, never the real TFT":
    "frame.resetViewport();" in rotate
    and "frame.fillRect(0, 0, frame.bufW(), frame.bufH(), Theme::BG);" in rotate
    and "tft.fillScreen(" not in rotate
    and "tft.fillRect(" not in rotate,
  "no delete/realloc on rotate":
    "frame.deleteSprite(" not in rotate
    and "frame.createSprite(" not in rotate
    and "frame.resizeInPlace(" in rotate,
  "new orientation suppresses stale readback glitch":
    "s_rotatedSceneAt = transitionStart;" in rotate
    and "transitionStart != s_rotatedSceneAt" in post,
  "other screen transitions retain normal glitch effect":
    "Theme::drawTransitionGlitch(frame, now - transitionStart, TRANSITION_MS);" in post,
  "failsafe for missing sprite stays unchanged":
    'Serial.println("[rotate] frame buffer was never allocated' in rotate,
  "scoped to CYD32 boards, not stable 2.8 render":
    "#if defined(CYD32)\n            // Resizing reinterprets" in rotate,
  "sprite clear once per rotation rather than every loop":
    rotate.count("frame.fillRect(0, 0, frame.bufW(), frame.bufH(), Theme::BG);") == 1
    and "frame.fillRect(0, 0, frame.bufW(), frame.bufH(), Theme::BG);" not in post,
}
for name, ok in checks.items():
    print(("PASS" if ok else "FAIL") + ": " + name)
assert all(checks.values()), "CYD32 rotation stride regression guard FAILED"
print(f"Rotation stride/ghosting guards: {len(checks)} PASS")
