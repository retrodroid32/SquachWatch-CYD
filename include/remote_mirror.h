#pragma once
// LAB-only serial USB framebuffer streaming and remote touch on SquachWatch.
// This is an explicit opt-in. Commands are blocked by Clock::pollSerial()
// while the on-device PIN is locked. No network listener is created.
// The source is the 8-bit TFT_eSprite buffer actually pushed to the LCD.
#include <stdint.h>
namespace RemoteMirror {
  bool command(const char* line);
  // Returns true when a valid active injected press overrides physical input.
  bool touch(int width,int height,int& x,int& y);
  void capture(const uint8_t* bytes,int width,int bandHeight,int panelHeight,int y);
  bool requested();
}
