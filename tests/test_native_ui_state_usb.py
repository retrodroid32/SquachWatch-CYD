"""Guard rails for the draft SquachWatch USB UI state protocol."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
REMOTE = (ROOT / "src/remote_mirror.cpp").read_text()
CLOCK = (ROOT / "src/clock.cpp").read_text()
MAIN = (ROOT / "src/main.cpp").read_text()
STATE = (ROOT / "include/state.h").read_text()


class NativeUiStateTests(unittest.TestCase):
    def test_explicit_opt_in_and_off(self):
        self.assertIn('strcmp(line,"UI ON")', REMOTE)
        self.assertIn('strcmp(line,"UI GET")', REMOTE)
        self.assertIn('strcmp(line,"UI OFF")', REMOTE)
        self.assertIn("UI READY 1 SQUACHWATCH TOUCH STATE", REMOTE)
        self.assertIn("if(!s.uiEnabled)return;", REMOTE)

    def test_pin_lock_gate_precedes_commands(self):
        gate = CLOCK.index("if (Security::locked())", CLOCK.index("void pollSerial()"))
        mirror = CLOCK.index("RemoteMirror::command(line)", gate)
        ui = CLOCK.index("RemoteMirror::uiCommand(line)", gate)
        self.assertLess(gate, mirror)
        self.assertLess(mirror, ui)
        self.assertIn("if(locked){", REMOTE)
        self.assertIn("s.uiEnabled=false", REMOTE)

    def test_shared_touch_dispatch_and_bounds(self):
        self.assertIn("RemoteMirror::touch(tft.width(),tft.height(),rx,ry)", MAIN)
        self.assertIn('sscanf(line,"UI TOUCH %d %d %7s"', REMOTE)
        self.assertIn("x<s.uiWidth && y<s.uiHeight", REMOTE)
        self.assertIn("millis()-s.lastTouch>4000", REMOTE)

    def test_bounded_metadata_only(self):
        self.assertIn("now-s.lastUiAt<200", REMOTE)
        self.assertIn("Serial.availableForWrite()<64", REMOTE)
        self.assertIn('Serial.printf("UI STATE 1 %u %d %d %u %lu\\n"', REMOTE)
        self.assertIn("RemoteMirror::uiTick((uint8_t)state", MAIN)
        self.assertRegex(STATE, r"enum class AppState\s*:\s*uint8_t")
        # The UI STATE frame is intentionally metadata, not detections or pixels.
        body = REMOTE.split("void uiTick(", 1)[1].split("bool touch(", 1)[0]
        self.assertNotIn("MIRROR ROW", body)
        self.assertNotIn("Detection", body)


if __name__ == "__main__":
    unittest.main()
