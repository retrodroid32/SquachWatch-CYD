# Classic CYD Pocket Mode (v1.41 parity draft)
- On 2.8 ST7789 and 3.2 ST7798, short BOOT/GPIO0 press (release within 40–1199 ms) toggles a temporary dark screen; never persist across reboot.
- Ignore BOOT held during initial boot; never drive GPIO0; long hold >=1200 ms has no pocket action.
- Touch while dark must not wake or trigger any UI action. Second short BOOT press wakes.
- BLE/Wi-Fi scanning, logging, SquachMesh and GPS continue in the dark. No ESP deep sleep.
- An allowed alert may light the display temporarily; pocket status remains enabled and darkness resumes afterwards. Test this particularly in Desk and when SCREEN TIMEOUT is NEVER.
- Test boot with GPIO0 held, boot release, Wi-Fi/SD/GPS, touch coordinate regression, portrait/landscape, NVS preservation, and both CYDs.
- CI and physical hardware required; do not merge or release from this draft automatically.
