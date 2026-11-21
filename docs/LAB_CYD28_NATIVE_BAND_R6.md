# CYD 2.8 LCD redraw / restart isolation — LAB R6

## Hardware report
User reports R5 is still faulty, with unchanged main-screen flicker,
missing UI elements, and redraw artifacts. The supplied 44-second video
also visibly shows CHECKING FOR UPDATES around 10 seconds followed by
startup visuals. That suggests an actual restart or equivalent boot
sequence at least once; it is not proof of a particular reset cause.

## Two concrete code paths changed
- Classic CYD28 8/16-band low-memory framebuffer now transfers entire
  already-rendered 8-bit sprite stripes using TFT_eSPI::pushSprite(), not
  FramePush's custom raw-SPI register bursts and cached row spans.
  Native transfer is already used by CYD32. This is a controlled isolation
  from the raw SPI backend, not a claim that hardware is validated.
- enterSettings() no longer calls uiSettingsInit() on the physical LCD
  while classic CYD28 multi-band recovery is active. Its state-reset
  and initial clear happen on the retained small sprite instead, so the
  live glass is not blanked before the R4 direct menu redraw starts.
- Startup prints [cyd28-r6] boot reset=... with the raw ESP32 reset
  reason and heap measures. The existing boot banner/crash reporter
  also retains its previous reset diagnostics.

R1 SD-in low-memory safeguards, R2 SD browser, R4 direct SETTINGS,
R5 stripe clearing, all board variants outside classic CYD28, OTA
partition layout and radio code are unchanged.

## Hardware acceptance criteria
1. Flash R6 LAB CYD 2.8 ST7789. Boot with SD inserted 3 times.
2. Capture serial at 2000000 baud around any disappearance / redraw:
   identify [cyd28-r6] boot reset=, the standard boot banner's
   'last reset', and any crash message. A restart is not LCD flicker.
3. Leave MAIN with Synthwave background on for 45+ seconds. Verify
   animation, text, mascot, counters and bottom buttons do not disappear.
4. Repeat MAIN ↔ SETTINGS 10 times, and SYSTEM → SD FILES → BACK.
   Check no all-black transition and no old menu pixels on MAIN.
5. Repeat both orientations, Wi-Fi/BLE and GPS/SD/mesh smoke checks.
6. If display still tears, collect a new short video AND serial reset
   lines; don't merge or conclude success from CI alone.

Strictly LAB only; PR is stacked on R5 #248, R4 #247, R3 #246,
R2 #245, R1 #241, upstream #239. No merge, public tag or
production release without explicit user approval.
