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

# Structured LOG uses the real shared ring and a bounded, resumable USB pump.
det = (root / "src/detection.cpp").read_text()
clock = (root / "src/clock.cpp").read_text()
log = det.split("// LOG on the console.",1)[1].split("// The lifetime tally",1)[0]
usb, uart = log.split("#else",1)
for token in ('[log] %u rows, newest first', 'row,type,mac,rssi,hits,state,when', '%u,%s,%02x:%02x:%02x:%02x:%02x:%02x,%d,%u,%s,%s', 'g_engine->logAt(i)', 'detectionTypeName(d->type)', 'Clock::formatEpochStamp(d->firstSeen', 'Clock::formatStamp(d->firstSeen', 'd->rssi', 'd->hits', '"KEPT" : (d->active ? "here" : "gone")'):
    assert token in usb and token in uart, token
assert "MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT" in usb
assert "Serial.availableForWrite()" in usb and "s_logOffset += sent" in usb
assert "budget < 16" in usb and ">= 10000u" in usb
assert "Serial.flush(" not in usb and "delay(" not in usb
assert "logDumpTick(Security::locked());" in source
assert "rxBudget = 128" in clock
assert "logDump();" in clock and 'strncasecmp(line, "LOG", 3)' in clock
assert "Serial.setTxTimeoutMs(0);" in source
print("Freenove LOG protocol, real-record snapshot, bounded USB output and security guards PASS")

assert "if (c != '\\r' && c != '\\n')" in clock
assert 'strncasecmp(line,"LOG",3) == 0' in clock
assert "millis()-lastRx" in clock
assert source.index("Serial.setTxBufferSize(2048)") < source.index("Serial.begin(SERIAL_BAUD)")
assert source.index("Serial.setRxBufferSize(512)") < source.index("Serial.begin(SERIAL_BAUD)")
