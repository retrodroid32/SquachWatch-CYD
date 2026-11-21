// SquachWatch-CYD — boot splash screen
#pragma once
#include <TFT_eSPI.h>
#include <stdint.h>

void uiBootInit(TFT_eSPI& t);
// Optional real LCD geometry for low-memory stripe sprites. Defaults keep
// normal full-frame and emulator callers unchanged. suppressGlitch prevents
// deliberate boot-only glitch bursts while testing CYD28 flicker recovery.
void uiBootTick(TFT_eSPI& t, uint32_t now, int physicalW = 0,
                int physicalH = 0, bool suppressGlitch = false);
bool uiBootDone(uint32_t startMs, uint32_t holdMs = 3000);
