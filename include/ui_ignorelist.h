// SquachWatch-CYD — persistent per-device policy screen.
//
// Reuses the old ignored-devices state/geometry so no new app state or
// navigation path is required. Only the right-side policy button is active;
// the rest of each row remains inert so a scrolling gesture cannot silently
// change a device policy.
#pragma once
#include <TFT_eSPI.h>
#include <stdint.h>
#include "state.h"

void uiIgnoreListInit(TFT_eSPI& t);
void uiIgnoreListTick(TFT_eSPI& t, uint32_t now);
void uiIgnoreListScroll(int delta);   // positive = scroll down

// Index of the row whose policy button falls at (x,y), or 0xFF for none.
// A tap cycles IGNORE -> TRUSTED -> ALWAYS ALERT -> NORMAL/remove.
uint8_t uiIgnoreListHitPolicy(TFT_eSPI& t, int x, int y, int screenW, int screenH);
