# LAB CYD32 SD-in boot frame fallback R1

Observed user hardware: CYD 3.2 with SD inserted at power-on loops after OTA boot check; without SD boots, inserting SD after boot works. Serial: boot check heap 138424, largest 53236; ERROR: frame buffer allocation failed (low memory).

The historical CYD32 startup required a full RGB332 320x240 76800-byte sprite unless compiled with GPS_SUPPORT/GPS_RX_PIN; GPS variant used a 2-band 38400-byte sprite. After a failed allocation following OTA boot check, firmware deliberately restarted once. This LAB uses the existing multi-band renderer for every CYD32 and starts at 2 bands, then tries 4, 8 and 16 bands, preserving a 1024-byte free-block margin. If all allocations fail it records a direct-panel fallback and continues startup without a framebuffer-induced restart. No change to CYD 2.8, SD mount, detection, OTA, PIN, or mirror protocol.

Serial identifiers:
- [frame-boot] skip N bands: need ... + reserve, largest ...
- [frame-boot] CYD32 ready: bands=N sprite=... heap=... largest=...
- [frame-boot] CYD32 DIRECT PANEL: no sprite; heap=... largest=...

Hardware gates: reset/power cycle 3.2 with SD inserted 3 times; confirm SD mount and file listing; run WiFi boot check; repeat with no SD and hot insert; rotate portrait/landscape and navigate LOG/SCAN/SETTINGS/CLEAR/WATCH/HUNT; measure redraws/FPS, heap, BLE/WiFi, GPS, SquachMesh; confirm 2.8 unchanged. No merge or production release pending hardware approval.

CI static guard tools/test_cyd32_sd_boot_adaptive.py and existing native push / shared draw / firmware matrix. Build success is not proof of physical boot. 
