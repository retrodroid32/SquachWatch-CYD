// The LIVE PC emulator has no SD card attached. Build the SD browser's
// navigation glue without linking Arduino's SD/FatFS library. These are not
// replacements for the firmware's real file browser and perform no I/O.
#include "ui_sd_browser.h"
#include "theme.h"
class SdLog;
namespace SdBrowser {
void begin(SdLog&) {}
void end() {}
bool reloadMountedCard() { return false; } // emulator has no physical card
void draw(TFT_eSPI& t) {
    t.fillScreen(Theme::BG);
    Theme::drawTitleBar(t, "SD FILES");
    t.setTextFont(1);
    t.setTextSize(2);
    t.setTextColor(Theme::CYAN, Theme::BG);
    t.setCursor(8, 32); t.print("SD FILES");
    t.setCursor(8, 62); t.print("NO SD IN SIM");
    Theme::drawButton(t, 0, t.height() - 30, t.width(), 30, "[ BACK ]", false, 1);
}
bool tap(int, int y, int, int h) { return y >= h - 30; }
}
