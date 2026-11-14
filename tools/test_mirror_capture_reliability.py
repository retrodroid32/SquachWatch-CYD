#!/usr/bin/env python3
"""SQW LCD source provenance and bounded FRAME failure regression guards.
Source-only tests, NOT physical proof of CYD screen capture.
"""
from pathlib import Path
p=Path(__file__).resolve().parents[1]
def read(name):return (p/name).read_text(encoding="utf8")
main=read("src/main.cpp")
mirror=read("src/remote_mirror.cpp")
hdr=read("include/remote_mirror.h")
clock=read("src/clock.cpp")
workflow=read(".github/workflows/build.yml")
assert "void panelState(uint8_t mode,uint8_t bands,uint8_t screen,uint8_t rotation)" in mirror
assert "void panelState(uint8_t mode,uint8_t bands,uint8_t screen,uint8_t rotation);" in hdr
assert "MIRROR STATUS 2 %s %u %u %u %u %lu %d %d %lu %lu %lu" in mirror
assert "MIRROR QUEUED %u %u %u" in mirror
assert '"MIRROR ERROR DIRECT_PANEL_UNSUPPORTED"' in mirror
assert '"MIRROR ERROR NO_AUTHENTIC_BUFFER"' in mirror
assert '"SCREEN_CHANGED"' in mirror
assert "s.baselineValid=false;" in mirror
assert "s.down=false;" in mirror
assert "constexpr int MAX_SENT_PER_PASS = 4;" in mirror
assert "constexpr int MAX_SCANNED_PER_PASS = 32;" in mirror
assert "Serial.availableForWrite()<n+32" in mirror
assert "Serial.setTxBufferSize(2048);" in main
assert "s_mirrorPresentedState=state;" in main
assert "s_mirrorPresentedRotation=screenRotation;" in main
assert "s_mirrorPresentedTransition=transitionStart;" in main
assert "s_mirrorPresentedState==state" in main
assert "s_mirrorPresentedTransition==transitionStart" in main
assert "mirrorSource==1" in main
assert "if(state==AppState::SD_FILES) mirrorSource=3;" in main
assert "RemoteMirror::panelState(mirrorSource,s_recoveryBands," in main
assert main.index("RemoteMirror::panelState(mirrorSource") < main.index("Clock::pollSerial();", main.index("void loop()"))
assert "forceClearVisual || RemoteMirror::wantsRender()" in main
assert "const bool wlNeedsDraw" in main
assert "const bool hlNeedsDraw" in main
wl=main.split("const bool wlNeedsDraw",1)[1].split("if (wlNeedsDraw)",1)[0]
hl=main.split("const bool hlNeedsDraw",1)[1].split("if (hlNeedsDraw)",1)[0]
assert "RemoteMirror::wantsRender()" in wl
assert "RemoteMirror::wantsRender()" in hl
assert "RemoteMirror::capture(frame.buf(),frame.bufW(),frame.bufH(),tft.height(),y)" in main
assert "RemoteMirror::pauseEvidence();\n            logDump();" in clock
assert "python3 tools/test_mirror_capture_reliability.py" in workflow
print("PASS: source-aware native LCD captures and bounded error/progress records")
