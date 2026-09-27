// SquachWatch-CYD — watched-target alert screen implementation
#include "ui_watchalert.h"
#include "theme.h"
#include "squachy.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

// Shared by the drawing and the hit test so the two cannot drift -- the rule
// every other panel here follows. Full width minus a margin: it is the only
// control on the screen, so there is nothing for it to crowd.
static const int REMOVE_H = 26;
static void removeRect(TFT_eSPI& t, int& x, int& y, int& w, int& h) {
    const int margin = 10;
    w = t.width() - 2 * margin;
    if (w > 240) w = 240;
    h = REMOVE_H;
    x = (t.width() - w) / 2;
    y = t.height() - h - margin;
}

// The full label needs ~22 characters and the narrowest portrait rotation
// cannot hold it. Shortened rather than shrunk: size-1 is already the
// smallest the built-in font offers.
static const char* removeLabel(TFT_eSPI& t, int w) {
    const char* full = "REMOVE FROM WATCH LIST";
    // At the size this button will actually be drawn. On a wide panel that is
    // size 2, where the full label wants 270 px in a 240 px box -- so it takes
    // the short word and keeps the bigger letters, rather than keeping all
    // twenty-two characters and being the one small button on the screen.
    t.setTextSize(Theme::uiTextSize(t, 1));
    const bool fits = t.textWidth(full) <= w - 8;
    t.setTextSize(1);
    return fits ? full : "UNWATCH";
}

// ---- design studies (2026-09-27) ------------------------------------------
// Three candidate looks for this screen, picked between on a review page.
// Style 0 is the screen as it shipped; the others are drawn from the same
// facts -- the label, the type the log knows it as, the signal and its trend.
// The emulator picks one with SQUACHSIM_WATCHSTYLE; the device stays on 0
// until one is chosen, and the losers get deleted.
static uint8_t s_style = 0;
void uiWatchAlertSetStyle(uint8_t s) { s_style = s; }

namespace {

struct WatchView {
    const char*   label;
    DetectionType type;       // UNKNOWN when the log has no row for it
    bool          haveRssi;
    int8_t        rssi;
    float         f;          // signal as 0..1, -100 dBm to -30 dBm
    int8_t        trend;      // +1 closer, -1 further, 0 holding
};

WatchView gather(const DetectionEngine& eng) {
    WatchView v{};
    v.label = eng.watchLabel();
    v.type  = DetectionType::UNKNOWN;
    for (uint8_t i = 0; i < eng.logCount(); i++) {
        const Detection* d = eng.logAt(i);
        if (d && (eng.isWatched(d->mac, true) || eng.isWatched(d->mac, false))) {
            v.type = d->type;
            if (!v.label || !v.label[0]) v.label = d->name;
            break;
        }
    }
    if (!v.label || !v.label[0]) v.label = "UNKNOWN DEVICE";
    const uint8_t n = eng.watchRssiCount();
    if (n > 0) {
        v.haveRssi = true;
        v.rssi = eng.watchRssiAt(n - 1);
        int r = v.rssi; if (r < -100) r = -100; if (r > -30) r = -30;
        v.f = (float)(r + 100) / 70.0f;
        if (n >= 4) {
            const int before = (eng.watchRssiAt(n - 2) + eng.watchRssiAt(n - 3) + eng.watchRssiAt(n - 4)) / 3;
            if (v.rssi - before >= 3) v.trend = 1;
            else if (before - v.rssi >= 3) v.trend = -1;
        }
    }
    return v;
}

// The label, cut to fit w at the given size with a trailing "..".
void fitPrint(TFT_eSPI& t, int x, int y, const char* s, int w, uint8_t size, uint16_t col, bool centre) {
    char buf[28];
    snprintf(buf, sizeof buf, "%s", s);
    t.setTextSize(size);
    while (strlen(buf) > 3 && t.textWidth(buf) > w) {
        const size_t n = strlen(buf);
        buf[n - 3] = '.'; buf[n - 2] = '.'; buf[n - 1] = 0;
    }
    t.setTextColor(col);
    const int tw = t.textWidth(buf);
    t.setCursor(centre ? x + (w - tw) / 2 : x, y);
    t.print(buf);
}

// Ten blocks, cold on the left and hot on the right: how close, at a glance.
void meter(TFT_eSPI& t, int x, int y, int w, int h, float f, uint16_t off) {
    const int N = 10, gap = 2;
    const int bw = (w - gap * (N - 1)) / N;
    const int lit = (int)(f * N + 0.5f);
    for (int i = 0; i < N; i++) {
        uint16_t c = off;
        if (i < lit) {
            if (i < 4)      c = t.color565(0, 219, 0);
            else if (i < 7) c = t.color565(255, 219, 0);
            else            c = t.color565(255, 36, 0);
        }
        t.fillRect(x + i * (bw + gap), y, bw, h, c);
    }
}

void trendLine(TFT_eSPI& t, int x, int y, const WatchView& v, uint16_t col, bool centreIn, int w) {
    char buf[32];
    const char* word = v.trend > 0 ? "CLOSER" : v.trend < 0 ? "FURTHER" : "HOLDING";
    snprintf(buf, sizeof buf, "%d dBm  %s", (int)v.rssi, word);
    t.setTextSize(1);
    t.setTextColor(col);
    const int tw = t.textWidth(buf) + 10;
    const int x0 = centreIn ? x + (w - tw) / 2 : x;
    t.setCursor(x0 + 10, y);
    t.print(buf);
    const uint16_t ac = v.trend > 0 ? t.color565(255, 36, 0) : v.trend < 0 ? t.color565(0, 219, 0) : col;
    if (v.trend > 0)      t.fillTriangle(x0, y + 7, x0 + 6, y + 7, x0 + 3, y, ac);
    else if (v.trend < 0) t.fillTriangle(x0, y, x0 + 6, y, x0 + 3, y + 7, ac);
    else                  t.fillRect(x0, y + 3, 7, 2, ac);
}

void spark(TFT_eSPI& t, const DetectionEngine& eng, int gx, int gy, int gw, int gh, uint16_t col, uint16_t dot) {
    const uint8_t n = eng.watchRssiCount();
    if (n < 2) return;
    auto mapY = [&](int v) {
        if (v < -100) v = -100; if (v > -30) v = -30;
        return gy + gh - ((v + 100) * gh) / 70;
    };
    int px = gx, py = mapY(eng.watchRssiAt(0));
    for (uint8_t i = 1; i < n; i++) {
        const int x = gx + (int)((uint32_t)i * gw / (n - 1)), y = mapY(eng.watchRssiAt(i));
        t.drawLine(px, py, x, y, col);
        t.drawLine(px, py + 1, x, y + 1, col);
        px = x; py = y;
    }
    t.fillCircle(px, py, 2, dot);
}

void headline(TFT_eSPI& t, int cx, int y, const char* msg, uint16_t col) {
    const int tw = Theme::bangersTextWidth(msg, Theme::BangersSize::MD);
    Theme::drawBangersOutline(t, cx - tw / 2, y, msg, Theme::BLACK, Theme::BangersSize::MD, 2);
    Theme::drawBangersText(t, cx - tw / 2, y, msg, col, Theme::BangersSize::MD);
}

// ---- 1: RED ALERT ------------------------------------------------------------
// An alarm, not a mood: black ground, a hazard stripe crawling along the top,
// a border that pulses, the headline flashing hard. The device gets a card
// with its own icon, then its name, the signal as a hot-and-cold meter and
// which way it is going.
void drawRedAlert(TFT_eSPI& t, uint32_t now, const DetectionEngine& eng, int hintY) {
    const int w = t.width(), h = t.height();
    const WatchView v = gather(eng);
    const uint16_t red = t.color565(255, 36, 0), redDk = t.color565(109, 0, 0);
    t.fillRect(0, 0, w, h, Theme::BLACK);
    // The stripe: parallelograms scrolling left.
    const int SH = 12, P = 18;
    const int off = (int)((now / 40) % P);
    for (int x = -P * 2 - off; x < w + P; x += P) {
        t.fillTriangle(x, SH, x + P / 2, 0, x + P, 0, red);
        t.fillTriangle(x, SH, x + P, 0, x + P / 2, SH, red);
    }
    t.drawFastHLine(0, SH, w, redDk);
    // The border pulses.
    const float pulse = 0.5f + 0.5f * sinf((float)(now % 900) / 900.0f * 6.2831853f);
    const uint16_t bc = Theme::blend(redDk, red, (uint16_t)(pulse * 255.0f));
    for (int k = 0; k < 3; k++) t.drawRect(k, SH + 1 + k, w - 2 * k, h - SH - 1 - 2 * k, bc);
    // The headline flashes, hard -- an alarm does not fade.
    const bool on = ((now / 350) & 1u) == 0;
    headline(t, w / 2, SH + 7, "BACK IN RANGE", on ? Theme::WHITE : red);

    const bool wide = w >= 300;
    const int top = SH + 36;
    const int card = wide ? 84 : 60;
    const int cardX = wide ? 14 : (w - card) / 2, cardY = top;
    t.fillRoundRect(cardX - 2, cardY - 2, card + 4, card + 4, 8, bc);
    t.fillRoundRect(cardX, cardY, card, card, 6, redDk);
    if (v.type != DetectionType::UNKNOWN)
        Theme::drawTypeIcon(t, v.type, cardX + card / 2, cardY + card / 2, card / 4);
    else
        headline(t, cardX + card / 2, cardY + card / 2 - 10, "!", Theme::WHITE);

    int tx, ty, tw;
    if (wide) { tx = cardX + card + 14; ty = top + 2; tw = w - tx - 12; }
    else      { tx = 12; ty = cardY + card + 8; tw = w - 24; }
    fitPrint(t, tx, ty, v.label, tw, 2, Theme::WHITE, !wide);
    ty += 20;
    t.setTextSize(1);
    fitPrint(t, tx, ty, v.type != DetectionType::UNKNOWN ? detectionTypeName(v.type) : "ON YOUR WATCH LIST",
             tw, 1, t.color565(255, 146, 85), !wide);
    ty += 14;
    if (v.haveRssi) {
        meter(t, tx, ty, tw, wide ? 12 : 9, v.f, t.color565(36, 36, 85));
        ty += (wide ? 12 : 9) + 6;
        trendLine(t, tx, ty, v, Theme::WHITE, !wide, tw);
        ty += 14;
        if (ty + 22 < hintY - 4) spark(t, eng, tx, ty, tw, 20, t.color565(146, 146, 170), red);
    }
}

// ---- 2: WANTED ---------------------------------------------------------------
// A wanted poster pinned up on the red wash, the device's icon as the
// mugshot, and Squachy beside it pointing it out -- he is a snitch by hobby.
void drawWanted(TFT_eSPI& t, uint32_t now, const DetectionEngine& eng, int hintY, bool advance) {
    const int w = t.width(), h = t.height();
    const WatchView v = gather(eng);
    const float pulse = 0.5f + 0.5f * sinf((float)(now % 1400) / 1400.0f * 6.2831853f);
    const uint16_t bg = Theme::blend(Theme::BLACK, Theme::RED, (uint16_t)(pulse * 90.0f));
    t.fillRect(0, 0, w, h, bg);

    const bool wide = w >= 300;
    const int pw = wide ? (w * 50) / 100 : w - 60;
    // On a wide screen the poster starts lower, under the row his speech
    // bubble uses: the bubble is wider than the gap beside the poster.
    const int px = wide ? 14 : 30, py = wide ? 26 : 12;
    const int ph = hintY - 8 - py;

    const uint16_t paper = t.color565(255, 219, 146), paperDk = t.color565(219, 182, 109);
    const uint16_t ink = t.color565(73, 36, 0);
    t.fillRect(px - 2, py - 2, pw + 4, ph + 4, Theme::BLACK);
    t.fillRect(px, py, pw, ph, paper);
    // A curled corner, bottom right.
    t.fillTriangle(px + pw - 14, py + ph, px + pw, py + ph, px + pw, py + ph - 14, bg);
    t.fillTriangle(px + pw - 14, py + ph, px + pw, py + ph - 14, px + pw - 14, py + ph - 14, paperDk);
    t.drawLine(px + pw - 14, py + ph, px + pw, py + ph - 14, Theme::BLACK);
    // The pin.
    t.fillCircle(px + pw / 2, py + 4, 5, Theme::BLACK);
    t.fillCircle(px + pw / 2, py + 4, 4, t.color565(219, 0, 0));
    t.fillCircle(px + pw / 2 - 1, py + 3, 1, Theme::WHITE);

    const int cx = px + pw / 2;
    const int wt = Theme::bangersTextWidth("WANTED", Theme::BangersSize::MD);
    Theme::drawBangersText(t, cx - wt / 2, py + 12, "WANTED", ink, Theme::BangersSize::MD);
    // The mugshot.
    const int ms = (ph - 110 > 36) ? ((ph - 110 < 70) ? ph - 110 : 70) : 36;
    const int mx = cx - ms / 2, my = py + 40;
    t.fillRect(mx - 2, my - 2, ms + 4, ms + 4, ink);
    t.fillRect(mx, my, ms, ms, t.color565(36, 36, 85));
    for (int k = 1; k < 4; k++) t.drawFastHLine(mx, my + k * ms / 4, ms, t.color565(73, 73, 170));
    if (v.type != DetectionType::UNKNOWN)
        Theme::drawTypeIcon(t, v.type, cx, my + ms / 2, ms / 4);
    // The stamp across the mugshot's corner.
    {
        t.setTextSize(1);
        const char* st = "SPOTTED";
        const int sw = t.textWidth(st) + 8;
        const int sx = mx + ms - sw + 10, sy = my + ms - 8;
        const uint16_t stc = t.color565(219, 0, 0);
        t.fillRect(sx, sy, sw, 13, paper);
        t.drawRect(sx, sy, sw, 13, stc);
        t.drawRect(sx + 1, sy + 1, sw - 2, 11, stc);
        t.setTextColor(stc);
        t.setCursor(sx + 4, sy + 3);
        t.print(st);
    }
    int y = my + ms + 10;
    fitPrint(t, px + 6, y, v.label, pw - 12, 2, ink, true);
    y += 20;
    fitPrint(t, px + 6, y, "LAST SEEN: JUST NOW", pw - 12, 1, ink, true);
    y += 14;
    if (v.haveRssi && y + 16 < py + ph) {
        // Five bars, like a phone's, filled as the signal climbs.
        const int bars = 5, bw = 7, gap = 3;
        const int total = bars * bw + (bars - 1) * gap;
        t.setTextSize(1);
        t.setTextColor(ink);
        const int lw = t.textWidth("SIGNAL ");
        int bx = cx - (total + lw) / 2;
        t.setCursor(bx, y + 6);
        t.print("SIGNAL ");
        bx += lw;
        const int lit = (int)(v.f * bars + 0.5f);
        for (int i = 0; i < bars; i++) {
            const int bh = 4 + i * 3;
            const int yy = y + 14 - bh;
            if (i < lit) t.fillRect(bx, yy, bw, bh, t.color565(219, 0, 0));
            else         t.drawRect(bx, yy, bw, bh, ink);
            bx += bw + gap;
        }
    }
    // Squachy last, beside the poster, so his speech bubble lands on top of
    // it rather than under it. Narrow screens give the poster the room.
    if (wide) Squachy::tick(t, px + pw + (w - px - pw) / 2, 2, hintY - 12, now, advance);
}

// ---- LOCKED ON, five ways (2026-09-27) --------------------------------------
// The radar won the first round. These are variations on it: 3 SCOPE (the
// round-one radar with the approach drawn in), 4 SONAR, 5 HUD, 6 BULLSEYE and
// 7 OPERATOR (the scope with Squachy on the headphones).

// Where the watched target has been, as signal fractions, oldest first.
int historyF(const DetectionEngine& eng, float* out, int maxN) {
    const uint8_t n = eng.watchRssiCount();
    const int from = n > maxN ? n - maxN : 0;
    int k = 0;
    for (int i = from; i < n; i++) {
        int r = eng.watchRssiAt((uint8_t)i); if (r < -100) r = -100; if (r > -30) r = -30;
        out[k++] = (float)(r + 100) / 70.0f;
    }
    return k;
}

float labelAngle(const char* label) {
    uint32_t hs = 2166136261u;
    for (const char* p = label; *p; p++) { hs ^= (uint8_t)*p; hs *= 16777619u; }
    return (float)(hs % 628) / 100.0f;
}

// The text column every variant shares: headline, name, type, trend, line.
void lockedText(TFT_eSPI& t, const DetectionEngine& eng, const WatchView& v, int tx, int ty, int tw,
                bool centre, int hintY, uint16_t head, uint16_t name, uint16_t dim, uint16_t line,
                const char* word = "LOCKED ON") {
    headline(t, tx + tw / 2, ty, word, head);
    ty += 30;
    fitPrint(t, tx, ty, v.label, tw, 2, name, centre);
    ty += 20;
    fitPrint(t, tx, ty, v.type != DetectionType::UNKNOWN ? detectionTypeName(v.type) : "WATCH LIST", tw, 1, dim, centre);
    ty += 14;
    if (v.haveRssi) {
        trendLine(t, tx, ty, v, name, centre, tw);
        ty += 14;
        if (ty + 26 < hintY) spark(t, eng, tx, ty + 4, tw, 22, dim, head);
    }
}

// A round scope with rings, a trailed sweep and the target's recent
// positions as a path closing in on the middle. Shared by SCOPE and OPERATOR.
void scope(TFT_eSPI& t, uint32_t now, const DetectionEngine& eng, const WatchView& v,
           int rcx, int rcy, int R, bool tag) {
    const uint16_t g1 = t.color565(0, 73, 0), g2 = t.color565(0, 146, 0), g3 = t.color565(0, 255, 0);
    const float TAU = 6.2831853f;
    t.fillCircle(rcx, rcy, R, Theme::BLACK);
    const float a = (float)(now % 2400) / 2400.0f * TAU;
    for (int k = 5; k >= 1; k--) {
        const float a0 = a - (float)k * 0.09f, a1 = a - (float)(k - 1) * 0.09f;
        t.fillTriangle(rcx, rcy, rcx + (int)(cosf(a0) * R), rcy + (int)(sinf(a0) * R),
                       rcx + (int)(cosf(a1) * R), rcy + (int)(sinf(a1) * R), k <= 1 ? g2 : g1);
    }
    for (int k = 1; k <= 3; k++) t.drawCircle(rcx, rcy, R * k / 3, g2);
    t.drawCircle(rcx, rcy, R + 1, g3);
    t.drawFastHLine(rcx - R, rcy, 2 * R, g1);
    t.drawFastVLine(rcx, rcy - R, 2 * R, g1);
    t.drawLine(rcx, rcy, rcx + (int)(cosf(a) * R), rcy + (int)(sinf(a) * R), g3);

    const float b = labelAngle(v.label);
    auto radius = [&](float f) { return (float)R * (0.12f + 0.78f * (1.0f - f)); };
    // The approach: where it was on each of the last few readings.
    float hf[8];
    const int hn = historyF(eng, hf, 8);
    for (int i = 0; i + 1 < hn; i++) {
        const float rr = radius(hf[i]);
        t.fillCircle(rcx + (int)(cosf(b) * rr), rcy + (int)(sinf(b) * rr), i >= hn - 3 ? 2 : 1, g2);
    }
    const float rr = radius(v.haveRssi ? v.f : 0.3f);
    const int bx = rcx + (int)(cosf(b) * rr), by = rcy + (int)(sinf(b) * rr);
    float since = a - b; while (since < 0) since += TAU;
    const float fl = since < 1.2f ? 1.0f - since / 1.2f : 0.0f;
    t.fillCircle(bx, by, 3 + (int)(fl * 3.0f), Theme::blend(g2, Theme::WHITE, (uint16_t)(fl * 255.0f)));
    const int s = 10 + (int)(3.0f * sinf((float)(now % 800) / 800.0f * TAU));
    const uint16_t lc = ((now / 250) & 1u) ? t.color565(255, 36, 0) : t.color565(255, 219, 0);
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sy = -1; sy <= 1; sy += 2) {
            const int cx = bx + sx * s, cy = by + sy * s;
            t.drawFastHLine(sx < 0 ? cx : cx - 4, cy, 5, lc);
            t.drawFastVLine(cx, sy < 0 ? cy : cy - 4, 5, lc);
        }
    // A tag on a leader line: what it is, pinned to where it is.
    if (tag && v.type != DetectionType::UNKNOWN) {
        const char* nm = detectionTypeName(v.type);
        t.setTextSize(1);
        const int lw = t.textWidth(nm) + 6;
        const int dir = (cosf(b) > 0) ? -1 : 1;              // point the tag back toward the middle
        const int lx = bx + dir * (s + 10), ly = by - s - 10;
        t.drawLine(bx + dir * s, by - s, lx, ly + 5, lc);
        const int rx = dir > 0 ? lx : lx - lw;
        t.fillRect(rx, ly, lw, 11, Theme::BLACK);
        t.drawRect(rx, ly, lw, 11, g3);
        t.setTextColor(g3);
        t.setCursor(rx + 3, ly + 2);
        t.print(nm);
    }
}

// ---- 3: SCOPE -------------------------------------------------------------------
void drawScope(TFT_eSPI& t, uint32_t now, const DetectionEngine& eng, int hintY) {
    const int w = t.width(), h = t.height();
    const WatchView v = gather(eng);
    const uint16_t g0 = t.color565(0, 36, 0), g1 = t.color565(0, 73, 0), g2 = t.color565(0, 146, 0),
                   g3 = t.color565(0, 255, 0);
    t.fillRect(0, 0, w, h, g0);
    for (int x = 0; x < w; x += 20) t.drawFastVLine(x, 0, h, g1);
    for (int y = 0; y < h; y += 20) t.drawFastHLine(0, y, w, g1);
    const bool wide = w >= 300;
    const int R = wide ? (hintY - 20) / 2 : (hintY - 100) / 2;
    const int rcx = wide ? 14 + R : w / 2, rcy = wide ? 10 + R : 42 + R;
    scope(t, now, eng, v, rcx, rcy, R, true);
    if (wide) lockedText(t, eng, v, rcx + R + 14, 16, w - rcx - R - 24, false, hintY, g3,
                         t.color565(182, 255, 170), g2, g2);
    else {
        headline(t, w / 2, 8, "LOCKED ON", g3);
        int ty = rcy + R + 8;
        fitPrint(t, 10, ty, v.label, w - 20, 2, t.color565(182, 255, 170), true);
        if (v.haveRssi) trendLine(t, 10, ty + 20, v, g3, true, w - 20);
    }
}

// ---- 4: SONAR -------------------------------------------------------------------
// A submarine's fan, rising from the bottom of the screen: a ping goes out
// every couple of seconds, and the target only shows when the ping reaches it,
// then fades until the next one. Deep blue and cyan instead of green.
void drawSonar(TFT_eSPI& t, uint32_t now, const DetectionEngine& eng, int hintY) {
    const int w = t.width(), h = t.height();
    const WatchView v = gather(eng);
    const uint16_t n0 = t.color565(0, 0, 85), n1 = t.color565(0, 36, 170), n2 = t.color565(0, 109, 170),
                   c3 = t.color565(73, 255, 255);
    t.fillRect(0, 0, w, h, Theme::BLACK);
    // Centred along the bottom on every screen, with the words above it: beside
    // it, the text column on a 2.8" was too narrow for the headline.
    const int cx = w / 2;
    const int cy = hintY - 10;
    const int top = 76;
    int R = (cy - top < w / 2 - 8) ? cy - top : w / 2 - 8;
    if (R < 40) R = 40;
    const float PI_ = 3.14159265f;
    // The fan: filled, then its rings and spokes.
    for (int dy = 0; dy <= R; dy++) {
        const int half = (int)sqrtf((float)(R * R - dy * dy));
        t.drawFastHLine(cx - half, cy - dy, 2 * half + 1, n0);
    }
    auto arc = [&](int r, uint16_t c, int thick) {
        const int steps = r * 3;
        for (int i = 0; i <= steps; i++) {
            const float a = PI_ + PI_ * (float)i / (float)steps;
            for (int k = 0; k < thick; k++)
                t.drawPixel(cx + (int)(cosf(a) * (r - k)), cy + (int)(sinf(a) * (r - k)), c);
        }
    };
    for (int k = 1; k <= 4; k++) arc(R * k / 4, n1, 1);
    for (int k = 1; k < 6; k++) {
        const float a = PI_ + PI_ * (float)k / 6.0f;
        t.drawLine(cx, cy, cx + (int)(cosf(a) * R), cy + (int)(sinf(a) * R), n1);
    }
    t.drawFastHLine(cx - R, cy, 2 * R + 1, n2);
    arc(R, n2, 2);
    // The ping.
    const uint32_t P = 2000, T = 1400;
    const uint32_t pp = now % P;
    const float b = PI_ + 0.15f * PI_ + fmodf(labelAngle(v.label), 0.7f * PI_);
    const float br = (float)R * (0.14f + 0.8f * (1.0f - (v.haveRssi ? v.f : 0.3f)));
    if (pp < T) {
        const int pr = (int)((float)R * (float)pp / (float)T);
        arc(pr, pp < T / 2 ? c3 : n2, 2);
    }
    // The blip lights as the ping crosses it and fades until the next.
    const uint32_t hitAt = (uint32_t)(br / (float)R * (float)T);
    const int32_t age = (int32_t)pp - (int32_t)hitAt;
    const float lit = age >= 0 ? 1.0f - (float)age / (float)(P - hitAt) : 0.25f;
    const int bx = cx + (int)(cosf(b) * br), by = cy + (int)(sinf(b) * br);
    t.fillCircle(bx, by, 3 + (age >= 0 && age < 300 ? 2 : 0), Theme::blend(n1, Theme::WHITE, (uint16_t)(lit * 255.0f)));
    if (age >= 0 && age < 500) {
        t.setTextSize(1);
        t.setTextColor(c3);
        t.setCursor(bx + 7, by - 12);
        t.print("PING");
    }
    int ty = cy - R - 76;
    if (ty < 6) ty = 6;
    headline(t, w / 2, ty, "LOCKED ON", c3);
    fitPrint(t, 10, ty + 30, v.label, w - 20, 2, Theme::WHITE, true);
    if (v.haveRssi) trendLine(t, 10, ty + 50, v, c3, true, w - 20);
}

// ---- 5: HUD ----------------------------------------------------------------------
// A targeting display: the device's own icon in the middle of a reticle, and
// four brackets that close in on it as the signal climbs. Amber on black,
// with scanlines, a range tape down one side and readouts down the other.
void drawHud(TFT_eSPI& t, uint32_t now, const DetectionEngine& eng, int hintY) {
    const int w = t.width(), h = t.height();
    const WatchView v = gather(eng);
    const uint16_t a1 = t.color565(109, 73, 0), a2 = t.color565(182, 146, 0), a3 = t.color565(255, 219, 0);
    t.fillRect(0, 0, w, h, Theme::BLACK);
    for (int y = 0; y < h; y += 3) t.drawFastHLine(0, y, w, t.color565(36, 36, 0));
    const bool wide = w >= 300;
    headline(t, w / 2, 6, "LOCKED ON", a3);
    const int cx = wide ? w * 42 / 100 : w / 2;
    const int cy = wide ? (36 + hintY) / 2 : 36 + (hintY - 36 - 44) / 2;
    const int Rmax = wide ? (hintY - 36) / 2 - 6 : (hintY - 36 - 44) / 2 - 4;
    // The reticle.
    t.drawCircle(cx, cy, Rmax * 55 / 100, a1);
    t.drawCircle(cx, cy, Rmax * 55 / 100 + 1, a1);
    for (int s = -1; s <= 1; s += 2) {
        t.drawFastHLine(s < 0 ? cx - Rmax : cx + Rmax * 30 / 100, cy, Rmax * 70 / 100, a1);
        t.drawFastVLine(cx, s < 0 ? cy - Rmax : cy + Rmax * 30 / 100, Rmax * 70 / 100, a1);
    }
    if (v.type != DetectionType::UNKNOWN) Theme::drawTypeIcon(t, v.type, cx, cy, Rmax / 5);
    // The brackets: far out when faint, hugging the icon when close, and
    // breathing a little so they read as live.
    const float TAU = 6.2831853f;
    const float f = v.haveRssi ? v.f : 0.2f;
    const int d = (int)((float)Rmax * (1.0f - 0.55f * f)) + (int)(2.0f * sinf((float)(now % 700) / 700.0f * TAU));
    const int L = Rmax / 4;
    const uint16_t bc = ((now / 300) & 1u) ? a3 : t.color565(255, 109, 0);
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sy = -1; sy <= 1; sy += 2) {
            const int x = cx + sx * d, y = cy + sy * d;
            for (int k = 0; k < 2; k++) {
                t.drawFastHLine(sx < 0 ? x : x - L, y + sy * -k, L + 1, bc);
                t.drawFastVLine(x + sx * -k, sy < 0 ? y : y - L, L + 1, bc);
            }
        }
    // The range tape, down the left.
    const int tx = wide ? 12 : 6, tyT = cy - Rmax, tyB = cy + Rmax;
    t.drawFastVLine(tx + 8, tyT, tyB - tyT, a2);
    for (int k = 0; k <= 10; k++) {
        const int y = tyT + (tyB - tyT) * k / 10;
        t.drawFastHLine(tx + (k % 5 == 0 ? 2 : 5), y, k % 5 == 0 ? 6 : 3, a2);
    }
    const int py = tyB - (int)((float)(tyB - tyT) * f);
    t.fillTriangle(tx + 10, py, tx + 16, py - 4, tx + 16, py + 4, a3);
    t.setTextSize(1);
    t.setTextColor(a2);
    t.setCursor(tx, tyT - 10); t.print("NEAR");
    t.setCursor(tx, tyB + 3);  t.print("FAR");
    // Readouts.
    if (wide) {
        const int rx = cx + Rmax + 16, rw = w - rx - 8;
        int ry = 44;
        fitPrint(t, rx, ry, v.label, rw, 2, a3, false); ry += 22;
        fitPrint(t, rx, ry, v.type != DetectionType::UNKNOWN ? detectionTypeName(v.type) : "WATCH LIST", rw, 1, a2, false); ry += 16;
        char buf[24];
        if (v.haveRssi) {
            snprintf(buf, sizeof buf, "SIG %d dBm", (int)v.rssi);
            fitPrint(t, rx, ry, buf, rw, 1, a3, false); ry += 12;
            fitPrint(t, rx, ry, v.trend > 0 ? "VEC CLOSING" : v.trend < 0 ? "VEC OPENING" : "VEC HOLDING", rw, 1, a3, false); ry += 12;
            const char* rng = f > 0.8f ? "RNG POINT BLANK" : f > 0.55f ? "RNG CLOSE" : f > 0.3f ? "RNG MEDIUM" : "RNG LONG";
            fitPrint(t, rx, ry, rng, rw, 1, a3, false); ry += 16;
            if (ry + 24 < hintY) spark(t, eng, rx, ry, rw, 22, a1, a3);
        }
    } else {
        fitPrint(t, 10, cy + Rmax + 6, v.label, w - 20, 2, a3, true);
        if (v.haveRssi) trendLine(t, 10, cy + Rmax + 26, v, a3, true, w - 20);
    }
}

// ---- 6: BULLSEYE ---------------------------------------------------------------
// Hot and cold as rings: blue outside, red in the middle, the target sitting
// on the ring its signal puts it in, and a white ring drawing in toward the
// centre over and over. The word under it says it plainly.
void drawBullseye(TFT_eSPI& t, uint32_t now, const DetectionEngine& eng, int hintY) {
    const int w = t.width(), h = t.height();
    const WatchView v = gather(eng);
    t.fillRect(0, 0, w, h, Theme::BLACK);
    const bool wide = w >= 300;
    const int R = wide ? (hintY - 20) / 2 : (hintY - 104) / 2;
    const int rcx = wide ? 14 + R : w / 2, rcy = wide ? 10 + R : 40 + R;
    static const uint8_t RC[5][3] = { {0, 36, 170}, {0, 146, 170}, {255, 219, 0}, {255, 109, 0}, {255, 0, 0} };
    for (int k = 0; k < 5; k++) {
        const int r = R - k * R / 5;
        t.fillCircle(rcx, rcy, r, Theme::BLACK);
        t.fillCircle(rcx, rcy, r - 2, t.color565(RC[k][0], RC[k][1], RC[k][2]));
    }
    // The closing ring.
    const uint32_t P = 1600;
    const int cr = R - (int)((float)R * (float)(now % P) / (float)P);
    if (cr > 2) { t.drawCircle(rcx, rcy, cr, Theme::WHITE); t.drawCircle(rcx, rcy, cr - 1, Theme::WHITE); }
    const float f = v.haveRssi ? v.f : 0.2f;
    const float b = labelAngle(v.label);
    const float rr = (float)R * (0.08f + 0.86f * (1.0f - f));
    const int bx = rcx + (int)(cosf(b) * rr), by = rcy + (int)(sinf(b) * rr);
    t.fillCircle(bx, by, 6, Theme::BLACK);
    t.fillCircle(bx, by, 4, Theme::WHITE);
    const char* word = f > 0.8f ? "RIGHT HERE" : f > 0.55f ? "HOT" : f > 0.3f ? "WARM" : "COLD";
    const uint16_t wc = f > 0.8f ? t.color565(255, 0, 0) : f > 0.55f ? t.color565(255, 109, 0)
                      : f > 0.3f ? t.color565(255, 219, 0) : t.color565(0, 146, 170);
    if (wide) {
        const int tx = rcx + R + 14, tw = w - tx - 10;
        headline(t, tx + tw / 2, 14, "LOCKED ON", Theme::WHITE);
        headline(t, tx + tw / 2, 44, word, wc);
        int ty = 76;
        fitPrint(t, tx, ty, v.label, tw, 2, Theme::WHITE, true); ty += 20;
        fitPrint(t, tx, ty, v.type != DetectionType::UNKNOWN ? detectionTypeName(v.type) : "WATCH LIST", tw, 1, t.color565(146, 146, 170), true); ty += 14;
        if (v.haveRssi) { trendLine(t, tx, ty, v, Theme::WHITE, true, tw); ty += 14; }
        if (ty + 26 < hintY) spark(t, eng, tx, ty + 4, tw, 22, t.color565(146, 146, 170), wc);
    } else {
        headline(t, w / 2, 6, "LOCKED ON", Theme::WHITE);
        headline(t, w / 2, rcy + R + 4, word, wc);
        fitPrint(t, 10, rcy + R + 32, v.label, w - 20, 1, Theme::WHITE, true);
    }
}

// ---- 7: OPERATOR ---------------------------------------------------------------
// The scope, with Squachy beside it on the headphones: the one version of this
// screen where he is still in the room.
void drawOperator(TFT_eSPI& t, uint32_t now, const DetectionEngine& eng, int hintY, bool advance) {
    const int w = t.width(), h = t.height();
    const WatchView v = gather(eng);
    const uint16_t g0 = t.color565(0, 36, 0), g1 = t.color565(0, 73, 0), g3 = t.color565(0, 255, 0);
    t.fillRect(0, 0, w, h, g0);
    for (int x = 0; x < w; x += 20) t.drawFastVLine(x, 0, h, g1);
    for (int y = 0; y < h; y += 20) t.drawFastHLine(0, y, w, g1);
    const bool wide = w >= 300;
    // On a narrow screen the scope gives up width so he still fits beside it.
    int R = wide ? (hintY - 46) / 2 : (hintY - 60) / 2 - 10;
    if (!wide && R > w * 28 / 100) R = w * 28 / 100;
    const int rcx = wide ? 12 + R : 8 + R, rcy = 40 + R;
    scope(t, now, eng, v, rcx, rcy, R, wide);
    // Squachy in the space to the right of the scope, headphones on. His
    // region starts well down the screen: his speech bubble sits above it
    // and is wider than the gap, and up top it covered the headline.
    const int sx = rcx + R + (w - rcx - R) / 2;
    const int sTop = wide ? 64 : 70;
    // Sized to the gap beside the scope as well as the height under it: he
    // is about two thirds as wide as he is tall.
    int avail = hintY - 4 - sTop;
    const int byWidth = (w - (rcx + R)) * 13 / 10;
    if (avail > byWidth) avail = byWidth;
    Squachy::tick(t, sx, sTop, avail, now, advance, 0.4f);
    headline(t, rcx, 6, "LOCKED ON", g3);
    const int crown = Squachy::crownY();
    const float sc = Squachy::lastScale();
    auto S = [sc](float u) { return (int)(u * sc); };
    const uint16_t band = t.color565(182, 182, 170), cup = t.color565(73, 73, 85);
    for (int k = 0; k < 2; k++) {
        const int r = S(17) - k;
        for (int i = 0; i <= 90; i++) {
            const float a = 3.14159265f + 3.14159265f * (float)i / 90.0f;
            t.drawPixel(sx + (int)(cosf(a) * r), crown + S(11) + (int)(sinf(a) * (float)r * 0.9f), band);
        }
    }
    for (int sg = -1; sg <= 1; sg += 2) {
        t.fillRoundRect(sx + sg * S(16) - S(3), crown + S(9), S(6), S(9), S(2), Theme::BLACK);
        t.fillRoundRect(sx + sg * S(16) - S(3) + 1, crown + S(9) + 1, S(6) - 2, S(9) - 2, S(2), cup);
    }
    // The mic boom off his left cup, round to his mouth.
    t.drawLine(sx - S(16), crown + S(16), sx - S(8), crown + S(21), band);
    t.fillCircle(sx - S(7), crown + S(21), S(1) + 1, Theme::BLACK);
    // Name and trend under the scope.
    const int ty = rcy + R + 6;
    if (ty + 18 < hintY) {
        fitPrint(t, rcx - R, ty, v.label, 2 * R, 1, t.color565(182, 255, 170), true);
        if (v.haveRssi && ty + 30 < hintY) trendLine(t, rcx - R, ty + 12, v, g3, true, 2 * R);
    }
}

}  // namespace

void uiWatchAlertInit(TFT_eSPI& t) {
    t.fillRect(0, 0, t.width(), t.height(), Theme::BLACK);
    Squachy::watchAlertReaction();
}

void uiWatchAlertTick(TFT_eSPI& t, uint32_t now, const DetectionEngine& eng, bool advance) {
    int w = t.width();
    int h = t.height();

    if (s_style != 0) {
        const int hintY = h - REMOVE_H - 10 - 8 - 6;
        switch (s_style) {
            case 1:  drawRedAlert(t, now, eng, hintY); break;
            case 2:  drawWanted(t, now, eng, hintY, advance); break;
            case 3:  drawScope(t, now, eng, hintY); break;
            case 4:  drawSonar(t, now, eng, hintY); break;
            case 5:  drawHud(t, now, eng, hintY); break;
            case 6:  drawBullseye(t, now, eng, hintY); break;
            default: drawOperator(t, now, eng, hintY, advance); break;
        }
        const char* tapMsg = "tap anywhere to dismiss";
        t.setTextSize(1);
        t.setTextColor(Theme::WHITE);
        t.setCursor((w - t.textWidth(tapMsg)) / 2, hintY);
        t.print(tapMsg);
        int bx, by, bw, bh;
        removeRect(t, bx, by, bw, bh);
        Theme::drawButton(t, bx, by, bw, bh, removeLabel(t, bw), false);
        return;
    }

    // Urgent pulsing wash -- deliberately different from CLEAR's
    // vaporwave background and from the normal ALERT screen's
    // per-type ambient scene, so this reads as its own distinct thing
    // the instant it appears. Kept fairly dark/modest rather than a
    // big dramatic swing -- Squachy's own bubble-erase (see tick()'s
    // header comment) clears its footprint with the app's flat
    // Theme::BG rather than this screen's own color, so a subtler
    // pulse keeps any stray patch from that unlikely to stand out.
    float pulse = 0.5f + 0.5f * sinf((float)(now % 1400) / 1400.0f * 6.2831853f);
    uint16_t bg = Theme::blend(Theme::BLACK, Theme::RED, (uint16_t)(pulse * 90.0f));
    t.fillRect(0, 0, w, h, bg);

    // Squachy runs around behind the text, full-size (not the raw-scan
    // screen's mini cameo) -- same tick() call CLEAR uses, just with
    // the whole screen to himself instead of sharing it with counters
    // and buttons.
    const int topY        = 16;
    // The 40 was the old bottom reserve, back when "tap to dismiss" was the
    // only thing down there. The REMOVE button and the hint above it now own
    // that strip, and Squachy erases his own footprint with flat Theme::BG
    // rather than this screen's pulsing red -- so anything he walks over gets
    // stamped out in the wrong colour. Give him the room above them instead.
    const int availHeight = h - topY - (REMOVE_H + 30);
    Squachy::tick(t, w / 2, topY, availHeight, now, advance);

    // Headline, outlined the same way CLEAR's ALL CLEAR/DETECTIONS
    // LOGGED status text is (2px black outline via an offset grid) --
    // stays legible over him regardless of where he's standing.
    static const int8_t OUTLINE_OFS[24][2] = {
        {-2,-2},{-1,-2},{0,-2},{1,-2},{2,-2},
        {-2,-1},{-1,-1},{0,-1},{1,-1},{2,-1},
        {-2, 0},{-1, 0},        {1, 0},{2, 0},
        {-2, 1},{-1, 1},{0, 1},{1, 1},{2, 1},
        {-2, 2},{-1, 2},{0, 2},{1, 2},{2, 2},
    };
    const char* msg = "TARGET IN RANGE";
    uint16_t col = Theme::blend(Theme::RED, Theme::WHITE, (uint16_t)(pulse * 120.0f));
    int ty = h / 2 - 20;
    int tw = Theme::bangersTextWidth(msg, Theme::BangersSize::MD);
    if (tw <= w - 8) {
        int tx = (w - tw) / 2;
        // One pass, not twenty-four -- see drawBangersOutline() in theme.cpp.
        Theme::drawBangersOutline(t, tx, ty, msg, Theme::BLACK, Theme::BangersSize::MD, 2);
        Theme::drawBangersText(t, tx, ty, msg, col, Theme::BangersSize::MD);
    } else {
        // Narrowest portrait rotations: same fallback CLEAR's status
        // text uses when the Bangers glyph set won't fit.
        t.setTextSize(2);
        int sw = t.textWidth(msg);
        int sx = (w - sw) / 2, sy = ty;
        t.setTextColor(Theme::BLACK, bg);
        for (uint8_t i = 0; i < 24; i++) {
            t.setCursor(sx + OUTLINE_OFS[i][0], sy + OUTLINE_OFS[i][1]);
            t.print(msg);
        }
        t.setTextColor(col, bg);
        t.setCursor(sx, sy);
        t.print(msg);
    }

    // Sub-line: what's actually being watched, and the dismiss hint --
    // plain text, not outlined (same tier as CLEAR's counter row).
    t.setTextSize(1);
    t.setTextWrap(false);
    t.setTextColor(Theme::WHITE, bg);
    int sw2 = t.textWidth(eng.watchLabel());
    t.setCursor((w - sw2) / 2, ty + 30);
    t.print(eng.watchLabel());

    // Signal-strength trend -- lets you tell "getting closer" from
    // "just sitting there" instead of only knowing it's in range at
    // all. Needs at least 2 samples to draw a line; a single fresh
    // watch (or one that's only fired once) just shows the number.
    uint8_t rssiN = eng.watchRssiCount();
    if (rssiN > 0) {
        char rbuf[24];
        snprintf(rbuf, sizeof(rbuf), "%d dBm", (int)eng.watchRssiAt(rssiN - 1));
        int rw = t.textWidth(rbuf);
        int labelY = ty + 44;
        t.setCursor((w - rw) / 2, labelY);
        t.print(rbuf);

        if (rssiN >= 2) {
            // -100..-30 dBm covers "barely there" to "right next to
            // it" for both BLE and WiFi -- clamped rather than
            // auto-scaled so the line's slope means the same thing
            // graph to graph instead of rescaling per-target.
            const int RSSI_LO = -100, RSSI_HI = -30;
            const int graphW = (w - 40 < 140) ? (w - 40) : 140;
            const int graphH = 24;
            const int gx = (w - graphW) / 2;
            const int gy = labelY + 12;

            auto mapY = [&](int8_t rssi) {
                int v = rssi;
                if (v < RSSI_LO) v = RSSI_LO;
                if (v > RSSI_HI) v = RSSI_HI;
                return gy + graphH - ((v - RSSI_LO) * graphH) / (RSSI_HI - RSSI_LO);
            };

            int prevX = gx, prevY = mapY(eng.watchRssiAt(0));
            for (uint8_t i = 1; i < rssiN; i++) {
                int x = gx + (int)((uint32_t)i * graphW / (rssiN - 1));
                int y = mapY(eng.watchRssiAt(i));
                t.drawLine(prevX, prevY, x, y, Theme::WHITE);
                prevX = x;
                prevY = y;
            }
            t.fillCircle(prevX, prevY, 2, col);
        }
    }

    // Two ways out doing different things: anywhere on the screen dismisses
    // the alert and leaves the watch running, the button ends the watch.
    // Drawn last so the button sits over Squachy rather than under him.
    // Sits ABOVE the button with real clearance: size-1 glyphs are 8px tall
    // and draw downward from the cursor, so the old -14 put the text's own
    // rows 4px inside the button and it read as half-erased. The button's top
    // is at h - REMOVE_H - 10, so clear it by the font height plus a gap.
    const char* tapMsg = "tap anywhere to dismiss";
    t.setTextSize(1);
    t.setTextColor(Theme::WHITE, bg);
    int tmw = t.textWidth(tapMsg);
    t.setCursor((w - tmw) / 2, h - REMOVE_H - 10 - 8 - 6);
    t.print(tapMsg);

    int bx, by, bw, bh;
    removeRect(t, bx, by, bw, bh);
    Theme::drawButton(t, bx, by, bw, bh, removeLabel(t, bw), false);
}

bool uiWatchAlertHitRemove(TFT_eSPI& t, int x, int y) {
    int bx, by, bw, bh;
    removeRect(t, bx, by, bw, bh);
    return x >= bx && x <= bx + bw && y >= by && y <= by + bh;
}
