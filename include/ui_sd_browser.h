// SD file explorer — all filesystem operations are explicit user actions.
// Drawing uses cached names/text; it never polls an absent SD card.
#pragma once
#include <TFT_eSPI.h>
class SdLog;
namespace SdBrowser {
    void begin(SdLog& sd);
    void end();
    void draw(TFT_eSPI& t);
    bool tap(int x, int y, int screenW, int screenH); // true => leave screen
}
