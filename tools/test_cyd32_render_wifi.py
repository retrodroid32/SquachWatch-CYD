#!/usr/bin/env python3
"""Static integration guard for CYD32 redraw budget and live Wi-Fi status."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
r=lambda p:(root/p).read_text(encoding="utf-8")
main=r("src/main.cpp")
fp=r("src/frame_push.cpp")
theme=r("src/theme.cpp")
wifi=r("src/ota_wifi.cpp")
sim=r("sim/ota_sim.cpp")
hdr=r("include/ota_wifi.h")
menu=main.split("static bool cyd32MenuState(",1)[1].split("template <typename F>",1)[0]
badge=theme.split("static void drawWifiIcon(",1)[1].split("static void drawLockIcon(",1)[0]
checks={
 "125ms visual budget for noncritical menus":
    "(now-lastPaint) < 125U" in menu,
 "touch-down/up and rotation force repaint":
    all(s in menu for s in ("s_ui32TouchDown","s_ui32TouchUp",
        "transitionStart!=lastScene","repaintAfterRelease=s_ui32TouchUp")),
 "no throttle on alert, scan, clear or keyboard":
    all(("case AppState::"+s+":") not in menu for s in
        ("CLEAR","ALERT","WATCH_ALERT","RAWSCAN","WIFI_PASS","WIFI_NETS")),
 "primary menus governed by cadence":
    all(("case AppState::"+s+":") in menu for s in
        ("SETTINGS","POWER_SAVER","SECURITY","STATUS_LIGHT","LOG","DIAGNOSTICS")),
 "skipping menu draw never pushes stale sprite":
    "if (s_ui32SkippedFrame) renderedThisLoop=false" in main,
 "CYD32 refreshes complete band group once per 240 frames":
    "period = bands * 240U" in fp and "(s_frameNo % period) < bands" in fp,
 "green requires actual live Wi-Fi":
    "SavedResult::JOINED" not in badge and
    "if (OtaWifi::liveConnected()) ink = GREEN" in badge,
 "bad password, out of range, connection attempt distinct":
    "SavedResult::BAD_PASSWORD" in badge and "SavedResult::NOT_FOUND" in badge
    and "SavedTest::CONNECTING" in badge,
 "real and simulated current radio status wired":
    "WiFi.status() == WL_CONNECTED" in wifi and
    "bool        liveConnected();" in hdr and
    "bool       liveConnected()" in sim,
 "instrumentation logs draw/skip counts":
    "[ui32] menu draw" in main,
}
for name,ok in checks.items():
    print(("PASS" if ok else "FAIL")+": "+name)
assert all(checks.values()),"CYD32 GUI/Wi-Fi status guard FAILED"
print(f"CYD32 checks: {len(checks)} PASS")
