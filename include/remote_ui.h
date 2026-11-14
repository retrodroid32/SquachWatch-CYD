#pragma once
// LAB: low-bandwidth authoritative SquachWatch UI model on the USB console.
// Independent of the authentic RGB332 MIRROR protocol.
#include <stdint.h>
class DetectionEngine;
namespace RemoteUi {
  bool command(const char* line); // only called AFTER serial PIN gate
  void tick(uint8_t screen,int width,int height,uint8_t rotation,
            const DetectionEngine& engine,uint8_t logCategory);
  bool touch(int width,int height,int& x,int& y);
  void stop(); // revoked on PIN lock, pixel fallback or explicit UI OFF
}
