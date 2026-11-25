"""Freenove must use the shared application, not a second feature implementation."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
profile = (root / "platformio.ini").read_text().split("[env:freenove35-st77922]", 1)[1].split("\n[env:", 1)[0]
source = (root / "src/main.cpp").read_text()
touch = (root / "src/st77922_touch.cpp").read_text()
assert "-<main.cpp>" not in profile
assert "-<freenove35_main.cpp>" in profile
assert "-DFREENOVE35_N=1" in profile and "-DSQUACH_MESH=1" in profile
for token in ("St77922::begin()", "St77922Touch::begin(38,39,48,47)", "St77922::queuePixels", "fnMap(screenRotation", "frame.setColorDepth(16)", "Security::begin()", "MeshTalk::begin()", "Theme::consumePetUnlock()", "Pet::grab(", "uiPetUnlockInit(", "enterCategoryLog(", "enterWatchList(", "enterHuntList(", "case AppState::ALERT:", "case AppState::PIN_ENTRY:", "case AppState::UPDATE:", "case AppState::WIFI_NETS:", "case AppState::SD_FILES:"):
    assert token in source, token
assert "x = s_x; y = s_y;" in touch
print("Freenove shared application architecture guard PASS")
