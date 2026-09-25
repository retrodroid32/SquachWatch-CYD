// SquachWatch-CYD — persistent detection timeline
#include "ui_timeline.h"
#include "blackbox.h"
#include "clock.h"
#include "theme.h"
#include "ui_scroll.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

static int g_scroll = 0;
static const int TOP = 16;
static const int ROW_H = 34;
static const uint8_t PAGE_MAX = 12;

void uiTimelineInit(TFT_eSPI& t) {
    g_scroll = 0;
    t.fillRect(0, 0, t.width(), t.height(), Theme::BG);
}

void uiTimelineScroll(int delta) {
    g_scroll += delta;
    if (g_scroll < 0) g_scroll = 0;
}

static void stampFor(const BlackBox::DetRecord& r, char* out, size_t n) {
    if (r.epoch) {
        Clock::formatEpochStamp(r.epoch, out, n);
        return;
    }
    const unsigned long m = (unsigned long)(r.upSec / 60u);
    const unsigned long sec = (unsigned long)(r.upSec % 60u);
    snprintf(out, n, "+%lu:%02lu", m, sec);
}

void uiTimelineTick(TFT_eSPI& t, uint32_t now) {
    (void)now;
    const int w = t.width(), h = t.height();
    t.fillRect(0, 0, w, h, Theme::BG);
    Theme::drawTitleBar(t, ">> TIMELINE <<");
    Theme::drawListHeading(t, "NEWEST FIRST", Theme::CYAN);
    Theme::drawPinnedBack(t, "[ BACK ]");

    const int top = TOP + Theme::LIST_HEADING_H;
    const int bottom = h - Theme::pinnedBackH(w) - 2;
    const int total = (int)BlackBox::detectionsKept();
    const int visible = (bottom - top) / ROW_H;
    uiClampScroll(g_scroll, total, bottom - top, ROW_H);

    if (total <= 0) {
        t.setTextSize(1);
        t.setTextColor(Theme::W95_SHADOW, Theme::BG);
        const char* a = "NO TIMELINE EVENTS YET";
        const char* b = "First sightings and returns appear here.";
        t.setCursor((w - t.textWidth(a)) / 2, top + 34);
        t.print(a);
        t.setCursor((w - t.textWidth(b)) / 2, top + 52);
        t.print(b);
        return;
    }

    BlackBox::DetRecord rec[PAGE_MAX];
    uint16_t want = (uint16_t)visible;
    if (want > PAGE_MAX) want = PAGE_MAX;
    const uint16_t got = BlackBox::readDetections((uint16_t)g_scroll, want, rec);

    int y = top;
    for (uint16_t i = 0; i < got && y + ROW_H <= bottom; i++, y += ROW_H) {
        const BlackBox::DetRecord& r = rec[i];
        const DetectionType ty = r.type < (uint8_t)DetectionType::COUNT
                               ? (DetectionType)r.type : DetectionType::UNKNOWN;
        char stamp[16];
        stampFor(r, stamp, sizeof stamp);

        t.setTextSize(1);
        t.setTextColor(Theme::WHITE, Theme::BG);
        t.setCursor(6, y + 3);
        t.print(stamp);

        const char* tn = detectionTypeDisplayName(ty);
        t.setTextColor(Theme::colorFor(ty), Theme::BG);
        const int rightPad = (r.flags & BlackBox::DET_AGAIN) ? 38 : 5;
        int tx = w - rightPad - t.textWidth(tn);
        if (tx < 56) tx = 56;
        t.setCursor(tx, y + 3);
        t.print(tn);

        if (r.flags & BlackBox::DET_AGAIN) {
            t.setTextColor(Theme::AMBER, Theme::BG);
            t.setCursor(w - 34, y + 3);
            t.print("BACK");
        }

        char who[42];
        const char* src = r.name[0] ? r.name : (r.vendor[0] ? r.vendor : "UNKNOWN");
        snprintf(who, sizeof who, "%s  %d dBm  CH%u", src, (int)r.rssi, (unsigned)r.channel);
        while (who[0] && t.textWidth(who) > w - 12) who[strlen(who) - 1] = 0;
        t.setTextColor(Theme::W95_HILIGHT, Theme::BG);
        t.setCursor(6, y + 19);
        t.print(who);

        t.drawFastHLine(4, y + ROW_H - 1, w - 8, Theme::PURPLE);
    }

    if (total > visible)
        Theme::drawScrollbar(t, w - 3, top, bottom - top, total, visible, g_scroll);

    char count[28];
    snprintf(count, sizeof count, "%d event%s kept", total, total == 1 ? "" : "s");
    t.setTextSize(1);
    t.setTextColor(Theme::W95_SHADOW, Theme::BG);
    t.setCursor(5, bottom - t.fontHeight() - 2);
    t.print(count);
}
