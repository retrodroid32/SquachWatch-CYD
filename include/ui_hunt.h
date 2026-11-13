// SquachWatch-CYD — HUNT MODE: live signal-strength gauge for the
// watched target (see DetectionEngine::watchBle/watchWifi and its
// watchRssiCount()/watchRssiAt() history). No magnetometer on this
// hardware, so this deliberately isn't a self-orienting compass -- it's
// a strength meter you sweep by hand/body the way real fox-hunting
// works with a plain omnidirectional receiver: rotate to find where
// the signal fades, walk toward where it doesn't.
#pragma once
#include <TFT_eSPI.h>
#include "detection.h"

void uiHuntInit(TFT_eSPI& t);
// `advance` is false on the second of the 3.5"'s two band passes -- the
// same frame drawn again -- so anything that steps by the call rather
// than by the clock must sit still for it. Other boards draw once.
// The renderer can pass a half-height sprite on the CYD32. Always accept
// the physical screen dimensions separately from t.width()/t.height() so
// footer drawing and tft-based hit testing use one coordinate system.
void uiHuntTick(TFT_eSPI& t, uint32_t now, const DetectionEngine& eng,
                bool advance = true, int screenW = 0, int screenH = 0);
void uiHuntListTick(TFT_eSPI& t, uint32_t now, const DetectionEngine& eng,
                    int screenW = 0, int screenH = 0);
bool uiHuntHitBack(int x, int y, int screenW, int screenH);
// STOP ends the hunt outright (DetectionEngine::clearHunt); BACK leaves the
// target set. main.cpp owns navigation and returns either exit to the screen
// that launched HUNT when that context is known.
bool uiHuntHitStop(int x, int y, int screenW, int screenH);
// Toggle an alternate session history chart, without changing the target.
bool uiHuntHitGraph(int x, int y, int screenW, int screenH);
void uiHuntToggleGraph();

// When no single target is active, HUNT MODE becomes the ranked selected-
// targets list. The hit returns the DetectionEngine storage index to activate.
int  uiHuntListHitTarget(TFT_eSPI& t, int x, int y, int screenW, int screenH);
bool uiHuntListHitBack(int x, int y, int screenW, int screenH);
void uiHuntListScroll(int delta);
// True while the gauge is showing CAUGHT: the signal has sat at
// arm's-length strength for two samples running. The status light reads
// it so the catch shows from the back of the board too.
bool uiHuntCaught();
