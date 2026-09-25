// SquachWatch-CYD — persistent detection timeline
#pragma once
#include <TFT_eSPI.h>
#include <stdint.h>

void uiTimelineInit(TFT_eSPI& t);
void uiTimelineTick(TFT_eSPI& t, uint32_t now);
void uiTimelineScroll(int delta);
