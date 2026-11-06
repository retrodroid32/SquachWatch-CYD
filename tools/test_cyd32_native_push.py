#!/usr/bin/env python3
"""CYD32 native LCD transfer isolation; static guards, hardware still required."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
src = (root/"src/main.cpp").read_text(encoding="utf-8")
push = src[src.index("static inline void pushFrame("):src.index("// Draw a whole screen",src.index("static inline void pushFrame("))]
rot = src[src.index("if (g_consoleRotate ||"):src.index("#endif  // !AWOK",src.index("if (g_consoleRotate ||"))]
checks = {
 "3.2 diagnostic explicitly uses TFT_eSPI's native full/band push":
     "#if defined(CYD32)\n    } else {" in push
     and "frame.pushSprite(x, y);" in push.split("#if defined(CYD32)\n    } else {",1)[1].split("#else",1)[0],
 "native path does not call custom FramePush burst writer":
     "FramePush::push(" not in push.split("#if defined(CYD32)\n    } else {",1)[1].split("#else",1)[0],
 "existing 2.8 fast-push implementation remains":
     "!FramePush::push(tft, frame.buf(), frame.bufW(), frame.bufH(), x, y)" in push.split("#else",1)[-1],
 "panel rotation clear is once per orientation change":
     "tft.setRotation(screenRotation);" in rot
     and "applyColorOrder();" in rot
     and "tft.fillScreen(Theme::BG);" in rot
     and rot.index("applyColorOrder();") < rot.index("tft.fillScreen(Theme::BG);")
     and rot.count("tft.fillScreen(Theme::BG);") == 1,
 "no extra physical panel clears during normal loop":
     "tft.fillScreen(Theme::BG);" not in push,
 "reused framebuffer remains allocated during rotation":
     "frame.resizeInPlace(tft.width(), tft.height() / s_recoveryBands);" in rot
     and "frame.deleteSprite(" not in rot
     and "frame.createSprite(" not in rot,
 "build prints a definite native vs fast-push identity":
     "[push] CYD32 NATIVE TFT_eSPI pushSprite" in push
     and "[rotate] CYD32 native TFT panel clear" in rot,
}
for label, ok in checks.items():
    print(("PASS" if ok else "FAIL") + ": " + label)
assert all(checks.values()), "Native push diagnostic not correctly isolated"
print(f"CYD32 native-driver rotation isolation: {len(checks)} PASS")
