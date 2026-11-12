// SquachWatch-CYD — five genuine compact button-font sizes.
// Vertically scaled 5x7 glyphs have deliberately condensed horizontal
// advances: SCAN, LOG and DESK remain readable on 69px portrait buttons.
#pragma once
#include <stdint.h>
namespace ButtonFont {
static const uint8_t HEIGHT[5] = {8, 11, 14, 18, 22};
static const uint8_t ADVANCE[5] = {6, 7, 8, 9, 10};
inline uint8_t step(uint8_t v) { return v > 4 ? 4 : v; }
inline int height(uint8_t v) { return HEIGHT[step(v)]; }
inline int advance(uint8_t v) { return ADVANCE[step(v)]; }
inline int width(const char* label, uint8_t v) {
    if (!label || !*label) return 0;
    int count=0; for (const char* p=label; *p; ++p) ++count;
    return count * advance(v) - 1;
}
inline int barHeight(uint8_t v) {
    const int h = height(v) + 10;
    return h < 20 ? 20 : h;
}
inline int bestStep(const char* label, int w, int h, uint8_t wanted) {
    for (int i=(int)step(wanted); i>=0; --i)
        if (width(label, (uint8_t)i) <= w && height((uint8_t)i) <= h)
            return i;
    return -1;
}
}
