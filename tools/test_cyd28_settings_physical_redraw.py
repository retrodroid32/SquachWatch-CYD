#!/usr/bin/env python3
"""CYD28 R4: SETTINGS must avoid short-sprite text drawing after SD FILES."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
main = (root / "src/main.cpp").read_text(encoding="utf-8")
settings = main.split("case AppState::SETTINGS: {", 1)[1].split(
    "case AppState::", 1)[0]
sd = main.split("case AppState::SD_FILES: {", 1)[1].split(
    "case AppState::DIAGNOSTICS:", 1)[0]

# R2 direct SD FILES remains intact, with next-screen cache invalidation.
assert "SdBrowser::draw(tft);" in sd
assert "FramePush::invalidate();" in sd
assert "enterSettings();" in sd
assert "uiSettingsOpenPage(SettingsPage::SYSTEM);" in sd

# Classic CYD28 alone gets physical-panel SETTINGS under memory recovery.
assert "defined(CYD) && !defined(CYD32) && !defined(CYD35)" in settings
assert "if (frameBufferOk && s_recoveryBands > 1)" in settings
assert "uiSettingsTick(tft, now, engine, false);" in settings
assert "FramePush::invalidate();" in settings
assert "paintedScene != transitionStart || paintedPage != page" in settings
assert "touchJustDown || touchJustUp || paintNext" in settings
assert "paintNext = touchJustUp;" in settings
assert "moved && (uint32_t)(now - paintedAt) >= 250UL" in settings
assert "(uint32_t)(now - paintedAt) >= 30000UL" in settings
assert settings.index("uiSettingsTick(tft, now, engine, false)") < settings.index(
    "if (frameBufferOk) {"
)

# Normal frame-buffer render and low-RAM direct fallback are preserved.
assert "uiSettingsTick(t, now, engine, false, advance, settingsPhysicalH)" in settings
assert "uiSettingsTick(*canvas, now, engine, true);" in settings
assert "uiSettingsTapPinnedBack(" in settings

print("PASS: CYD28 R4 physical full-panel SETTINGS, state/redraw gates, SD FILES return")
