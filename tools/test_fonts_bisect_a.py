#!/usr/bin/env python3
"""Guard the post-PR216 #209 Fonts bisect (NVS-only, no new UI/drawing)."""
from pathlib import Path

root = Path(__file__).resolve().parent.parent
s = (root / "src/settings.cpp").read_text(encoding="utf-8")
h = (root / "include/settings.h").read_text(encoding="utf-8")
ui = (root / "src/ui_settings.cpp").read_text(encoding="utf-8")
main = (root / "src/main.cpp").read_text(encoding="utf-8")
theme = (root / "src/theme.cpp").read_text(encoding="utf-8")
for key in ("fntMenu","fntText","fntBtn","fntBub","fntXp","fntPet",
            "fcMenu","fcText","fcBtn","fcBub","fcXp","fcPet"):
    assert key in s, f"missing legacy NVS key {key}"
for token in ("s_prefs.getUChar(FONT_KEYS[i], FONT_DEFAULT[i])",
              "s_prefs.getUChar(FACE_KEYS[i], 0)",
              "s_fontStep[i] > 4", "s_fontFace[i] > 2",
              "[fonts-bisect-a]", "UI/render unchanged"):
    assert token in s, f"missing NVS isolation guard: {token}"
# Stage A alone has no UI. In the stacked Stage B branch a tightly scoped
# preview page is permitted, but global theme rendering must still be stable.
if "SettingsPage::FONTS" not in ui:
    assert "FontKind" not in h and "uiSettingsFontTap" not in main
else:
    assert "FontKind" in h and "uiSettingsFontTap" in main
if (root / "tools/test_fonts_live_render.py").exists():
    assert "fontDelta(" in theme and "s_bubbleHeight = t.fontHeight()" in theme
else:
    assert "fontDelta(" not in theme
# Do not regress the hard-won OTA partition and BIN size safety mechanisms.
part = (root / "partitions_ota.csv").read_text(encoding="utf-8")
ci = (root / ".github/workflows/build.yml").read_text(encoding="utf-8")
assert "0x1F0000" in part and "0x1E4000" in part
assert "tools/check_app_image_sizes.py" in ci
print("PASS #209 NVS storage guard; rendering remains isolated")
