#include "draw_band.h"

#if defined(CYD35) || defined(ARDUINO_ARCH_ESP32)
namespace DrawBand {
// CYD35 uses bands from boot. Ordinary ESP32 CYDs use full-screen drawing
// until WiFi-OTA recovery activates a smaller sprite at runtime.
int16_t g_y0 = 0, g_y1 = 32767;
bool    g_on =
#if defined(CYD35)
    true;
#else
    false;
#endif
}
#endif
