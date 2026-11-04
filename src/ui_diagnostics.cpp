// SquachWatch-CYD — on-device diagnostics screen implementation
#include "ui_diagnostics.h"
#if defined(GPS_SUPPORT) && defined(GPS_RX_PIN)
#include "gps_runtime.h"
#include "wardrive.h" // UTC stamp formatting only; no WiGLE row export
#endif
#include "clock.h"
#include "theme.h"
#include "detection.h"
#if SQUACH_MESH
#include "meshtalk.h"
#endif
#include <Arduino.h>
#include <stdarg.h>

static void backButtonRect(int screenW, int screenH, int& x, int& y, int& w, int& h) {
    Theme::ButtonBarGeom g = Theme::computeButtonBar(screenW, screenH);
    w = 120;
    h = g.h;
    x = (screenW - w) / 2;
    y = g.y;
}

// Diagnostics are built as a small fixed array of complete rows. The old
// scrolling display painted each field on a single line, so long values
// vanished off the right edge, while page steps could stop in mid-row.
// A static array (no heap allocations) allows measured word wrapping and
// deterministic, whole-row pages on both CYD orientations and both bands.
static const int DIAG_MAX_ROWS = 40;
static const int DIAG_VALUE_CAP = 112;
struct DiagRow {
    const char* label; // All caller labels are string literals
    uint16_t color;
    char value[DIAG_VALUE_CAP];
};
static DiagRow s_diagRows[DIAG_MAX_ROWS];
static int s_diagRowCount = 0;
static int s_diagPage = 0;
static int s_diagPageCount = 1;
static uint8_t s_diagPageStarts[DIAG_MAX_ROWS + 1];

void uiDiagnosticsInit(TFT_eSPI& t) {
    s_diagPage = 0;
    s_diagPageCount = 1;
    s_diagRowCount = 0;
    t.fillRect(0, 0, t.width(), t.height(), Theme::BG);
}

void uiDiagnosticsPage(int direction) {
    const int next = s_diagPage + (direction > 0 ? 1 : direction < 0 ? -1 : 0);
    s_diagPage = next < 0 ? 0 : next >= s_diagPageCount ? s_diagPageCount - 1 : next;
}

void uiDiagnosticsScroll(int delta) {
    // A swipe changes one whole page. It cannot strand half of a report row
    // above or beneath the visible area.
    uiDiagnosticsPage(delta);
}

int uiDiagnosticsHitPage(int x, int y, int screenW, int screenH) {
    const Theme::ButtonBarGeom bar = Theme::computeButtonBar(screenW, screenH);
    if (y < bar.y - 20 || y >= bar.y) return 0;
    if (x >= 0 && x < screenW / 3) return -1;
    if (x >= (screenW * 2) / 3 && x < screenW) return 1;
    return 0;
}

bool uiDiagnosticsHitBack(int x, int y, int screenW, int screenH) {
    int bx, by, bw, bh;
    backButtonRect(screenW, screenH, bx, by, bw, bh);
    return x >= bx && x <= bx + bw && y >= by && y < screenH;
}

static int drawLine(TFT_eSPI& t, int y, uint16_t labelColor,
                    const char* label, const char* fmt, ...) {
    (void)t;
    if (s_diagRowCount < DIAG_MAX_ROWS) {
        DiagRow& row = s_diagRows[s_diagRowCount++];
        row.label = label;
        row.color = labelColor;
        va_list args;
        va_start(args, fmt);
        vsnprintf(row.value, sizeof(row.value), fmt, args);
        va_end(args);
    }
    return y + 1; // Compatibility with the existing report assembler
}

// Draw font 1 at 1x: six pixels per glyph. Copy a bounded fragment into a
// local buffer rather than changing rows' stored values or using String.
// Break at the last whitespace that fits, and hard-break long version/hex
// tokens when they have no spaces. Measure and paint using the SAME helper
// so pagination always matches the actual rendered line count.
static int diagRowHeight(TFT_eSPI& t, const DiagRow& row,
                         int screenW, int y, bool paint) {
    const int lineH = 11;
    const int xLabel = 8;
    const int xValue = xLabel + t.textWidth(row.label) + 7;
    const char* p = row.value;
    int nLines = 0;

    if (paint) {
        t.setTextColor(row.color, Theme::BG);
        t.setCursor(xLabel, y);
        t.print(row.label);
    }

    do {
        const int x = nLines == 0 ? xValue : xLabel;
        int maxChars = (screenW - 8 - x) / 6;
        if (maxChars < 1) maxChars = 1;
        const size_t available = strlen(p);
        size_t take = available < (size_t)maxChars ? available : (size_t)maxChars;
        if (take < available) {
            // Prefer a word boundary, but never leave an empty segment.
            size_t cut = take;
            while (cut > 0 && p[cut] != ' ' && p[cut - 1] != ' ') --cut;
            if (cut > 0) take = cut;
        }
        // Avoid drawing a trailing blank at the end of a wrapped segment.
        while (take > 0 && p[take - 1] == ' ') --take;
        if (take == 0 && available != 0) take = available < (size_t)maxChars ? available : (size_t)maxChars;
        if (paint && take) {
            char fragment[DIAG_VALUE_CAP];
            memcpy(fragment, p, take);
            fragment[take] = '\0';
            t.setTextColor(Theme::WHITE, Theme::BG);
            t.setCursor(x, y + nLines * lineH);
            t.print(fragment);
        }
        p += take;
        while (*p == ' ') ++p;
        ++nLines;
    } while (*p);

    return nLines * lineH + 4;
}

void uiDiagnosticsTick(TFT_eSPI& t, uint32_t now, const DetectionEngine& eng, const DiagnosticsInfo& info) {
    const int w = t.width(), h = t.height();
    // Reset inherited canvas font state before measuring or printing.
    t.setTextFont(1);
    t.setTextSize(1);
    t.setTextWrap(false);

    // Build the entire report before deciding where page breaks belong.
    // This runs independently in both halves of the 3.2's sprite pass.
    s_diagRowCount = 0;
    int y = 0;
    y = drawLine(t, y, Theme::CYAN, "BOARD:", "%s (%s)", info.boardName,
                 info.usingCapTouch ? "capacitive" : "resistive");
#if defined(GPS_SUPPORT) && defined(GPS_RX_PIN)
    // Live receiver diagnostics, rather than the LAB's synthetic AP/WiGLE test.
    {
        const GpsRuntime::Status g = GpsRuntime::status(now);
        y = drawLine(t, y, g.fresh ? Theme::GREEN : Theme::AMBER, "GPS:",
                     "%s %lu baud", g.fresh ? "FIX" :
                     (g.fix.valid ? "STALE" : "NO FIX"), (unsigned long)GpsRuntime::BAUD);
        y = drawLine(t, y, Theme::CYAN, "UBX:", "%lu OK %lu bad",
                     (unsigned long)g.ubxGood, (unsigned long)g.ubxBad);
        y = drawLine(t, y, Theme::CYAN, "NAV:", "PVT %lu SAT %lu",
                     (unsigned long)g.ubxPvt, (unsigned long)g.ubxSat);
        y = drawLine(t, y, Theme::CYAN, "NMEA:", "%lu OK %lu bad (%lu bytes)",
                     (unsigned long)g.nmeaGood, (unsigned long)g.nmeaBad,
                     (unsigned long)g.bytes);
        y = drawLine(t, y, Theme::CYAN, "SAT:", "%u used %u view %u heard",
                     (unsigned)g.fix.used, (unsigned)g.sky.view, (unsigned)g.sky.heard);
        if (g.fresh) {
            const int64_t la = g.fix.lat7, lo = g.fix.lon7;
            const uint64_t ala = (uint64_t)(la < 0 ? -la : la);
            const uint64_t alo = (uint64_t)(lo < 0 ? -lo : lo);
            y = drawLine(t, y, Theme::GREEN, "LAT:", "%s%lu.%07lu",
                         la < 0 ? "-" : "", (unsigned long)(ala / 10000000ULL),
                         (unsigned long)(ala % 10000000ULL));
            y = drawLine(t, y, Theme::GREEN, "LON:", "%s%lu.%07lu",
                         lo < 0 ? "-" : "", (unsigned long)(alo / 10000000ULL),
                         (unsigned long)(alo % 10000000ULL));
        }
        if (g.fix.epoch) {
            char stamp[24];
            Wardrive::utcStamp(g.fix.epoch, stamp, sizeof stamp);
            y = drawLine(t, y, Theme::CYAN, "UTC:", "%s", stamp);
        }
    }
#endif
    y = drawLine(t, y, Theme::CYAN, "RESET:", "%s", info.resetReason);
    // Only after an actual panic, and only when the breadcrumb survived. A
    // blank line here means a clean boot, not a missing feature.
    if (info.crash.valid || info.crash.haveDump) {
        if (info.crash.valid)
            y = drawLine(t, y, Theme::RED, "LAST CRASH:", "%lum%lus up, %lu free, %lu block",
                         (unsigned long)(info.crash.uptimeMs / 60000),
                         (unsigned long)((info.crash.uptimeMs / 1000) % 60),
                         (unsigned long)info.crash.heapFree,
                         (unsigned long)info.crash.heapBlock);
        else
            y = drawLine(t, y, Theme::RED, "LAST CRASH:", "no breadcrumb survived");
        if (info.crash.haveDump) {
            // Where it died, from the core dump. With these two lines and the
            // ELF of the build that crashed, addr2line names the functions --
            // which is the difference between "it crashed" and a fix.
            y = drawLine(t, y, Theme::RED, "  IN:", "%s @ %08lX%s", info.crash.task,
                         (unsigned long)info.crash.pc, info.crash.dumpOlder ? " (old fw)" : "");
            char bt[40] = "--";
            size_t o = 0;
            for (uint8_t i = 0; i < info.crash.btN && i < 3; i++)
                o += snprintf(bt + o, sizeof bt - o, "%s%08lX", i ? " " : "",
                              (unsigned long)info.crash.bt[i]);
            y = drawLine(t, y, Theme::RED, "  BT:", "%s", bt);
        } else {
            y = drawLine(t, y, Theme::RED, "  ON:", "screen %u, %lu detections",
                         (unsigned)info.crash.screen, (unsigned long)info.crash.lifetime);
        }
    }
    // Uptime next to the reset reason on purpose: together they answer
    // "did this thing restart on me", which is one question and not two.
    {
        char up[24];
        Clock::formatUptime(up, sizeof(up));
        y = drawLine(t, y, Theme::CYAN, "UPTIME:", "%s", up);
        char clk[32];
        Clock::formatClock(clk, sizeof(clk));
        y = drawLine(t, y, Theme::CYAN, "CLOCK:", "%s%s", clk,
                     Clock::trusted() ? "" : "  (TIME <epoch> over serial)");
    }
    y = drawLine(t, y, Theme::CYAN, "HEAP:", "%lu free / %lu largest",
                 (unsigned long)info.freeHeap, (unsigned long)info.largestBlock);
#if SQUACH_MESH
    // Unlike BLE SEEN's old "advertising" flag, this is read from NimBLE's
    // actual controller state. Failed starts can leave the UI looking like
    // SEND worked; show the failure count and last failure stage on-device.
    {
        const Mesh::TxStatus tx = Mesh::txStatus();
        y = drawLine(t, y, tx.advertising ? Theme::GREEN : Theme::AMBER,
                     "MESH TX:", "%s %s, ok %lu / fail %lu (%u)",
                     tx.advertising ? "ON" : "OFF",
                     tx.sendingFrame ? "FRAME" : "IDLE",
                     (unsigned long)tx.starts, (unsigned long)tx.failures,
                     (unsigned)tx.lastFailure);
        const MeshTalk::RxDiagnostics rx = MeshTalk::rxDiagnostics();
        y = drawLine(t, y, Theme::CYAN, "MESH RX:", "%lu queued / %lu heard, full %lu",
                     (unsigned long)rx.queued, (unsigned long)rx.seen,
                     (unsigned long)rx.queueFull);
        y = drawLine(t, y, Theme::CYAN, "INV RX:", "parts %lu mask %x/15 done %lu",
                     (unsigned long)rx.inviteParts, (unsigned)rx.mask,
                     (unsigned long)rx.inviteComplete);
        y = drawLine(t, y, rx.inviteRejected ? Theme::AMBER : Theme::CYAN,
                     "INV ERR:", "%lu rejected (%u), replay %lu",
                     (unsigned long)rx.inviteRejected,
                     (unsigned)rx.lastReject,
                     (unsigned long)rx.replayRejected);
    }
#endif
    // Where the heap went on the way up, in KB: free/largest with WiFi up,
    // with Bluetooth up, and at the first pass of loop().
    {
        const BootHeap bh = bootHeap();
        y = drawLine(t, y, Theme::CYAN, "BOOT:", "wifi %lu/%lu  ble %lu/%lu  loop %lu/%lu KB",
                     (unsigned long)(bh.wifiFree / 1024), (unsigned long)(bh.wifiLargest / 1024),
                     (unsigned long)(bh.bleFree / 1024),  (unsigned long)(bh.bleLargest / 1024),
                     (unsigned long)(info.loopFree / 1024), (unsigned long)(info.loopLargest / 1024));
    }
    y = drawLine(t, y, Theme::CYAN, "SLOT:", "%s  other: %s",
                 info.otaSlot ? info.otaSlot : "?", info.otaOther ? info.otaOther : "none");
    // The black box: what is kept in flash across restarts. The newest crash
    // kept by date and version, which is what a photo of this screen needs
    // to match it to a build; BLACKBOX on the console has the rest.
    if (!info.bbReady) {
        y = drawLine(t, y, Theme::CYAN, "KEPT:", "off (flash space in use)");
    } else if (!info.bbCrashes) {
        y = drawLine(t, y, Theme::CYAN, "KEPT:", "%u seen, no crashes", (unsigned)info.bbKept);
    } else {
        char when[12] = "";
        if (info.bbHaveLast) Clock::formatEpochStamp(info.bbLast.epoch, when, sizeof when);
        // The version only where it fits: portrait is forty characters.
        const bool wide = w >= 300;
        y = drawLine(t, y, Theme::RED, "KEPT:", "%u seen, %u crash%s, last %s%s%s",
                     (unsigned)info.bbKept, (unsigned)info.bbCrashes, info.bbCrashes == 1 ? "" : "es",
                     when, wide ? " v" : "", wide ? info.bbLast.version : "");
    }
    y += 4;

    if (info.hasRaw) {
        y = drawLine(t, y, Theme::VAPOR_PURPLE, "RAW TOUCH:", "%s a=%d b=%d",
                     info.rawTouching ? "DOWN" : "up", info.rawA, info.rawB);
    }
    y = drawLine(t, y, Theme::VAPOR_PURPLE, "MAPPED:", "%s x=%d y=%d",
                 info.touchValid ? "valid" : "--", info.mappedX, info.mappedY);
    y += 4;

    y = drawLine(t, y, Theme::VAPOR_PINK, "CAL SOURCE:", "%s",
                 info.calSource ? info.calSource
                                : info.usingSavedCal ? "saved" : "compiled-in default");
    y = drawLine(t, y, Theme::VAPOR_PINK, "CAL RANGE:", "A[%d,%d] B[%d,%d]",
                 info.calA0, info.calA1, info.calB0, info.calB1);
    y += 4;

    // Frame cost. The push is a fixed byte count over SPI, so it moves
    // only if the bus clock does -- kept as its own figure so a change to
    // SPI_FREQUENCY shows up here unambiguously instead of being averaged
    // into one "it feels smoother" number. fps is derived from the whole
    // frame, not the push alone. On one line with FRAME since the crash
    // report grew a line: the screen already ran into the BACK button.
    {
        uint32_t fus = info.frameUs ? info.frameUs : 1;
        y = drawLine(t, y, Theme::AMBER, "FRAME:", "%lu.%lu ms (%lu fps), push %lu.%lu",
                     (unsigned long)(fus / 1000),
                     (unsigned long)((fus % 1000) / 100),
                     (unsigned long)(1000000UL / fus),
                     (unsigned long)(info.pushUs / 1000),
                     (unsigned long)((info.pushUs % 1000) / 100));
        // FRAME above is this screen, which has no backdrop. LAST is the
        // screen you came from -- its whole frame, which is the frame rate
        // you actually saw there -- and the part of it its background took.
        if (info.lastScreenName && info.lastScreenUs) {
            const uint32_t lus = info.lastScreenUs;
            y = drawLine(t, y, Theme::AMBER, "LAST:", "%s %lu.%lu ms (%lu fps), bg %lu.%lu",
                         info.lastScreenName,
                         (unsigned long)(lus / 1000), (unsigned long)((lus % 1000) / 100),
                         (unsigned long)(1000000UL / lus),
                         (unsigned long)(info.bgUs / 1000), (unsigned long)((info.bgUs % 1000) / 100));
        } else {
            y = drawLine(t, y, Theme::AMBER, "LAST:", "no screen timed yet, bg %lu.%lu",
                         (unsigned long)(info.bgUs / 1000), (unsigned long)((info.bgUs % 1000) / 100));
        }
    }
    y += 4;

    y = drawLine(t, y, Theme::GREEN, "LOG:", "%u entries, %lu lifetime",
                 (unsigned)eng.logCount(), (unsigned long)eng.lifetimeTotal());

#if SQUACH_MESH
    // Phase 0, read without a computer. The device alternates advertising on
    // and off in 30s arms and pools each separately, so leaving it somewhere
    // for ten minutes gives ten samples of each rather than one of a good
    // moment and one of a bad one.
    {
        y += 4;
        const MeshProbe::Stats ms = MeshProbe::stats();
        y = drawLine(t, y, ms.advOn ? Theme::GREEN : Theme::CYAN, "BLE SEEN:", "%u.%u /s, %s%s",
                     (unsigned)(ms.offRate / 10), (unsigned)(ms.offRate % 10),
                     ms.advOn ? "advertising" : "not advertising",
                     scanPassiveNow() ? ", passive" : ", active");
        // The seatbelt's count: adverts refused because the largest free
        // block was under 1.5 KB when they came in. A big number here with
        // a small HEAP line is a board that is only alive because it is
        // ignoring the radio.
        if (advertsDropped())
            y = drawLine(t, y, Theme::VAPOR_PINK, "DROPPED:", "%lu adverts, no heap for them",
                         (unsigned long)advertsDropped());
        // What the once-a-minute scan restart gave back: the leak it closes,
        // measured. This row used to repeat the HEAP line at the top, which
        // already carries free and largest block. Kilobytes a restart in a busy
        // room and a flat heap up top means the leak was NimBLE holding on to
        // devices that never answered; zeros here and a falling heap mean it
        // was not, and the search goes on.
        const ScanFlushStats fs = scanFlushStats();
        y = drawLine(t, y, Theme::VAPOR_PINK, "FLUSHED:", "%lu KB total, %lu B last (x%lu)",
                     (unsigned long)(fs.totalFreed / 1024), (unsigned long)fs.lastFreed,
                     (unsigned long)fs.count);
    }
#endif

    // The fixed title/icon band, report and navigation never overlap.
    // Pages are composed from *complete* measured rows, not pixel offsets.
    const Theme::ButtonBarGeom bar = Theme::computeButtonBar(w, h);
    const int footerTop = bar.y - 20;
    const int bodyTop = Theme::TITLE_ICON_BAND_H + 22; // title below the icons
    const int bodyBottom = footerTop - 5;
    const int availableH = bodyBottom - bodyTop;
    s_diagPageCount = 1;
    s_diagPageStarts[0] = 0;
    int usedH = 0;
    for (int i = 0; i < s_diagRowCount; ++i) {
        const int rh = diagRowHeight(t, s_diagRows[i], w, 0, false);
        if (usedH > 0 && usedH + rh > availableH) {
            s_diagPageStarts[s_diagPageCount++] = (uint8_t)i;
            usedH = 0;
        }
        usedH += rh;
    }
    s_diagPageStarts[s_diagPageCount] = (uint8_t)s_diagRowCount;
    if (s_diagPage >= s_diagPageCount) s_diagPage = s_diagPageCount - 1;

    // IMPORTANT on the CYD 3.2 ST7798: the same half-height sprite is
    // reused for both physical bands and for every diagnostics page.
    // Clearing only [TITLE_ICON_BAND_H, bar.y) left stale glyphs in
    // rows 0..19 (behind the status icons) and in the bottom strip
    // (under BACK). The 2.8 full-frame canvas did not exhibit this.
    // Repaint the entire LOGICAL page inside each band's existing
    // buffered viewport before drawing the current page. The viewport
    // clips this full-screen clear to that band's own sprite rows.
    // Never clear the physical TFT or push an incomplete page.
    t.fillRect(0, 0, w, h, Theme::BG);
    Theme::drawTitleBar(t, ">> DIAGNOSTICS <<");
    t.setTextFont(1);
    t.setTextSize(1);
    t.setTextWrap(false);
    t.setTextColor(Theme::CYAN, Theme::BG);
    t.setCursor(8, Theme::TITLE_ICON_BAND_H + 5);
    t.print("DIAGNOSTICS");

    int rowY = bodyTop;
    for (int i = s_diagPageStarts[s_diagPage];
         i < s_diagPageStarts[s_diagPage + 1]; ++i) {
        rowY += diagRowHeight(t, s_diagRows[i], w, rowY, true);
    }

    // The footer is always visible. Both its labels and the page count
    // remain completely above the pinned BACK button, in either rotation.
    char pager[20];
    snprintf(pager, sizeof pager, "%d/%d", s_diagPage + 1, s_diagPageCount);
    const int footY = footerTop + 6;
    t.setTextColor(s_diagPage > 0 ? Theme::CYAN : Theme::W95_SHADOW, Theme::BG);
    t.setCursor(8, footY);
    t.print("< PREV");
    t.setTextColor(Theme::WHITE, Theme::BG);
    t.setCursor((w - t.textWidth(pager)) / 2, footY);
    t.print(pager);
    t.setTextColor(s_diagPage + 1 < s_diagPageCount ? Theme::CYAN : Theme::W95_SHADOW,
                   Theme::BG);
    const char* next = "NEXT >";
    t.setCursor(w - 8 - t.textWidth(next), footY);
    t.print(next);

    int bx, by, bw, bh;
    backButtonRect(w, h, bx, by, bw, bh);
    Theme::drawButton(t, bx, by, bw, bh, "[ BACK ]", false);
}
