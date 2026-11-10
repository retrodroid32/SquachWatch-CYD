#!/usr/bin/env python3
"""Source integration guards for the staged, boot-safe font-settings bring-up.

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
    "font settings use bounded ranges":
        "void fontRange(TFT_eSPI& t" in theme and
        "void setFontDelta(TFT_eSPI& t" in theme and
        "if (step < lo) step = lo;" in theme,
    "no unsupported XP font exposed":
        "case Settings::FontKind::XP_TILES:" in theme and
        "lo = hi = 0;" in theme,
    "proven bubble and pet rendering preserved":
        "#if BUBBLE_FONT == 2" in theme and
        "Theme::petFontOn(t);" not in pet,
    "global button renderer held at recovered baseline":
        "int requested = (int)ts + fontDelta" not in theme and
        "uint8_t ts = uiTextSize(t, textSize);" in theme,
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
print(f"Staged font settings: {len(checks)} checks PASS; global rendering unchanged")
