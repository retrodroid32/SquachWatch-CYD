#!/usr/bin/env python3
"""#180 Stage B: editable font preferences and preview, no global font hooks."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
u = (root/"src/ui_settings.cpp").read_text(encoding="utf-8")
h = (root/"include/ui_settings.h").read_text(encoding="utf-8")
sh = (root/"include/settings.h").read_text(encoding="utf-8")
s = (root/"src/settings.cpp").read_text(encoding="utf-8")
m = (root/"src/main.cpp").read_text(encoding="utf-8")
t = (root/"src/theme.cpp").read_text(encoding="utf-8")
p = (root/"src/pet.cpp").read_text(encoding="utf-8")
for x in ("SettingsPage::FONTS", "SettingsRow::FONTS", "drawFontRow(",
          "drawFontPreview(", "uiSettingsFontTap(", "Settings::fontStep(",
          "Settings::fontFace(", "Settings::setFontStep(", "Settings::setFontFace(",
          "DrawBand::has(bodyBottom", "uiSettingsParentPage()"):
    assert x in (u+m), f"Missing functional UI: {x}"
assert "SettingsPage::FONTS = " not in h
assert "FONTS = 6" in h
assert "void setFontStep(FontKind kind, uint8_t step)" in sh
assert "s_prefs.putUChar(FONT_KEYS[i], value)" in s
assert "s_prefs.putUChar(FACE_KEYS[i], value)" in s
assert "s_prefs.getUChar(FONT_KEYS[i], FONT_DEFAULT[i])" in s
assert "s_prefs.getUChar(FACE_KEYS[i], 0)" in s
assert "case SettingsRow::FONT_MENUS:" in m
assert "uiSettingsOpenPage(uiSettingsParentPage())" in m
# The actual UI, bubble, pet and global Theme renderers must not react yet.
assert "FontKind" not in t and "fontDelta(" not in t
assert "FontKind" not in p and "fontStep(" not in p
assert "Settings::fontStep(" in u and "Settings::fontFace(" in u
print("PASS #180 stage B: interactive preview and persisted controls, global fonts unaffected")
