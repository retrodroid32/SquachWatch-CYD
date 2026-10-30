// SquachWatch-CYD -- WIFI NETWORKS. See include/ui_wifinets.h.
#include "ui_wifinets.h"
#include "ota_wifi.h"
#include "theme.h"
#include "privacy.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

namespace {

const int BTN_H   = 28;
const int ROW_Y0  = 36;
const int ROW_H   = 26;
const int ROW_GAP = 3;

int s_selected = -1;

// Small, direct-to-panel renderer for a CYD32 which has lent its full-frame
// sprite to WiFi. A press must not erase and redraw every pixel of the menu.
bool s_directReady = false;
int s_directW = 0, s_directH = 0, s_lastSelected = -1;
uint8_t s_lastCount = 0, s_lastUse = 0;
OtaWifi::SavedTest s_lastTest = OtaWifi::SavedTest::IDLE;
OtaWifi::SavedResult s_lastResult[OtaWifi::SAVED_MAX] = {};
uint32_t s_lastToastSerial = 0;
bool s_lastToastVisible = false;


bool in(int x, int y, int bx, int by, int bw, int bh) {
    const int slop = 6;
    return x >= bx - slop && x < bx + bw + slop && y >= by - slop && y < by + bh + slop;
}

// The button bar: one row of four on a wide screen, two rows of two on a
// tall one. Returns the y the rows above must stay clear of.
struct Bar { int rows, w, x[4], y[4]; };
Bar bar(TFT_eSPI& t, int n) {
    Bar b;
    const int gap = 8;
    const bool wide = t.width() >= 300;
    const int perRow = wide ? n : 2;
    b.rows = (n + perRow - 1) / perRow;
    b.w = (t.width() - 16 - (perRow - 1) * gap) / perRow;
    if (b.w > 110) b.w = 110;
    for (int i = 0; i < n; i++) {
        const int r = i / perRow, c = i % perRow;
        const int span = perRow * b.w + (perRow - 1) * gap;
        b.x[i] = (t.width() - span) / 2 + c * (b.w + gap);
        b.y[i] = t.height() - 6 - (b.rows - r) * BTN_H - (b.rows - 1 - r) * 6;
    }
    return b;
}

int rowsThatFit(TFT_eSPI& t, int barTop) {
    const int n = (barTop - 6 - ROW_Y0) / (ROW_H + ROW_GAP);
    return n < 1 ? 1 : n;
}

void paragraph(TFT_eSPI& t, int y, uint16_t c, const char* s) {
    // Wrapped at the screen width, on the small font, a word at a time.
    const int maxW = t.width() - 16;
    char line[72] = "";
    t.setTextColor(c, Theme::BG);
    while (*s) {
        while (*s == ' ') s++;
        const char* e = s;
        while (*e && *e != ' ') e++;
        char word[40];
        snprintf(word, sizeof word, "%.*s", (int)(e - s), s);
        char trial[72];
        snprintf(trial, sizeof trial, "%s%s%s", line, line[0] ? " " : "", word);
        if (line[0] && t.textWidth(trial) > maxW) {
            t.setCursor(8, y); t.print(line); y += 11;
            snprintf(line, sizeof line, "%s", word);
        } else {
            snprintf(line, sizeof line, "%s", trial);
        }
        s = e;
    }
    if (line[0]) { t.setCursor(8, y); t.print(line); }
}

const char* resultWords(OtaWifi::SavedResult r) {
    switch (r) {
        case OtaWifi::SavedResult::JOINED:       return "last connection verified";
        case OtaWifi::SavedResult::BAD_PASSWORD: return "wrong password";
        case OtaWifi::SavedResult::NOT_FOUND:    return "network not found";
        case OtaWifi::SavedResult::FAILED:       return "connection failed";
        default:                                 return "not tried yet";
    }
}

uint16_t resultColour(OtaWifi::SavedResult r) {
    switch (r) {
        case OtaWifi::SavedResult::JOINED:       return Theme::GREEN;
        case OtaWifi::SavedResult::BAD_PASSWORD: return Theme::AMBER;
        case OtaWifi::SavedResult::NOT_FOUND:    return Theme::W95_SHADOW;
        case OtaWifi::SavedResult::FAILED:       return Theme::AMBER;
        default:                                 return Theme::CYAN;
    }
}

void signalBars(TFT_eSPI& t, int x, int yBottom, int8_t rssi) {
    const int bars = rssi >= -55 ? 4 : rssi >= -65 ? 3 : rssi >= -75 ? 2 : 1;
    for (int i = 0; i < 4; i++) {
        const int bh = 3 + i * 3;
        t.fillRect(x + i * 4, yBottom - bh, 3, bh, i < bars ? Theme::CYAN : Theme::W95_SHADOW);
    }
}

}  // namespace

// ---- the list ---------------------------------------------------------------

// Paint the one saved-network row whose selection or result changed.
// The 2.8 full-frame implementation reuses this exact same visual layout.
static void drawSavedRow(TFT_eSPI& t, int i, int w) {
    const int y = ROW_Y0 + i * (ROW_H + ROW_GAP);
    const bool sel = i == s_selected;
    const bool use = i == OtaWifi::savedUse();
    t.fillRect(8, y, w - 16, ROW_H, Theme::TASKBAR);
    t.drawRect(8, y, w - 16, ROW_H, sel ? Theme::VAPOR_PINK : Theme::VAPOR_PURPLE);
    const char* tag = use ? "USE" : "";
    const int tagW = tag[0] ? t.textWidth(tag) + 6 : 0;
    const int maxChars = (w - 16 - 12 - tagW) / t.textWidth("M");
    char name[40], pv[40];
    snprintf(name, sizeof name, "%.*s", maxChars > 32 ? 32 : maxChars,
             Privacy::name(OtaWifi::savedSsidAt((uint8_t)i), pv, sizeof pv));
    t.setTextColor(Theme::WHITE, Theme::TASKBAR);
    t.setCursor(14, y + 4);
    t.print(name);
    if (tag[0]) {
        t.setTextColor(Theme::CYAN, Theme::TASKBAR);
        t.setCursor(w - 16 - tagW, y + 4);
        t.print(tag);
    }
    const OtaWifi::SavedResult result = OtaWifi::savedResult((uint8_t)i);
    t.setTextColor(resultColour(result), Theme::TASKBAR);
    t.setCursor(14, y + 15);
    t.print(OtaWifi::savedTestState() == OtaWifi::SavedTest::CONNECTING &&
            i == OtaWifi::savedUse() ? "CONNECTING..." : resultWords(result));
}

static void drawSavedButtons(TFT_eSPI& t, uint8_t n, const Bar& b) {
    const bool haveSel = s_selected >= 0 && s_selected < n;
    const bool full = n >= OtaWifi::SAVED_MAX;
    const bool busy = OtaWifi::savedTestState() == OtaWifi::SavedTest::CONNECTING;
    Theme::drawWin95Button(t, b.x[0], b.y[0], b.w, BTN_H, busy ? "TEST..." : "USE", !haveSel || busy);
    Theme::drawWin95Button(t, b.x[1], b.y[1], b.w, BTN_H, "REMOVE", !haveSel);
    Theme::drawWin95Button(t, b.x[2], b.y[2], b.w, BTN_H, full ? "FULL" : "ADD", full);
    Theme::drawWin95Button(t, b.x[3], b.y[3], b.w, BTN_H, "BACK", false);
}

void uiWifiNetsInit(TFT_eSPI& t) {
    s_selected = OtaWifi::savedCount() ? (int)OtaWifi::savedUse() : -1;
    s_directReady = false;
    t.fillRect(0, 0, t.width(), t.height(), Theme::BG);
}

void uiWifiNetsSelect(int row) { s_selected = row; }
int  uiWifiNetsSelected()      { return s_selected; }

void uiWifiNetsTick(TFT_eSPI& t, uint32_t now) {
    s_directReady = false;
    t.fillRect(0, 0, t.width(), t.height(), Theme::BG);
    Theme::drawTitleBar(t, ">> WIFI NETWORKS <<");
    t.setTextSize(1);
    t.setTextWrap(false);
    const int w = t.width();
    const uint8_t n = OtaWifi::savedCount();
    const Bar b = bar(t, 4);
    if (!n) {
        paragraph(t, ROW_Y0 + 4, Theme::WHITE,
                  "No networks saved. ADD picks one from a scan. The password is checked at the next boot, "
                  "and this list says how that went.");
    }
    const int fit = rowsThatFit(t, b.y[0]);
    for (int i = 0; i < n && i < fit; i++) drawSavedRow(t, i, w);
    drawSavedButtons(t, n, b);
    Theme::drawToast(t, now);
}

// CYD32-only call: paints ONLY dirty rows/buttons and the toast's former
// central footprint after the initial full scene. No full-screen fill on a
// button press, finger lift, selection change or WiFi test completion.
void uiWifiNetsTickDirect(TFT_eSPI& t, uint32_t now) {
    const int w = t.width(), h = t.height();
    const uint8_t n = OtaWifi::savedCount();
    const uint8_t use = OtaWifi::savedUse();
    const OtaWifi::SavedTest test = OtaWifi::savedTestState();
    const bool toastVisible = Theme::toastActive(now);
    const uint32_t toastSerial = Theme::toastSerial();
    const Bar b = bar(t, 4);
    const int fit = rowsThatFit(t, b.y[0]);

    if (!s_directReady || s_directW != w || s_directH != h) {
        uiWifiNetsTick(t, now); // initial scene, not a button update
        s_directReady = true;
    } else {
        t.setTextSize(1);
        t.setTextWrap(false);
        const bool toastChanged = toastSerial != s_lastToastSerial ||
                                  toastVisible != s_lastToastVisible;
        // Toasts are centred; even the maximum 3-line toast is <=68px
        // high and <=w-20 wide. Restore only that footprint, not the LCD.
        if (s_lastToastVisible && toastChanged) {
            const int top = (h - 68) / 2;
            t.fillRect(10, top, w - 20, 68, Theme::BG);
            if (!n) {
                paragraph(t, ROW_Y0 + 4, Theme::WHITE,
                          "No networks saved. ADD picks one from a scan. The password is checked at the next boot, "
                          "and this list says how that went.");
            }
            // Redraw underlying rows touched by the old toast and nothing
            // beyond. The row painters restore their own backgrounds.
            for (int i = 0; i < n && i < fit; i++) {
                const int y = ROW_Y0 + i * (ROW_H + ROW_GAP);
                if (y < top + 68 && y + ROW_H > top) drawSavedRow(t, i, w);
            }
        }
        if (n != s_lastCount) {
            const uint8_t rows = n > s_lastCount ? n : s_lastCount;
            for (int i = 0; i < rows && i < fit; i++) {
                const int y = ROW_Y0 + i * (ROW_H + ROW_GAP);
                t.fillRect(8, y, w - 16, ROW_H, Theme::BG);
                if (i < n) drawSavedRow(t, i, w);
            }
            if (!n) {
                paragraph(t, ROW_Y0 + 4, Theme::WHITE,
                          "No networks saved. ADD picks one from a scan. The password is checked at the next boot, "
                          "and this list says how that went.");
            } else if (!s_lastCount) {
                // Clear the old empty-list paragraph before the new rows.
                t.fillRect(8, ROW_Y0, w - 16, b.y[0] - ROW_Y0, Theme::BG);
                for (int i = 0; i < n && i < fit; i++) drawSavedRow(t, i, w);
            }
        } else {
            for (int i = 0; i < n && i < fit; i++) {
                const OtaWifi::SavedResult result = OtaWifi::savedResult((uint8_t)i);
                if (i == s_selected || i == s_lastSelected ||
                    i == use || i == s_lastUse) {
                    if (s_selected != s_lastSelected || use != s_lastUse ||
                        test != s_lastTest || result != s_lastResult[i])
                        drawSavedRow(t, i, w);
                } else if (result != s_lastResult[i]) {
                    drawSavedRow(t, i, w);
                }
            }
        }
        if (n != s_lastCount || s_selected != s_lastSelected ||
            test != s_lastTest) drawSavedButtons(t, n, b);
        if (toastVisible && (toastChanged || n != s_lastCount ||
                            test != s_lastTest || s_selected != s_lastSelected ||
                            use != s_lastUse))
            Theme::drawToast(t, now);
    }
    s_directW = w;
    s_directH = h;
    s_lastCount = n;
    s_lastUse = use;
    s_lastSelected = s_selected;
    s_lastTest = test;
    for (uint8_t i = 0; i < n; i++) s_lastResult[i] = OtaWifi::savedResult(i);
    s_lastToastSerial = toastSerial;
    s_lastToastVisible = toastVisible;
}

WifiNetsHit uiWifiNetsHit(TFT_eSPI& t, int x, int y, int* row) {
    const Bar b = bar(t, 4);
    const int fit = rowsThatFit(t, b.y[0]);
    const uint8_t n = OtaWifi::savedCount();
    for (int i = 0; i < n && i < fit; i++) {
        const int ry = ROW_Y0 + i * (ROW_H + ROW_GAP);
        if (y >= ry && y < ry + ROW_H + ROW_GAP && x >= 8 && x <= t.width() - 8) {
            if (row) *row = i;
            return WifiNetsHit::ROW;
        }
    }
    if (in(x, y, b.x[0], b.y[0], b.w, BTN_H)) return WifiNetsHit::USE;
    if (in(x, y, b.x[1], b.y[1], b.w, BTN_H)) return WifiNetsHit::REMOVE;
    if (in(x, y, b.x[2], b.y[2], b.w, BTN_H)) return WifiNetsHit::ADD;
    if (in(x, y, b.x[3], b.y[3], b.w, BTN_H)) return WifiNetsHit::BACK;
    return WifiNetsHit::NONE;
}

// ---- adding one: the scan -----------------------------------------------------

void uiWifiAddInit(TFT_eSPI& t) {
    t.fillRect(0, 0, t.width(), t.height(), Theme::BG);
}

void uiWifiAddTick(TFT_eSPI& t, uint32_t now, const DetectionEngine& eng) {
    t.fillRect(0, 0, t.width(), t.height(), Theme::BG);
    Theme::drawTitleBar(t, ">> ADD A NETWORK <<");
    t.setTextSize(1);
    t.setTextWrap(false);
    const int w = t.width();
    const Bar b = bar(t, 2);
    const int fit = rowsThatFit(t, b.y[0]);
    if (!eng.rawWifiScanDone()) {
        const char* dots[] = { "SCANNING", "SCANNING.", "SCANNING..", "SCANNING..." };
        t.setTextColor(Theme::CYAN, Theme::BG);
        t.setCursor(8, ROW_Y0 + 4);
        t.print(dots[(now / 400) % 4]);
    } else if (!eng.rawWifiCount()) {
        paragraph(t, ROW_Y0 + 4, Theme::WHITE, "No networks found. Move closer to the router and press RESCAN.");
    }
    const uint8_t n = eng.rawWifiCount();
    for (int i = 0; i < n && i < fit; i++) {
        const int y = ROW_Y0 + i * (ROW_H + ROW_GAP);
        const char* ssid = eng.rawWifiSsid((uint8_t)i);
        const bool saved = OtaWifi::savedIndexOf(ssid) >= 0;
        const bool open  = eng.rawWifiOpen((uint8_t)i);
        t.fillRect(8, y, w - 16, ROW_H, Theme::TASKBAR);
        t.drawRect(8, y, w - 16, ROW_H, Theme::VAPOR_PURPLE);
        const char* tag  = saved ? "SAVED" : (open ? "OPEN" : "");
        const int   tagW = tag[0] ? t.textWidth(tag) + 6 : 0;
        const int maxChars = (w - 16 - 12 - 20 - tagW) / t.textWidth("M");
        char name[40];
        char pv[40];
        snprintf(name, sizeof name, "%.*s", maxChars > 32 ? 32 : maxChars, ssid[0] ? Privacy::name(ssid, pv, sizeof pv) : "(hidden)");
        t.setTextColor(Theme::WHITE, Theme::TASKBAR);
        t.setCursor(14, y + (ROW_H - t.fontHeight()) / 2);
        t.print(name);
        if (tag[0]) {
            t.setTextColor(saved ? Theme::CYAN : Theme::AMBER, Theme::TASKBAR);
            t.setCursor(w - 16 - 20 - tagW, y + (ROW_H - t.fontHeight()) / 2);
            t.print(tag);
        }
        signalBars(t, w - 16 - 16, y + ROW_H - 5, eng.rawWifiRssi((uint8_t)i));
    }
    Theme::drawWin95Button(t, b.x[0], b.y[0], b.w, BTN_H, "RESCAN", false);
    Theme::drawWin95Button(t, b.x[1], b.y[1], b.w, BTN_H, "BACK",   false);
    Theme::drawToast(t, now);
}

WifiAddHit uiWifiAddHit(TFT_eSPI& t, int x, int y, const DetectionEngine& eng, int* row) {
    const Bar b = bar(t, 2);
    const int fit = rowsThatFit(t, b.y[0]);
    const uint8_t n = eng.rawWifiScanDone() ? eng.rawWifiCount() : 0;
    for (int i = 0; i < n && i < fit; i++) {
        const int ry = ROW_Y0 + i * (ROW_H + ROW_GAP);
        if (y >= ry && y < ry + ROW_H + ROW_GAP && x >= 8 && x <= t.width() - 8) {
            if (row) *row = i;
            return WifiAddHit::ROW;
        }
    }
    if (in(x, y, b.x[0], b.y[0], b.w, BTN_H)) return WifiAddHit::RESCAN;
    if (in(x, y, b.x[1], b.y[1], b.w, BTN_H)) return WifiAddHit::BACK;
    return WifiAddHit::NONE;
}
