// SD file explorer — all filesystem operations are explicit user actions.
// Drawing uses cached names/text; it never polls an absent SD card.
#pragma once
#include <TFT_eSPI.h>
class SdLog;
namespace SdBrowser {
    void begin(SdLog& sd);
    void end();
    bool reloadMountedCard(); // clear stale directory/preview and reload root
    // Physical geometry is optional: short CYD28 framebuffer stripes must use
    // rotated LCD coordinates without drawing directly to the LCD.
    void draw(TFT_eSPI& t, int physicalW = 0, int physicalH = 0);
    bool tap(int x, int y, int screenW, int screenH); // true => leave screen
}
