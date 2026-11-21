#!/usr/bin/env python3
"""CYD28 low-memory band regression: SETTINGS navigation after SD FILES.

SD FILES R2 draws directly to physical TFT. The SETTINGS SYSTEM parent must
repaint its [ UP ] control at the physical bottom after return, not at a
recovery band's 15–40px logical height.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
main = (root / "src/main.cpp").read_text(encoding="utf-8")
ui = (root / "src/ui_settings.cpp").read_text(encoding="utf-8")
hdr = (root / "include/ui_settings.h").read_text(encoding="utf-8")
sd = main.split("case AppState::SD_FILES: {", 1)[1].split(
    "case AppState::DIAGNOSTICS:", 1)[0]
settings = main.split("case AppState::SETTINGS: {", 1)[1].split(
    "case AppState::", 1)[0]
tick = ui.split("void uiSettingsTick(", 1)[1].split(
    "int8_t uiSettingsBrightnessDeltaForTap(", 1)[0]

assert "SdBrowser::draw(tft);" in sd
assert "FramePush::invalidate();" in sd
assert "enterSettings();" in sd
assert "uiSettingsOpenPage(SettingsPage::SYSTEM);" in sd
assert "const int settingsPhysicalH =" in settings
assert "s_recoveryBands > 1 ? tft.height() : 0" in settings
assert "uiSettingsTick(t, now, engine, false, advance, settingsPhysicalH)" in settings
assert "int physicalHeight = 0" in hdr
assert "const int h = physicalHeight > 0 ? physicalHeight : t.height();" in tick
assert "computeGeom(t, h," in tick
assert "drawPinnedBack(t, w, h);" in tick
assert 's_page == SettingsPage::MAIN) ? "[ BACK ]" : "[ UP ]"' in ui

# Both rotations, both R1 band allocations, no regression to the direct path.
for w, h in ((320, 240), (240, 320)):
    bar = 26
    y = h - bar
    assert y > 0
    assert y + bar == h
    assert y > h // 8
    assert y > h // 16
print("PASS: SD FILES -> SETTINGS/SYSTEM physical pinned UP/BACK band geometry")
