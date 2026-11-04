#!/usr/bin/env python3
"""Source integration guards for the upstream v1.40 font backport.

Full firmware/simulator builds remain mandatory; these do not replace
2.8/3.2-inch portrait/landscape touch and clipping hardware validation.
"""
from pathlib import Path

root = Path(__file__).resolve().parent.parent
def src(path): return (root / path).read_text(encoding="utf-8")

settings_h = src("include/settings.h")
settings = src("src/settings.cpp")
theme_h = src("include/theme.h")
theme = src("src/theme.cpp")
ui_h = src("include/ui_settings.h")
ui = src("src/ui_settings.cpp")
main = src("src/main.cpp")
pet = src("src/pet.cpp")
names = ["MENUS", "TEXT", "BUTTONS", "BUBBLES", "XP_TILES", "PETS"]

checks = {
    "six persistent font kinds":
        "enum class FontKind : uint8_t" in settings_h and all(n in settings_h for n in names),
    "original defaults preserved":
        "FONT_DEFAULT[(int)FontKind::COUNT] = {2, 2, 2, 2, 2, 2}" in settings,
    "upstream keys":
        all(k in settings for k in ("fntMenu", "fntText", "fntBtn", "fntBub", "fntXp", "fntPet")),
    "face keys":
        all(k in settings for k in ("fcMenu", "fcText", "fcBtn", "fcBub", "fcXp", "fcPet")),
    "clamps invalid NVS steps and faces":
        "if (s_fontStep[i] > 4)" in settings and "if (s_fontFace[i] > 2)" in settings,
    "persists steps and faces":
        's_prefs.putUChar(FONT_KEYS[(int)k], step)' in settings and
        's_prefs.putUChar(FACE_KEYS[(int)k], face)' in settings,
    "font ranges and measurement in theme":
        "void fontRange(TFT_eSPI& t" in theme and
        "t.textWidth(label) <= w - 6" in theme and
        "t.fontHeight() <= h - 2" in theme,
    "no unsupported XP font exposed":
        "case FontKind::XP_TILES:" in theme and
        "lo = 0; hi = 0; return;" in theme,
    "measured bubble box":
        "s_bubbleHeight = t.fontHeight();" in theme and
        "int  bubbleTextH()  { return s_bubbleHeight; }" in theme,
    "pet metrics included":
        "Theme::petFontOn(t);" in pet and "Theme::petFontOff(t);" in pet,
    "dedicated nested FONTS page":
        "FONTS = 6" in ui_h and "uiSettingsParentPage()" in ui,
    "six actual FONTS rows":
        all("SettingsRow::FONT_" + n.replace("XP_TILES", "XP") in ui for n in names),
    "scroll and preview":
        "g_scrollFor[7]" in ui and "drawFontPreview(t" in ui,
    "tap and redraw use shared geometry":
        "fontSliderX(screenW" in ui and "fontSliderX(w" in ui and
        "uiSettingsHitTest(t, x, y, screenW, screenH)" in ui,
    "main font controls connected":
        "case SettingsRow::FONTS:" in main and
        "uiSettingsFontTap(*canvas" in main,
    "nested back preserves appearance":
        "uiSettingsOpenPage(uiSettingsParentPage())" in main,
}
for label, ok in checks.items():
    print(("PASS" if ok else "FAIL") + ": " + label)
assert all(checks.values()), "font backport integration incomplete"
print(f"Font-controls integration: {len(checks)} checks PASS")
