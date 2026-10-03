// SquachWatch-CYD — watched-target alert screen implementation
#include "ui_watchalert.h"
#include "theme.h"
#include "squachy.h"
#include "ui_fit.h"
#include <Arduino.h>

// Shared by the drawing and the hit test so the two cannot drift -- the rule
// every other panel here follows. Full width minus a margin: it is the only
// control on the screen, so there is nothing for it to crowd.
static const int REMOVE_H = 26;
static bool s_alertReactionFired = false;
static uint8_t s_watchOrder[DetectionEngine::WATCH_TARGET_CAP] = {0};
static uint8_t s_watchCount = 0;
static uint8_t s_watchScroll = 0;
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

void uiWatchAlertInit(TFT_eSPI& t) {
    t.fillRect(0, 0, t.width(), t.height(), Theme::BLACK);
    s_alertReactionFired = false;
    s_watchScroll = 0;
}

static void watchListGeom(TFT_eSPI& t, int w, int h, int& top, int& bottom, int& rowH, int& visible) {
    Theme::ButtonBarGeom bar = Theme::computeButtonBar(w, h);
    top = 24;
    bottom = bar.y - 4;
    t.setTextSize(2); const int nameH = t.fontHeight();
    t.setTextSize(1); const int detailH = t.fontHeight();
    rowH = nameH + detailH + 5;
    visible = (bottom - top) / rowH;
    if (visible < 1) visible = 1;
}

static int watchScore(const DetectionEngine::WatchTargetInfo& info, uint32_t now) {
    if (!info.seen || now - info.lastSeenMs > 10000) return -200;
    return (int)info.rssi;
}

static void buildWatchOrder(const DetectionEngine& eng, uint32_t now) {
    s_watchCount = eng.watchTargetCount();
    for (uint8_t i=0;i<s_watchCount;i++) s_watchOrder[i]=i;
    for (uint8_t i=1;i<s_watchCount;i++) {
        const uint8_t key=s_watchOrder[i];
        DetectionEngine::WatchTargetInfo ki; eng.watchTargetInfo(key,ki);
        const int ks=watchScore(ki,now);
        uint8_t j=i;
        while(j>0){
            DetectionEngine::WatchTargetInfo pi; eng.watchTargetInfo(s_watchOrder[j-1],pi);
            if(watchScore(pi,now)>=ks) break;
            s_watchOrder[j]=s_watchOrder[j-1]; j--;
        }
        s_watchOrder[j]=key;
    }
}

static void drawWatchTrend(TFT_eSPI& t,int x,int y,const DetectionEngine::WatchTargetInfo& info){
    RssiTrend tr=info.trend;
    if(tr==RssiTrend::UNKNOWN && info.samples>=2){
        const int d=(int)info.rssi-(int)info.previousRssi;
        if(d>=4) tr=RssiTrend::APPROACHING; else if(d<=-4) tr=RssiTrend::MOVING_AWAY;
    }
    if(tr==RssiTrend::APPROACHING) t.fillTriangle(x,y+7,x+8,y+7,x+4,y,Theme::GREEN);
    else if(tr==RssiTrend::MOVING_AWAY) t.fillTriangle(x,y,x+8,y,x+4,y+7,Theme::RED);
    else if(tr==RssiTrend::STEADY) t.drawFastHLine(x,y+4,8,Theme::CYAN);
}

static void drawWatchList(TFT_eSPI& t,uint32_t now,const DetectionEngine& eng){
    const int w=t.width(),h=t.height();
    Theme::drawTitleBar(t,">> WATCH TARGETS <<");
    int top,bottom,rowH,visible; watchListGeom(t,w,h,top,bottom,rowH,visible);
    t.fillRect(0,20,w,bottom-20,Theme::BG);
    buildWatchOrder(eng,now);
    const int maxScroll=s_watchCount>visible?s_watchCount-visible:0;
    if(s_watchScroll>maxScroll)s_watchScroll=(uint8_t)maxScroll;

    for(uint8_t row=0;row<(uint8_t)visible;row++){
        const uint8_t pos=(uint8_t)(s_watchScroll+row); if(pos>=s_watchCount)break;
        DetectionEngine::WatchTargetInfo info; if(!eng.watchTargetInfo(s_watchOrder[pos],info))continue;
        const int y=top+row*rowH; const bool fresh=info.seen && now-info.lastSeenMs<=10000;
        t.drawFastHLine(6,y+rowH-1,w-12,Theme::PURPLE);
        char fitted[28]; t.setTextSize(2);
        UiFit::fitMid(fitted,sizeof fitted,info.label,UiFit::chars(w-104,2));
        t.setTextColor(Theme::WHITE,Theme::BG); t.setCursor(8,y+1); t.print(fitted);
        char rbuf[16]; if(fresh)snprintf(rbuf,sizeof rbuf,"%d dBm",(int)info.rssi); else if(info.seen)snprintf(rbuf,sizeof rbuf,"OUT"); else snprintf(rbuf,sizeof rbuf,"WAIT");
        const int rw=t.textWidth(rbuf); t.setTextColor(fresh?Theme::CYAN:Theme::AMBER,Theme::BG); t.setCursor(w-rw-24,y+1); t.print(rbuf);
        if(fresh)drawWatchTrend(t,w-17,y+5,info);
        t.setTextSize(1); t.setTextColor(Theme::VAPOR_PURPLE,Theme::BG); t.setCursor(8,y+18);
        t.print(info.kind==DetectionEngine::WatchKind::BLE?"BLE":"WIFI");
        if(info.seen){ const uint32_t age=(now-info.lastSeenMs)/1000; t.printf("   seen %lus ago",(unsigned long)age); }
        else t.print("   waiting for signal");
    }
    Theme::ButtonBarGeom bar=Theme::computeButtonBar(w,h);
    const int bw=120,bx=(w-bw)/2; Theme::drawButton(t,bx,bar.y,bw,bar.h,"[ BACK ]",false);
}

bool uiWatchAlertListHitBack(int x,int y,int screenW,int screenH){
    Theme::ButtonBarGeom bar=Theme::computeButtonBar(screenW,screenH);
    const int bw=120,bx=(screenW-bw)/2;
    return x>=bx&&x<=bx+bw&&y>=bar.y&&y<screenH;
}
void uiWatchAlertListScroll(int delta){
    int n=(int)s_watchScroll+delta; if(n<0)n=0; if(n>=s_watchCount)n=s_watchCount?s_watchCount-1:0; s_watchScroll=(uint8_t)n;
}

void uiWatchAlertTick(TFT_eSPI& t, uint32_t now, const DetectionEngine& eng, bool advance) {
    int w = t.width();
    int h = t.height();

    if (eng.watchKind() == DetectionEngine::WatchKind::NONE) {
        drawWatchList(t, now, eng);
        return;
    }
    if (!s_alertReactionFired) {
        s_alertReactionFired = true;
        Squachy::watchAlertReaction();
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
    return x >= bx && x <= bx + bw && y >= by && y < t.height();
}
