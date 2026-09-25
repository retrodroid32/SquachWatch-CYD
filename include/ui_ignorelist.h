// SquachWatch-CYD — ignored-devices screen, reached via Settings'
// "IGNORED DEVICES" row. A flat scrollable list, one row per muted MAC,
// each with a REMOVE hit zone on the right. Same row-list shape as the
// detection filter, which is the closest existing sibling: a list of
// things you turn off, one tap per row.
//
// The list is short by construction (IgnoreList::MAX is 64) and is
// usually far shorter, so there is no search or paging here -- scrolling
// a handful of rows is the whole interaction.
#pragma once
#include <TFT_eSPI.h>
#include <stdint.h>
#include "state.h"

void uiIgnoreListInit(TFT_eSPI& t);
void uiIgnoreListTick(TFT_eSPI& t, uint32_t now);
void uiIgnoreListScroll(int delta);   // positive = scroll down

// Index of the row whose policy button falls at (x,y), or 0xFF for none.
// A tap cycles IGNORE -> TRUSTED -> ALWAYS ALERT -> NORMAL/remove. Keeping
// the rest of the row inert prevents an accidental swipe from changing a
// device policy.
uint8_t uiIgnoreListHitPolicy(TFT_eSPI& t, int x, int y, int screenW, int screenH);
