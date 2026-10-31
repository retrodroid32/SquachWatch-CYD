#include "ui_sd_browser.h"
#include "sd_log.h"
#include "theme.h"
#include <SD.h>
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

namespace SdBrowser {
namespace {
struct Entry { char name[56]; uint32_t bytes; bool dir; };
static Entry entries[5];     // five rows fit the smallest 240px screen
static uint8_t count = 0;
static uint16_t offset = 0;
static bool more = false;
static char folder[96] = "/";
static SdLog* logger = nullptr;
static bool preview = false, askDelete = false;
static uint8_t wipeStage = 0; // 0 none, 1 confirm, 2 final confirmation
static uint8_t wipeResult = 0; // 0 none, 1 success, 2 partial/failed
static char chosen[96] = "";
static char title[56] = "";
static char lines[7][54];
static uint32_t previewPos = 0, previewNext = 0;
static uint16_t previewShift = 0;
static uint8_t previewCols = 37, previewRows = 5;
static bool previewMore = false, previewRight = false;
static bool error = false;
struct CardGuard {
    SdLog* log;
    bool locked;
    explicit CardGuard(SdLog* s) : log(s), locked(s && s->browseLock()) {}
    ~CardGuard() { if (locked) log->browseUnlock(); }
};

bool joined(const char* name, char* out, size_t cap) {
    if (!name || !*name || strstr(name, "..") || strchr(name, '\\')) return false;
    const char* base = strrchr(name, '/');
    base = base ? base + 1 : name;
    if (!*base || strlen(base) >= 55) return false;
    const int n = strcmp(folder, "/") == 0
        ? snprintf(out, cap, "/%s", base)
        : snprintf(out, cap, "%s/%s", folder, base);
    return n > 0 && (size_t)n < cap;
}
const char* baseName(const char* path) {
    const char* p = strrchr(path, '/');
    return p ? p + 1 : path;
}
bool list() {
    count = 0; more = false; error = false;
    if (!logger || !logger->ready()) return false;
    CardGuard guard(logger);
    if (!guard.locked) { error = true; return false; }
    if (!logger->ready()) return false;
    File d = SD.open(folder);
    if (!d || !d.isDirectory()) {
        if (d) d.close();
        logger->browseFault();
        error = true;
        return false;
    }
    uint16_t i = 0;
    while (File f = d.openNextFile()) {
        if (i++ < offset) { f.close(); continue; }
        if (count >= 5) { more = true; f.close(); break; }
        char path[96];
        if (joined(f.name(), path, sizeof path)) {
            Entry& e = entries[count++];
            snprintf(e.name, sizeof e.name, "%s", baseName(path));
            e.dir = f.isDirectory();
            e.bytes = e.dir ? 0 : (uint32_t)f.size();
        }
        f.close();
    }
    d.close();
    return true;
}
// Keep one physical log line per row. Read a horizontal window of each line.
void loadPreview() {
    memset(lines, 0, sizeof lines);
    previewMore = previewRight = false;
    if (!logger || !logger->ready()) return;
    CardGuard guard(logger);
    if (!guard.locked) { error = true; return; }
    File f = SD.open(chosen, FILE_READ);
    if (!f || f.isDirectory()) {
        if (f) f.close(); error = true; logger->browseFault(); return;
    }
    if (!f.seek(previewPos)) {
        error = true; f.close(); logger->browseFault(); return;
    }
    for (unsigned row = 0; row < previewRows && f.available(); ++row) {
        uint32_t col = 0;
        // Safety cap for files with no newline. Typical log rows are <128 chars.
        while (f.available() && col < 8192) {
            const int ch = f.read();
            if (ch < 0) { error = true; break; }
            if (ch == '\n') break;
            if (ch == '\r') continue;
            if (col >= previewShift && col < (uint32_t)previewShift + previewCols)
                lines[row][col - previewShift] =
                    (ch >= 32 && ch < 127) ? (char)ch : '.';
            else if (col >= (uint32_t)previewShift + previewCols)
                previewRight = true;
            ++col;
        }
        if (error) break;
    }
    previewNext = (uint32_t)f.position();
    previewMore = f.available() > 0;
    f.close();
}
void smallLine(TFT_eSPI& t, int x, int y, const char* s, uint16_t c) {
    t.setTextFont(1); t.setTextSize(1); t.setTextWrap(false);
    t.setTextColor(c, Theme::BG); t.setCursor(x, y); t.print(s);
    t.setTextWrap(true);
}
void line(TFT_eSPI& t, int x, int y, const char* s, uint16_t color) {
    t.setTextFont(1); t.setTextSize(2);
    t.setTextColor(color, Theme::BG);
    t.setCursor(x, y); t.print(s);
}
bool bottom(int y, int h) { return y >= h - 29; }
} // namespace

void begin(SdLog& sd) {
    logger = &sd; sd.browseBegin();
    strcpy(folder, "/");
    offset = 0; preview = askDelete = false; previewShift = 0;
    wipeStage = wipeResult = 0;
    chosen[0] = 0; error = false;
    list();
}
void end() { if (logger) logger->browseEnd(); logger = nullptr; }
bool reloadMountedCard() {
    if (!logger || !logger->ready()) return false;
    strcpy(folder, "/");
    chosen[0] = 0;
    preview = askDelete = false;
    wipeStage = wipeResult = 0;
    offset = 0;
    return list();
}

void draw(TFT_eSPI& t) {
    const int w = t.width(), h = t.height();
    t.fillScreen(Theme::BG);
    Theme::drawTitleBar(t, "SD FILES");
    line(t, 7, 26, wipeStage ? "WIPE SD" : preview ? "FILE VIEW" : "SD FILES", Theme::CYAN);
    if (wipeResult) {
        line(t, 8, 60, wipeResult == 1 ? "FILES DELETED" : "WIPE INCOMPLETE",
             wipeResult == 1 ? Theme::GREEN : Theme::RED);
        smallLine(t, 8, 90, logger->wipeError(), Theme::WHITE);
    } else if (wipeStage) {
        line(t, 8, 57, wipeStage == 1 ? "DELETE ALL FILES?" : "DELETE EVERYTHING?", Theme::RED);
        smallLine(t, 8, 81, wipeStage == 1 ? "Files and folders will be deleted." : "Keeps FAT. Cannot undo!", Theme::WHITE);
        line(t, 8, 102, wipeStage == 1 ? "Continue?" : "Final confirmation", Theme::AMBER);
        Theme::drawButton(t, 4, h - 62, w / 2 - 8, 28, "CANCEL", false, 1);
        Theme::drawButton(t, w / 2 + 4, h - 62, w / 2 - 8, 28,
                          wipeStage == 1 ? "NEXT" : "DELETE", false, 1);
    } else if (!logger || !logger->ready()) {
        line(t, 8, 55, "SD OFFLINE", Theme::RED);
        line(t, 8, 78, logger && logger->remounting() ? "REMOUNTING..." :
             logger ? logger->statusDetail() : "NO SD LOGGER", Theme::AMBER);
        // Show actual mount attempt count for a useful on-device failure
        // report even if USB serial is not connected.
        if (logger) {
            char attempts[40];
            snprintf(attempts, sizeof attempts, "ATTEMPTS: %lu",
                     (unsigned long)logger->mountAttempts());
            line(t, 8, 103, attempts, Theme::CYAN);
            if (logger->probeFailures()) {
                char probe[40];
                snprintf(probe, sizeof probe, "CMD0 FAILS: %lu",
                         (unsigned long)logger->probeFailures());
                line(t, 8, 126, probe, Theme::AMBER);
            }
        }
        line(t, 8, h - 59, "NO WIPE", Theme::AMBER);
        line(t, w / 2, h - 59, "RESCAN >", Theme::CYAN);
    } else if (askDelete) {
        line(t, 8, 54, "DELETE THIS FILE?", Theme::RED);
        line(t, 8, 77, title, Theme::WHITE);
        Theme::drawButton(t, 4, h - 62, w / 2 - 8, 28, "CANCEL", false, 1);
        Theme::drawButton(t, w / 2 + 4, h - 62, w / 2 - 8, 28, "DELETE", false, 1);
    } else if (preview) {
        const uint8_t cols = (uint8_t)((w - 16) / 6 < 53 ? (w - 16) / 6 : 53);
        const uint8_t rows = h < 280 ? 5 : 7;
        if (previewCols != cols || previewRows != rows) {
            previewCols = cols; previewRows = rows; loadPreview();
        }
        smallLine(t, 8, 48, title, Theme::AMBER);
        for (unsigned i = 0; i < previewRows; ++i)
            smallLine(t, 8, 69 + 14 * i, lines[i], Theme::WHITE);
        char range[30];
        snprintf(range, sizeof range, "CHARS %u-%u",
                 (unsigned)previewShift + 1, (unsigned)previewShift + previewCols);
        smallLine(t, 8, h - 111, range, Theme::CYAN);
        Theme::drawButton(t, 4, h - 96, w / 2 - 8, 26,
                          previewShift ? "< LEFT" : "< START", false, 1);
        Theme::drawButton(t, w / 2 + 4, h - 96, w / 2 - 8, 26,
                          previewRight ? "RIGHT >" : "END >", false, 1);
        Theme::drawButton(t, 4, h - 64, w / 2 - 8, 28, "DELETE", false, 1);
        Theme::drawButton(t, w / 2 + 4, h - 64, w / 2 - 8, 28,
                          previewMore ? "NEXT PAGE" : "END", false, 1);
    } else {
        line(t, 8, 48, folder, Theme::AMBER);
        const int rows = (h - 116) / 22 < 5 ? (h - 116) / 22 : 5;
        for (int i = 0; i < rows && i < count; i++) {
            char label[41];
            snprintf(label, sizeof label, "%s %.31s", entries[i].dir ? ">" : "-", entries[i].name);
            line(t, 8, 70 + 22 * i, label, entries[i].dir ? Theme::CYAN : Theme::WHITE);
        }
        if (error) line(t, 8, 73, "READ ERROR", Theme::RED);
        line(t, 8, h - 59, "WIPE ALL", Theme::AMBER);
        if (offset) line(t, w / 2 - 55, h - 59, "< PREV", Theme::CYAN);
        if (more) line(t, w - 75, h - 59, "NEXT >", Theme::CYAN);
    }
    Theme::drawButton(t, 0, h - 30, w, 30, "[ BACK ]", false, 1);
}

bool tap(int x, int y, int w, int h) {
    if (!logger) return true;
    if (bottom(y, h)) {
        if (wipeStage) { wipeStage = 0; return false; }
        if (wipeResult) { wipeResult = 0; list(); return false; }
        if (askDelete) { askDelete = false; return false; }
        if (preview) { preview = false; list(); return false; }
        if (strcmp(folder, "/") != 0) {
            char* slash = strrchr(folder, '/');
            if (slash == folder) folder[1] = '\0';
            else if (slash) *slash = '\0';
            offset = 0; list(); return false;
        }
        return true;
    }
    if (wipeResult) return false;
    if (wipeStage) {
        if (y >= h - 66 && y < h - 30) {
            if (x < w / 2) wipeStage = 0;
            else if (wipeStage == 1) wipeStage = 2;
            else {
                const bool ok = logger->wipeAllFiles();
                wipeStage = 0;
                wipeResult = ok ? 1 : 2;
                if (ok) {
                    strcpy(folder, "/"); offset = 0;
                    preview = askDelete = false; chosen[0] = 0;
                    list();
                }
            }
        }
        return false;
    }
    // File wiping needs a mounted FAT filesystem; when offline only RESCAN.
    if (!logger->ready()) {
        if (y >= h - 68 && y < h - 30 && x >= w / 2) logger->rescan();
        return false;
    }
    if (askDelete) {
        if (y >= h - 64 && y < h - 32) {
            if (x >= w / 2 && chosen[0]) {
                CardGuard guard(logger);
                if (!guard.locked || !logger->ready()) error = true;
                else if (!SD.remove(chosen)) { error = true; logger->browseFault(); }
                else { preview = false; offset = 0; }
            }
            askDelete = false;
            list();
        }
        return false;
    }
    if (preview) {
        if (y >= h - 100 && y < h - 67) {
            const uint16_t step = previewCols > 12 ? previewCols - 8 : 8;
            if (x < w / 2) {
                previewShift = previewShift > step ? previewShift - step : 0;
                loadPreview();
            } else if (previewRight && previewShift < 8192 - step) {
                previewShift += step;
                loadPreview();
            }
            return false;
        }
        if (y >= h - 66 && y < h - 30) {
            if (x < w / 2) { askDelete = true; return false; }
            if (previewMore) {
                previewPos = previewNext; previewShift = 0;
                loadPreview();
            }
        }
        return false;
    }
    if (y >= h - 65 && y < h - 31) {
        if (x < w / 3) { wipeStage = 1; return false; }
        if (x < 2 * w / 3 && offset >= 5) offset -= 5;
        else if (x >= 2 * w / 3 && more) offset += 5;
        list(); return false;
    }
    const int row = (y - 70) / 22;
    const int rows = (h - 116) / 22 < 5 ? (h - 116) / 22 : 5;
    if (y < 70 || row < 0 || row >= rows || row >= count) return false;
    char next[96];
    if (!joined(entries[row].name, next, sizeof next)) return false;
    if (entries[row].dir) {
        snprintf(folder, sizeof folder, "%s", next);
        offset = 0; list();
    } else {
        snprintf(chosen, sizeof chosen, "%s", next);
        snprintf(title, sizeof title, "%s", entries[row].name);
        previewPos = 0; previewShift = 0; preview = true; error = false;
        previewCols = (uint8_t)((w - 16) / 6 < 53 ? (w - 16) / 6 : 53);
        previewRows = h < 280 ? 5 : 7;
        loadPreview();
    }
    return false;
}
} // namespace SdBrowser
