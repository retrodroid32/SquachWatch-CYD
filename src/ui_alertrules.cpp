// SquachWatch-CYD — per-detection-type alert rule editor
#include "ui_alertrules.h"
#include "ui_scroll.h"
#include "theme.h"
#include "settings.h"
#include <Arduino.h>
#include <stdio.h>

static int g_scroll = 0;
static bool g_detail = false;
static DetectionType g_selected = DetectionType::FLOCK;

static const int TOP = 16;
static const int ROW_H = 28;

static int bodyTop() { return TOP + Theme::LIST_HEADING_H; }
static int bodyBottom(int w, int h) { return h - Theme::pinnedBackH(w) - 2; }

void uiAlertRulesInit(TFT_eSPI& t) {
    g_scroll = 0;
    g_detail = false;
    t.fillRect(0, 0, t.width(), t.height(), Theme::BG);
}

void uiAlertRulesScroll(int delta) {
    if (g_detail) return;
    g_scroll += delta;
    if (g_scroll < 0) g_scroll = 0;
}

bool uiAlertRulesInDetail() { return g_detail; }
DetectionType uiAlertRulesSelected() { return g_selected; }
void uiAlertRulesBackToList() { g_detail = false; g_scroll = 0; }

static void drawListRow(TFT_eSPI& t, int y, DetectionType ty) {
    const int w = t.width();
    t.setTextSize(1);
    t.setTextColor(Theme::colorFor(ty), Theme::BG);
    t.setCursor(7, y + 5);
    t.print(detectionTypeDisplayName(ty));

    const char* v = Settings::alertRuleSummary(ty);
    t.setTextColor(Settings::alertEnabled(ty) ? Theme::WHITE : Theme::W95_SHADOW, Theme::BG);
    t.setCursor(w - 8 - t.textWidth(v), y + 16);
    t.print(v);
    t.drawFastHLine(5, y + ROW_H - 1, w - 10, Theme::PURPLE);
}

static const char* onOff(bool v) { return v ? "ON" : "OFF"; }

static void drawDetailRow(TFT_eSPI& t, int y, const char* label, const char* value, uint16_t c) {
    const int w = t.width();
    t.setTextSize(1);
    t.setTextColor(c, Theme::BG);
    t.setCursor(8, y + 9);
    t.print(label);
    t.setTextColor(Theme::WHITE, Theme::BG);
    t.setCursor(w - 8 - t.textWidth(value), y + 9);
    t.print(value);
    t.drawFastHLine(5, y + ROW_H - 1, w - 10, Theme::PURPLE);
}

void uiAlertRulesTick(TFT_eSPI& t, uint32_t now) {
    (void)now;
    const int w = t.width(), h = t.height();
    t.fillRect(0, 0, w, h, Theme::BG);
    Theme::drawTitleBar(t, g_detail ? detectionTypeDisplayName(g_selected) : ">> ALERT RULES <<");
    Theme::drawPinnedBack(t, "[ BACK ]");

    if (!g_detail) {
        Theme::drawListHeading(t, "PER-TYPE ALERTS", Theme::AMBER);
        const int top = bodyTop(), bottom = bodyBottom(w, h);
        const int n = (int)DetectionType::COUNT - 1;
        uiClampScroll(g_scroll, n, bottom - top, ROW_H);
        int y = top;
        for (int row = g_scroll; row < n && y + ROW_H <= bottom; row++, y += ROW_H) {
            drawListRow(t, y, (DetectionType)(row + 1));
        }
        const int visible = (bottom - top) / ROW_H;
        if (n > visible) Theme::drawScrollbar(t, w - 3, top, bottom - top, n, visible, g_scroll);
        return;
    }

    Theme::drawListHeading(t, "RULE", Theme::colorFor(g_selected));
    int y = bodyTop();
    drawDetailRow(t, y, "FULL ALERT", onOff(Settings::alertEnabled(g_selected)), Theme::AMBER); y += ROW_H;
    drawDetailRow(t, y, "MIN CONF", Settings::alertRuleConfidenceLabel(g_selected), Theme::CYAN); y += ROW_H;
    char rep[12];
    snprintf(rep, sizeof rep, "%u HITS", (unsigned)Settings::alertMinRepeats(g_selected));
    drawDetailRow(t, y, "MIN REPEATS", rep, Theme::CYAN); y += ROW_H;
    drawDetailRow(t, y, "COOLDOWN", Settings::alertRuleCooldownLabel(g_selected), Theme::VAPOR_PURPLE); y += ROW_H;
    drawDetailRow(t, y, "WAKE SCREEN", onOff(Settings::alertWakeScreen(g_selected)), Theme::GREEN); y += ROW_H;
#if defined(TWATCH_S3)
    drawDetailRow(t, y, "HAPTIC", onOff(Settings::alertNotify(g_selected)), Theme::VAPOR_PINK); y += ROW_H;
#endif

    t.setTextSize(1);
    t.setTextColor(Theme::W95_SHADOW, Theme::BG);
    const char* hint = Settings::alertEnabled(g_selected)
                     ? "Detection still logs when a rule blocks its alert."
                     : "LOG ONLY: detection stays counted and recorded.";
    t.setCursor(7, bodyBottom(w, h) - t.fontHeight() - 5);
    // Keep this intentionally clipped rather than wrapping into BACK on 240px.
    t.print(hint);
}

AlertRuleHit uiAlertRulesHitTest(TFT_eSPI& t, int x, int y, int screenW, int screenH) {
    if (Theme::pinnedBackHit(x, y, screenW, screenH)) return AlertRuleHit::BACK;
    const int top = bodyTop(), bottom = bodyBottom(screenW, screenH);
    if (y < top || y >= bottom) return AlertRuleHit::NONE;

    if (!g_detail) {
        const int n = (int)DetectionType::COUNT - 1;
        uiClampScroll(g_scroll, n, bottom - top, ROW_H);
        const int visibleRow = (y - top) / ROW_H;
        const int row = g_scroll + visibleRow;
        if (row < 0 || row >= n) return AlertRuleHit::NONE;
        g_selected = (DetectionType)(row + 1);
        g_detail = true;
        (void)t;
        return AlertRuleHit::TYPE;
    }

    const int row = (y - top) / ROW_H;
    switch (row) {
        case 0: return AlertRuleHit::ENABLED;
        case 1: return AlertRuleHit::CONFIDENCE;
        case 2: return AlertRuleHit::REPEATS;
        case 3: return AlertRuleHit::COOLDOWN;
        case 4: return AlertRuleHit::WAKE;
#if defined(TWATCH_S3)
        case 5: return AlertRuleHit::NOTIFY;
#endif
        default: return AlertRuleHit::NONE;
    }
}
