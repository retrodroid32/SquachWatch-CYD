#pragma once
// Authentic, opt-in USB RGB332 LCD mirroring for classic CYDs.
// Framing is line-based; only completed keyframes/deltas may be displayed.
// No network server, synthetic screen renderer, or full-frame mirror heap.
#include <stdint.h>
namespace RemoteMirror {
  bool command(const char* line);
  bool touch(int panelWidth,int panelHeight,int& x,int& y);
  void capture(const uint8_t* rgb332,int width,int bandHeight,int panelHeight,int y);
  // Suppress USB mirror records while real receiver LOG evidence is exported.
  void pauseEvidence();
  // Direct-panel fallback has no authentic sprite pixels to report.
  void unavailable();
  // Timeout pending requests if rendering switched to an unsupported path.
  void tick();
  // Disable pixel capture when the USB host selects native UI mode.
  void stopForNative();
}
