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
static uint8_t formatStage = 0; // 0 none, 1 confirm, 2 irreversible confirmation
static uint8_t formatResult = 0; // 0 none, 1 formatted, 2 failed
static char chosen[96] = "";
static char title[56] = "";
static char lines[7][33];
static uint32_t previewPos = 0, previewNext = 0;
static bool previewMore = false;
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
void list() {
    count = 0; more = false; error = false;
    if (!logger || !logger->ready()) return;
    CardGuard guard(logger);
    if (!guard.locked) { error = true; return; }
    if (!logger->ready()) return;
    File d = SD.open(folder);
    if (!d || !d.isDirectory()) {
        if (d) d.close();
        logger->browseFault();
        error = true;
        return;
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
}
void loadPreview() {
    memset(lines, 0, sizeof lines);
    previewMore = false;
    if (!logger || !logger->ready()) return;
    CardGuard guard(logger);
    if (!guard.locked) { error = true; return; }
    if (!logger->ready()) return;
    File f = SD.open(chosen, FILE_READ);
    if (!f || f.isDirectory()) { if (f) f.close(); logger->browseFault(); error = true; return; }
    if (!f.seek(previewPos)) { error = true; f.close(); logger->browseFault(); return; }
    int line = 0, col = 0;
    while (line < 7 && f.available()) {
        const int v = f.read();
        if (v < 0) break;
        if (v == '\r') continue;
        if (v == '\n' || col == 32) {
            if (v != '\n' && col == 32) f.seek(f.position() - 1);
            line++; col = 0;
        } else {
            lines[line][col++] = (v >= 32 && v < 127) ? (char)v : '.';
        }
    }
    previewNext = (uint32_t)f.position();
    previewMore = f.available() > 0;
    f.close();
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
    offset = 0; preview = askDelete = false;
    formatStage = formatResult = 0;
    chosen[0] = 0; error = false;
    list();
}
void end() { if (logger) logger->browseEnd(); logger = nullptr; }

void draw(TFT_eSPI& t) {
    const int w = t.width(), h = t.height();
    t.fillScreen(Theme::BG);
    Theme::drawTitleBar(t, "SD FILES");
    line(t, 7, 26, formatStage ? "FORMAT SD" : preview ? "FILE VIEW" : "SD FILES", Theme::CYAN);
    if (formatResult) {
        line(t, 8, 60, formatResult == 1 ? "FORMAT SUCCESS" : "FORMAT FAILED",
             formatResult == 1 ? Theme::GREEN : Theme::RED);
        line(t, 8, 85, formatResult == 1 ? "Card is ready" : logger->formatError(), Theme::WHITE);
    } else if (!logger || !logger->ready()) {
        line(t, 8, 55, "SD OFFLINE", Theme::RED);
        line(t, 8, 78, "Insert SD card", Theme::AMBER);
        line(t, 8, h - 59, "RESCAN >", Theme::CYAN);
    } else if (formatStage) {
        line(t, 8, 57, formatStage == 1 ? "FORMAT SD CARD?" : "ERASE ALL FILES?", Theme::RED);
        line(t, 8, 81, formatStage == 1 ? "All data will be lost" : "Cannot be undone!", Theme::WHITE);
        line(t, 8, 102, formatStage == 1 ? "Continue?" : "Final confirmation", Theme::AMBER);
        Theme::drawButton(t, 4, h - 62, w / 2 - 8, 28, "CANCEL", false, 1);
        Theme::drawButton(t, w / 2 + 4, h - 62, w / 2 - 8, 28,
                          formatStage == 1 ? "NEXT" : "FORMAT", false, 1);
    } else if (askDelete) {
        line(t, 8, 54, "DELETE THIS FILE?", Theme::RED);
        line(t, 8, 77, title, Theme::WHITE);
        Theme::drawButton(t, 4, h - 62, w / 2 - 8, 28, "CANCEL", false, 1);
        Theme::drawButton(t, w / 2 + 4, h - 62, w / 2 - 8, 28, "DELETE", false, 1);
    } else if (preview) {
        line(t, 8, 48, title, Theme::AMBER);
        const int rows = (h - 120) / 16 < 7 ? (h - 120) / 16 : 7;
        for (int i = 0; i < rows; i++)
            line(t, 8, 69 + 16 * i, lines[i], Theme::WHITE);
        line(t, 8, h - 59, "DEL", Theme::RED);
        line(t, w / 2, h - 59, previewMore ? "NEXT >" : "END", Theme::CYAN);
    } else {
        line(t, 8, 48, folder, Theme::AMBER);
        const int rows = (h - 116) / 22 < 5 ? (h - 116) / 22 : 5;
        for (int i = 0; i < rows && i < count; i++) {
            char label[41];
            snprintf(label, sizeof label, "%s %.31s", entries[i].dir ? ">" : "-", entries[i].name);
            line(t, 8, 70 + 22 * i, label, entries[i].dir ? Theme::CYAN : Theme::WHITE);
        }
        if (error) line(t, 8, 73, "READ ERROR", Theme::RED);
        line(t, 8, h - 59, "FORMAT", Theme::AMBER);
        if (offset) line(t, w / 2 - 55, h - 59, "< PREV", Theme::CYAN);
        if (more) line(t, w - 75, h - 59, "NEXT >", Theme::CYAN);
    }
    Theme::drawButton(t, 0, h - 30, w, 30, "[ BACK ]", false, 1);
}

bool tap(int x, int y, int w, int h) {
    if (!logger) return true;
    if (bottom(y, h)) {
        if (formatStage) { formatStage = 0; return false; }
        if (formatResult) { formatResult = 0; list(); return false; }
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
    if (!logger->ready()) {
        // An explicit RESCAN works even when the monitor is unavailable.
        if (y >= h - 68 && y < h - 30) {
            logger->rescan(); // async; monitor remounts, screen auto-refreshes
        }
        return false;
    }
    if (formatResult) return false;
    if (formatStage) {
        if (y >= h - 66 && y < h - 30) {
            if (x < w / 2) formatStage = 0;
            else if (formatStage == 1) formatStage = 2;
            else {
                const bool ok = logger->formatCard();
                formatStage = 0;
                formatResult = ok ? 1 : 2;
                if (ok) {
                    strcpy(folder, "/"); offset = 0;
                    preview = askDelete = false; chosen[0] = 0;
                    list();
                }
            }
        }
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
        if (y >= h - 68 && y < h - 31) {
            if (x < w / 2) { askDelete = true; return false; }
            if (previewMore) { previewPos = previewNext; loadPreview(); }
        }
        return false;
    }
    if (y >= h - 65 && y < h - 31) {
        if (x < w / 3) { formatStage = 1; return false; }
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
        previewPos = 0; preview = true; error = false; loadPreview();
    }
    return false;
}
} // namespace SdBrowser
