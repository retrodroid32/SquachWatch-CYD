# CYD 2.8 SD-in startup low-memory recovery — LAB R1

Reported hardware: CYD 2.8 ST7789, SD card inserted at power-on. Latest serial showed OTA check heap 138400, largest 53236 followed by 76800-byte full-frame allocation failure, intentional software reset, then SD mounted with 25952 free / 15348 largest before mount, 10116 free / 48 largest after SD, MeshTalk crypto self-test failure and abort.

This LAB fixes the observed reboot chain by:
- Allocating a bounded, retained 8-bit 8-band (9600-byte) sprite on original CYD 2.8, with a 16-band (4800-byte) retry and direct-panel fallback, avoiding the failed 76800-byte allocation and OTA-triggered restart.
- Retaining existing shared DrawBand viewport rendering, FramePush push, rotation in-place resize, GPS and radio setup; excludes CYD32/CYD35/AWOK/Phantom.
- Deferring first SD FAT/VFS mounting when measured heap <45000 or contiguous heap <28000. This is intentional: the user's device went from 15348 largest to 48 after mount, then crypto initialization failed.
- Deferring creation of the 4096-word SD monitor task until after the mandatory MeshTalk crypto self-test, and only if heap >=24000 and largest >=12000.
- Logging selected frame bands and a clear SD DEFERRED - LOW RAM status.

IMPORTANT TRADE-OFF: SD may remain deferred until additional memory is recovered or the SD mount strategy is optimized. This revision prioritizes reliable boot and radio/security operation, NOT a claim that an inserted SD card will always be mounted. Do not misreport boot success as SD PASS. On this LAB, a deferred SD monitor will not start later automatically; a follow-up is needed for dynamic recovery if required.

Validation: build+release matrix, host tests, CYD 2.8 with SD inserted (3 power cycles), check first boot no software reset, inspect [frame-boot] CYD28 and [sd] deferred/ready, test MeshTalk self-test, touch, screen portrait/landscape, BLE/Wi-Fi scan, detection, GPS, RTC and 2.8 performance. Check SD functional separately; no data erasure. No merge without approval.
