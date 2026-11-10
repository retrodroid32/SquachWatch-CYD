#!/usr/bin/env python3
"""Verify the first Fonts regression bisect isolates startup NVS loading."""
from pathlib import Path
root = Path(__file__).resolve().parent.parent
src = (root / "src/settings.cpp").read_text(encoding="utf-8")
header = (root / "include/settings.h").read_text(encoding="utf-8")
main = (root / "src/main.cpp").read_text(encoding="utf-8")
theme = (root / "src/theme.cpp").read_text(encoding="utf-8")
ui = (root / "src/ui_settings.cpp").read_text(encoding="utf-8")
assert all(k in src for k in (
    "fntMenu", "fntText", "fntBtn", "fntBub", "fntXp", "fntPet",
    "fcMenu", "fcText", "fcBtn", "fcBub", "fcXp", "fcPet"))
assert 's_prefs.getUChar(FONT_KEYS[i], FONT_DEFAULT[i])' in src
assert 's_prefs.getUChar(FACE_KEYS[i], 0)' in src
assert '[fonts-bisect-a]' in src
assert "FontKind" not in header
assert "SettingsPage::FONTS" not in ui
assert "uiSettingsFontTap" not in main
assert "fontDelta(" not in theme
print("PASS: Fonts startup NVS-only bisect, with known-good UI and drawing")
