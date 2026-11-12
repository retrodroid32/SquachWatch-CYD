// SquachWatch-CYD — HUNT MODE screen implementation
#include "ui_hunt.h"
#include "hunt_signal.h"
#include "hunt_history.h"
#include "theme.h"
#include "privacy.h"
#include "squachy.h"
#include "ui_fit.h"
#include <Arduino.h>
#include <string.h>

// -100..-30 dBm covers "barely there" to "right next to it" for both
// BLE and WiFi -- same clamp range ui_watchalert.cpp's sparkline uses,
// so a reading means the same thing on both screens.
static const int RSSI_LO = -100, RSSI_HI = -30;

// The HUNT footer uses the same *three-slot* layout and five-step button
// font geometry as the global SCAN/LOG/DESK bar. One shared rect definition
// drives BOTH draw and hit tests, including after screen rotation.
enum HuntFooterSlot : uint8_t { HUNT_BACK = 0, HUNT_GRAPH = 1, HUNT_STOP = 2 };
static void huntFooterRect(HuntFooterSlot slot, int screenW, int screenH,
                           int& x, int& y, int& w, int& h) {
    const Theme::ButtonBarGeom g = Theme::computeButtonBar(screenW, screenH);
    x = g.x[slot];
    y = g.y;
    w = g.w[slot];
    h = g.h;
}
static bool hitHuntFooter(HuntFooterSlot slot, int x, int y,
                          int screenW, int screenH) {
    int bx, by, bw, bh;
    huntFooterRect(slot, screenW, screenH, bx, by, bw, bh);
    return x >= bx && x < bx + bw && y >= by && y < by + bh;
}

// Tracks quip-worthy transitions across ticks -- see the trend block
// in uiHuntTick(). Reset on every fresh entry to the screen so a new
// hunt always gets its own STARTED line and a clean slate for
// warmer/colder/hot instead of carrying over whatever the last target
// left behind.
static RssiTrend s_lastTrend    = RssiTrend::UNKNOWN;
static bool       s_hotFired     = false;
static bool       s_everHot      = false;  // gates STALLED -- no point razzing someone who already found it
static bool       s_gotFirstSignal = false;
static uint32_t   s_enterMs        = 0;
static bool       s_stalledFired   = false;
static const uint32_t STALLED_MS = 60000;

// The catch. HOT fires his line at the top of the gauge, but a needle in
// the green is easy to miss with two boards in your hands, so the screen
// says it outright: two samples running at arm's length or closer, and
// CAUGHT! takes the trend line, held for a few seconds so a single dip
// does not take it away again. -48 dBm is what two of these boards read
// at about a metre; touching, they read -30 to -40.
static const int8_t   CAUGHT_DBM  = -48;
static const uint32_t CAUGHT_HOLD = 5000;
static uint32_t s_caughtUntil = 0;
static bool     s_caughtFired = false;
static bool     s_gaugeStarted = false;
static bool     s_prevSignalLive = false;  // transition-only serial diagnostic

// Alternate HUNT history view. No dynamic allocations, NVS settings or
// radio callbacks: all observations come from the coherent snapshot below.
static HuntHistory::Tracker s_history;
static bool s_graphMode = false;

bool uiHuntHitGraph(int x, int y, int screenW, int screenH) {
    return hitHuntFooter(HUNT_GRAPH, x, y, screenW, screenH);
}

void uiHuntToggleGraph() { s_graphMode = !s_graphMode; }


// HUNT list view. The stored target array keeps selection order; this index
// array is rebuilt by RSSI every frame so the actual records never move while
// a radio callback may be updating them.
static uint8_t s_listOrder[DetectionEngine::HUNT_TARGET_CAP] = {0};
static uint8_t s_listCount = 0;
static uint8_t s_listScroll = 0;

void uiHuntInit(TFT_eSPI& t) {
    t.fillRect(0, 0, t.width(), t.height(), Theme::BG);
    s_lastTrend       = RssiTrend::UNKNOWN;
    s_hotFired        = false;
    s_everHot         = false;
    s_gotFirstSignal  = false;
    s_enterMs         = millis();
    s_stalledFired    = false;
    s_caughtUntil     = 0;
    s_caughtFired     = false;
    s_gaugeStarted    = false;
    s_prevSignalLive = false;
    s_history.reset();
    s_graphMode       = false;
    s_listScroll      = 0;
}

bool uiHuntHitBack(int x, int y, int screenW, int screenH) {
    return hitHuntFooter(HUNT_BACK, x, y, screenW, screenH);
}

bool uiHuntCaught() { return (int32_t)(s_caughtUntil - millis()) > 0; }

bool uiHuntHitStop(int x, int y, int screenW, int screenH) {
    return hitHuntFooter(HUNT_STOP, x, y, screenW, screenH);
}

// Semicircle strength gauge -- sweeps left (weak) to right (strong)
// over the top, speedometer-style. Deliberately not a compass: theta
// is driven purely by current signal strength, not a heading, so the
// "direction" only comes from the user physically turning/moving
// while watching which way the needle swings.
static void drawGauge(TFT_eSPI& t, int cx, int cy, int r, float frac, uint16_t needleColor) {
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;

    // Track: a 9-point arc outline plus a short radial tick at each
    // point, dim purple to read as scale rather than signal.
    const uint8_t TICKS = 8;
    int px = cx - r, py = cy;
    for (uint8_t i = 1; i <= TICKS; i++) {
        float theta = (float)i * (PI / TICKS);
        int x = cx + (int)(r * cosf(theta) * -1.0f);
        int y = cy - (int)(r * sinf(theta));
        t.drawLine(px, py, x, y, Theme::PURPLE);
        int tx = cx + (int)((r - 6) * cosf(theta) * -1.0f);
        int ty = cy - (int)((r - 6) * sinf(theta));
        t.drawLine(x, y, tx, ty, Theme::PURPLE);
        px = x;
        py = y;
    }

    // Needle: theta=0 at frac=0 (left/weak), theta=PI at frac=1
    // (right/strong) -- same theta(0..PI) -> left..right sweep as the
    // track loop above.
    float needleTheta = PI * frac;
    int nx = cx + (int)((r - 10) * cosf(needleTheta) * -1.0f);
    int ny = cy - (int)((r - 10) * sinf(needleTheta));
    t.drawLine(cx, cy, nx, ny, needleColor);
    t.drawLine(cx + 1, cy, nx + 1, ny, needleColor);
    t.fillCircle(cx, cy, 4, needleColor);
}

static void huntListGeom(TFT_eSPI& t, int w, int h,
                         int& top, int& bottom, int& rowH, int& visible) {
    Theme::ButtonBarGeom bar = Theme::computeButtonBar(w, h);
    top = 24;
    bottom = bar.y - 4;
    t.setTextSize(2);
    const int nameH = t.fontHeight();
    t.setTextSize(1);
    const int detailH = t.fontHeight();
    rowH = nameH + detailH + 5;
    visible = (bottom - top) / rowH;
    if (visible < 1) visible = 1;
}

static int huntListScore(const DetectionEngine::HuntTargetInfo& info, uint32_t now) {
    if (!HuntSignal::fresh(info.samples, info.lastSeenMs, now,
                               info.kind == DetectionEngine::WatchKind::WIFI)) return -200;
    return (int)info.rssi;
}

static void buildHuntOrder(const DetectionEngine& eng, uint32_t now) {
    s_listCount = eng.huntTargetCount();
    for (uint8_t i = 0; i < s_listCount; i++) s_listOrder[i] = i;

    // Eight entries maximum: insertion sort is smaller and cheaper than
    // dragging in a generic sorter, and stable ordering avoids rows dancing
    // when two targets have the same rounded RSSI.
    for (uint8_t i = 1; i < s_listCount; i++) {
        const uint8_t key = s_listOrder[i];
        DetectionEngine::HuntTargetInfo keyInfo;
        eng.huntTargetInfo(key, keyInfo);
        const int keyScore = huntListScore(keyInfo, now);
        uint8_t j = i;
        while (j > 0) {
            DetectionEngine::HuntTargetInfo prevInfo;
            eng.huntTargetInfo(s_listOrder[j - 1], prevInfo);
            if (huntListScore(prevInfo, now) >= keyScore) break;
            s_listOrder[j] = s_listOrder[j - 1];
            j--;
        }
        s_listOrder[j] = key;
    }
}

static void drawHuntTrend(TFT_eSPI& t, int x, int y,
                          const DetectionEngine::HuntTargetInfo& info) {
    RssiTrend trend = info.trend;
    if (trend == RssiTrend::UNKNOWN && info.samples >= 2) {
        const int delta = (int)info.rssi - (int)info.previousRssi;
        if (delta >= 4) trend = RssiTrend::APPROACHING;
        else if (delta <= -4) trend = RssiTrend::MOVING_AWAY;
    }
    if (trend == RssiTrend::APPROACHING)
        t.fillTriangle(x, y + 7, x + 8, y + 7, x + 4, y, Theme::GREEN);
    else if (trend == RssiTrend::MOVING_AWAY)
        t.fillTriangle(x, y, x + 8, y, x + 4, y + 7, Theme::RED);
    else if (trend == RssiTrend::STEADY)
        t.drawFastHLine(x, y + 4, 8, Theme::CYAN);
}

static void drawHuntList(TFT_eSPI& t, uint32_t now, const DetectionEngine& eng) {
    const int w = t.width(), h = t.height();

    // This roster owns the whole screen. drawTitleBar() deliberately does not
    // erase its band because normal screens paint their background first.
    // Clearing only the list body left the previous CLEAR/gauge frame visible
    // around the title and bottom button area, which looked like another
    // screen was painting over this one as RSSI rows refreshed.
    t.fillRect(0, 0, w, h, Theme::BG);
    Theme::drawTitleBar(t, ">> HUNT TARGETS <<");

    int top, bottom, rowH, visible;
    huntListGeom(t, w, h, top, bottom, rowH, visible);

    buildHuntOrder(eng, now);
    const int maxScroll = s_listCount > visible ? s_listCount - visible : 0;
    if (s_listScroll > maxScroll) s_listScroll = (uint8_t)maxScroll;

    if (!s_listCount) {
        t.setTextSize(1);
        t.setTextColor(Theme::WHITE, Theme::BG);
        const char* empty = "NO HUNT TARGETS";
        t.setCursor((w - t.textWidth(empty)) / 2, top + 24);
        t.print(empty);
    }

    for (uint8_t row = 0; row < (uint8_t)visible; row++) {
        const uint8_t pos = (uint8_t)(s_listScroll + row);
        if (pos >= s_listCount) break;

        DetectionEngine::HuntTargetInfo info;
        if (!eng.huntTargetInfo(s_listOrder[pos], info)) continue;
        const int y = top + row * rowH;
        const bool fresh = HuntSignal::fresh(info.samples, info.lastSeenMs, now,
                               info.kind == DetectionEngine::WatchKind::WIFI);

        t.drawFastHLine(6, y + rowH - 1, w - 12, Theme::PURPLE);

        char fitted[28];
        t.setTextSize(2);
        UiFit::fitMid(fitted, sizeof fitted, info.label,
                      UiFit::chars(w - 104, 2));
        t.setTextColor(Theme::WHITE, Theme::BG);
        t.setCursor(8, y + 1);
        t.print(fitted);

        char rbuf[16];
        if (fresh) snprintf(rbuf, sizeof rbuf, "%d dBm", (int)info.rssi);
        else if (info.seen) snprintf(rbuf, sizeof rbuf, "OUT");
        else snprintf(rbuf, sizeof rbuf, "WAIT");
        const int rw = t.textWidth(rbuf);
        t.setTextColor(fresh ? Theme::CYAN : Theme::AMBER, Theme::BG);
        t.setCursor(w - rw - 24, y + 1);
        t.print(rbuf);
        if (fresh) drawHuntTrend(t, w - 17, y + 5, info);

        t.setTextSize(1);
        t.setTextColor(Theme::VAPOR_PURPLE, Theme::BG);
        t.setCursor(8, y + 18);
        t.print(info.kind == DetectionEngine::WatchKind::BLE ? "BLE" : "WIFI");
        if (info.seen) {
            const uint32_t age = (uint32_t)HuntSignal::ageMs(info.lastSeenMs, now) / 1000;
            t.printf("   seen %lus ago", (unsigned long)age);
        } else {
            t.print("   waiting for signal");
        }
    }

    Theme::ButtonBarGeom bar = Theme::computeButtonBar(w, h);
    const int bw = 120, bx = (w - bw) / 2;
    Theme::drawButton(t, bx, bar.y, bw, bar.h, "[ BACK ]", false);
}

int uiHuntListHitTarget(TFT_eSPI& t, int x, int y, int screenW, int screenH) {
    int top, bottom, rowH, visible;
    huntListGeom(t, screenW, screenH, top, bottom, rowH, visible);
    if (x < 0 || x >= screenW || y < top || y >= bottom) return -1;
    const int row = (y - top) / rowH;
    const int pos = (int)s_listScroll + row;
    if (row < 0 || row >= visible || pos < 0 || pos >= s_listCount) return -1;
    return (int)s_listOrder[pos];
}

bool uiHuntListHitBack(int x, int y, int screenW, int screenH) {
    Theme::ButtonBarGeom bar = Theme::computeButtonBar(screenW, screenH);
    const int bw = 120, bx = (screenW - bw) / 2;
    return x >= bx && x <= bx + bw && y >= bar.y && y < screenH;
}

void uiHuntListScroll(int delta) {
    int n = (int)s_listScroll + delta;
    if (n < 0) n = 0;
    if (n >= s_listCount) n = s_listCount ? s_listCount - 1 : 0;
    s_listScroll = (uint8_t)n;
}

void uiHuntListTick(TFT_eSPI& t, uint32_t now, const DetectionEngine& eng) {
    drawHuntList(t, now, eng);
}

// A second view of the SAME active HUNT target, not a replacement for
// the live gauge. Old points remain visible as history when the radio goes
// quiet, but a missing signal is drawn as a GAP, never an invented 0 dBm.
static void drawHuntHistory(TFT_eSPI& t, const HuntHistory::Tracker& hist,
                            int w, int labelY, int bodyBottom,
                            bool live, bool wifi, uint8_t channel,
                            uint32_t now, bool caught) {
    const int statY = bodyBottom - 38;
    const int left = 38, top = labelY + 23;
    const int width = w - left - 12, height = statY - top - 8;
    if (width < 35 || height < 20) return;

    t.drawRect(left, top, width, height, Theme::PURPLE);
    for (int j = 1; j <= 2; j++) {
        const int y = top + j * (height - 1) / 3;
        t.drawFastHLine(left + 1, y, width - 2, Theme::VAPOR_PURPLE);
    }
    t.setTextSize(1);
    t.setTextColor(Theme::VAPOR_PURPLE, Theme::BG);
    t.setCursor(4, top + 1); t.print("-30");
    t.setCursor(1, top + height - 10); t.print("-100");

    bool prevValid = false;
    int prevX = 0, prevY = 0;
    for (uint8_t i = 0; i < hist.count; i++) {
        const int8_t r = hist.at(i);
        if (r == HuntHistory::NO_SIGNAL) { prevValid = false; continue; }
        const int dbm = r < RSSI_LO ? RSSI_LO : r > RSSI_HI ? RSSI_HI : r;
        const int x = left + 2 + (int)(HuntHistory::CAP - hist.count + i) *
                      (width - 5) / (HuntHistory::CAP - 1);
        const int y = top + 2 + (RSSI_HI - dbm) * (height - 5) /
                      (RSSI_HI - RSSI_LO);
        if (prevValid) t.drawLine(prevX, prevY, x, y, Theme::CYAN);
        else t.fillCircle(x, y, 1, Theme::CYAN);
        prevX = x; prevY = y; prevValid = true;
    }

    // Text size is already 1 from the chart-axis labels above.
    t.setTextColor(Theme::CYAN, Theme::BG);
    t.setCursor(7, statY);
    if (hist.peakValid) {
        t.print("PEAK ");
        t.printf("%d", (int)hist.peakDbm);
        t.print(" dBm");
    } else t.print("PEAK --");
    char age[22];
    if (!hist.seen) memcpy(age, "LAST --", sizeof("LAST --"));
    else snprintf(age, sizeof age, "LAST %lus",
                  (unsigned long)((uint32_t)HuntSignal::ageMs(hist.lastSeenMs, now) / 1000u));
    t.setTextColor(hist.seen ? Theme::WHITE : Theme::AMBER, Theme::BG);
    t.setCursor(w - t.textWidth(age) - 8, statY);
    t.print(age);

    t.setCursor(8, statY + 14);
    t.setTextColor(live ? Theme::GREEN : Theme::AMBER, Theme::BG);
    if (caught) t.print("CAUGHT! ");
    else t.print(live ? "LIVE " : "NO SIGNAL ");
    t.setTextColor(Theme::VAPOR_PURPLE, Theme::BG);
    if (wifi) {
        if (channel >= 1 && channel <= 13) {
            t.print("WIFI CH ");
            // Avoid printf's numeric formatting path for a 1..13 channel.
            // The emulator TFT shim intentionally only supports print(strings).
            if (channel >= 10) {
                t.write((uint8_t)'1');
                t.write((uint8_t)('0' + channel - 10));
            } else t.write((uint8_t)('0' + channel));
        } else t.print("WIFI CH ?");
    } else t.print("BLE");
}

void uiHuntTick(TFT_eSPI& t, uint32_t now, const DetectionEngine& eng, bool advance) {
    int w = t.width(), h = t.height();

    if (!s_gaugeStarted) {
        s_gaugeStarted = true;
        Squachy::huntReaction(Squachy::HuntMoment::STARTED);
    }

    Theme::drawTitleBar(t, ">> HUNT MODE <<");

    Theme::ButtonBarGeom bar = Theme::computeButtonBar(w, h);
    int bodyTop = 16, bodyBottom = bar.y - 4;
    t.fillRect(0, bodyTop, w, bodyBottom - bodyTop, Theme::BG);

    // Mini Squachy cameo, same reduced-header-strip reuse ui_rawscan.cpp
    // and ui_watchalert.cpp already use -- his normal idle tick, just
    // given a small box instead of the whole screen. scanningFx ties
    // his little "ping" animation to the hunting theme.
    const int sqH = 44;
    Squachy::tick(t, w / 2, bodyTop, sqH, now, advance, 0.6f, true);

    t.setTextSize(1);
    t.setTextWrap(false);
    t.setTextColor(Theme::CYAN, Theme::BG);
    int labelY = bodyTop + sqH + 2;
    char pv[40];
    const char* label = Privacy::name(eng.huntLabel(), pv, sizeof pv);
    int lw = t.textWidth(label);
    int maxLw = w - 16;
    t.setCursor((w - (lw < maxLw ? lw : maxLw)) / 2, labelY);
    t.print(label);

    // One atomic snapshot of target, count, chronological RSSI, timestamp
    // and radio type. The NimBLE host task can publish new observations
    // while this frame is being rendered. Separate getters once mixed a
    // new timestamp with an old count and caused false 0% flashes.
    DetectionEngine::HuntSignalSnapshot signal;
    const bool haveTarget = eng.huntSignalSnapshot(signal);
    const uint32_t huntNow = millis();  // never reuse a loop-start timestamp
    const bool live = haveTarget &&
        HuntSignal::fresh(signal.samples, signal.lastSeenMs, huntNow,
                          signal.kind == DetectionEngine::WatchKind::WIFI);
    const uint8_t storedRssiN = haveTarget ? signal.samples : 0;
    const uint8_t rssiN = live ? storedRssiN : 0;
    if (advance) s_history.tick(huntNow, haveTarget && storedRssiN > 0,
                               live, rssiN ? signal.rssi[rssiN - 1] : -127,
                               haveTarget ? signal.lastSeenMs : 0);
    if (live != s_prevSignalLive) {
        Serial.printf("[hunt-rssi] radio=%s live=%u samples=%u age=%ld ms channel=%u\n",
                      signal.kind == DetectionEngine::WatchKind::WIFI ? "WIFI" : "BLE",
                      (unsigned)live, (unsigned)storedRssiN,
                      (long)(haveTarget ? HuntSignal::ageMs(signal.lastSeenMs, huntNow) : -1),
                      (unsigned)signal.channel);
        s_prevSignalLive = live;
    }
    // History is kept for the HUNT roster but must NEVER drive a live
    // needle, numeric reading, trend or CAUGHT badge after signal loss.
    if (!live) {
        s_hotFired = false;
        s_caughtUntil = 0;
        s_caughtFired = false;
        s_lastTrend = RssiTrend::UNKNOWN;
    }

    // First-ever sample for this target -- fires once, well before
    // there's enough history for a warmer/colder trend (needs 3+).
    if (rssiN > 0 && !s_gotFirstSignal) {
        s_gotFirstSignal = true;
        Squachy::huntReaction(Squachy::HuntMoment::FIRST_SIGNAL);
    }

    // A hunt that's dragged on a while without ever reaching HOT gets
    // one (only one) impatient nudge -- pure flavor, checked once a
    // tick is cheap enough not to bother gating further.
    if (!s_everHot && !s_stalledFired && (now - s_enterMs) >= STALLED_MS) {
        s_stalledFired = true;
        Squachy::huntReaction(Squachy::HuntMoment::STALLED);
    }

    int textBlockH = 36;
    int gaugeTop = labelY + 12;
    int gaugeBottom = bodyBottom - textBlockH;
    int r = gaugeBottom - gaugeTop;
    int maxRw = (w - 40) / 2;
    if (r > maxRw) r = maxRw;
    if (r < 30) r = 30;
    int cx = w / 2, cy = gaugeBottom;

    float frac = 0.0f;
    int8_t latestRssi = RSSI_LO;
    if (rssiN > 0) {
        latestRssi = signal.rssi[rssiN - 1];
        int v = latestRssi;
        if (v < RSSI_LO) v = RSSI_LO;
        if (v > RSSI_HI) v = RSSI_HI;
        frac = (float)(v - RSSI_LO) / (float)(RSSI_HI - RSSI_LO);
    }
    uint16_t needleColor = (frac < 0.5f)
        ? Theme::blend(Theme::RED, Theme::AMBER, (uint16_t)(frac * 2.0f * 256))
        : Theme::blend(Theme::AMBER, Theme::GREEN, (uint16_t)((frac - 0.5f) * 2.0f * 256));
    if (!s_graphMode) drawGauge(t, cx, cy, r, frac, needleColor);

    // Fires once per visit to "basically on top of it" territory, not
    // every tick it stays there -- re-arms if the signal drops back out
    // so a genuine re-approach (walked away, came back) can fire again.
    if (rssiN > 0 && frac >= 0.92f) {
        if (!s_hotFired) { Squachy::huntReaction(Squachy::HuntMoment::HOT); s_hotFired = true; }
        s_everHot = true;
    } else {
        s_hotFired = false;
    }
    // The catch: this sample and the one before both at arm's length.
    if (rssiN >= 2 && latestRssi >= CAUGHT_DBM && signal.rssi[rssiN - 2] >= CAUGHT_DBM) {
        s_caughtUntil = huntNow + CAUGHT_HOLD;
        if (!s_caughtFired) {
            s_caughtFired = true;
            Squachy::huntReaction(Squachy::HuntMoment::HOT);
        }
    } else if (rssiN >= 1 && latestRssi < CAUGHT_DBM - 10 && !uiHuntCaught()) {
        s_caughtFired = false;         // walked off again: the next catch counts
    }
    const bool caught = uiHuntCaught();

    // Numeric readout + the shared v1.21 RSSI trend. HUNT keeps a longer
    // fixed history than a Detection, but classifies the newest up-to-eight
    // cadence samples with the same 3-sample / +/-6 dB rule used by LOG.
    // The playful HUNT wording stays intentionally different from the compact
    // LOG arrows even though the underlying meaning is now identical.
    char rbuf[16];
    if (rssiN > 0) snprintf(rbuf, sizeof(rbuf), "%d dBm", (int)latestRssi);
    else           snprintf(rbuf, sizeof(rbuf), "%s", HuntSignal::absentReadout(storedRssiN > 0));
    t.setTextSize(2);
    int rw = t.textWidth(rbuf);
    t.setTextColor(Theme::WHITE, Theme::BG);
    t.setCursor((w - rw) / 2, cy + 8);
    if (!caught && !s_graphMode) t.print(rbuf);
    // fontHeight() no-arg reads back the size-2 metrics just set above
    // -- fontHeight(int) takes a FONT INDEX, not a size multiplier, and
    // would silently query the wrong thing here.
    int readoutH = t.fontHeight();

    const char* trend = HuntSignal::absentStatus(storedRssiN > 0);
    uint16_t trendColor = Theme::WHITE;
    const uint8_t trendCount = rssiN > 8 ? 8 : rssiN;
    const uint8_t trendStart = (uint8_t)(rssiN - trendCount);
    const RssiTrend cur = trendCount
        ? classifyRssiTrend((int)signal.rssi[trendStart], (int)latestRssi, trendCount)
        : RssiTrend::UNKNOWN;
    if (cur == RssiTrend::APPROACHING) {
        trend = "GETTING WARMER";
        trendColor = Theme::GREEN;
    } else if (cur == RssiTrend::MOVING_AWAY) {
        trend = "GETTING COLDER";
        trendColor = Theme::RED;
    } else if (cur == RssiTrend::STEADY) {
        trend = "HOLDING STEADY";
        trendColor = Theme::CYAN;
    }

    // Only on an actual change -- not every tick the trend still reads the
    // same way, or he'd never shut up.
    if (cur != RssiTrend::UNKNOWN && cur != s_lastTrend) {
        if (cur == RssiTrend::APPROACHING)
            Squachy::huntReaction(Squachy::HuntMoment::WARMER);
        else if (cur == RssiTrend::MOVING_AWAY)
            Squachy::huntReaction(Squachy::HuntMoment::COLDER);
        s_lastTrend = cur;
    }
    // Caught: the word takes the readout's row, in a green box the eye cannot
    // miss from across a table, and the number moves down to the trend line.
    if (caught) {
        t.setTextSize(2);
        const int cw2 = t.textWidth("CAUGHT!") + 16, ch2 = t.fontHeight() + 4;
        const int cx2 = (w - cw2) / 2, cy2 = cy + 6;
        if (!s_graphMode) {
            t.fillRect(cx2, cy2, cw2, ch2, Theme::GREEN);
            t.setTextColor(Theme::BLACK, Theme::GREEN);
            t.setCursor(cx2 + 8, cy2 + 2);
            t.print("CAUGHT!");
        }
        trend = rbuf;
        trendColor = Theme::GREEN;
    }
    if (s_graphMode) {
        drawHuntHistory(t, s_history, w, labelY, bodyBottom, live,
                        haveTarget && signal.kind == DetectionEngine::WatchKind::WIFI,
                        haveTarget ? signal.channel : 0, huntNow, caught);
    } else {
        t.setTextSize(1);
        int tw = t.textWidth(trend);
        t.setTextColor(trendColor, Theme::BG);
        t.setCursor((w - tw) / 2, cy + 8 + readoutH + 2);
        t.print(trend);
    }

    // Button style, size and hit geometry all match the firmware's standard
    // footer. Short unbracketed labels fit even 240px portrait at size 5/5.
    int bx, by, bw, bh;
    huntFooterRect(HUNT_BACK, w, h, bx, by, bw, bh);
    Theme::drawButton(t, bx, by, bw, bh, "BACK", false);
    huntFooterRect(HUNT_GRAPH, w, h, bx, by, bw, bh);
    Theme::drawButton(t, bx, by, bw, bh, s_graphMode ? "GAUGE" : "GRAPH", false);
    huntFooterRect(HUNT_STOP, w, h, bx, by, bw, bh);
    Theme::drawButton(t, bx, by, bw, bh, "STOP", false);
}
