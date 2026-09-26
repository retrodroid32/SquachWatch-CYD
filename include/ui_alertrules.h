// SquachWatch-CYD — per-detection-type alert rule editor
#pragma once
#include <TFT_eSPI.h>
#include <stdint.h>
#include "state.h"

enum class AlertRuleHit : uint8_t {
    NONE = 0,
    BACK,
    TYPE,
    ENABLED,
    CONFIDENCE,
    REPEATS,
    COOLDOWN,
    WAKE
};

void uiAlertRulesInit(TFT_eSPI& t);
void uiAlertRulesTick(TFT_eSPI& t, uint32_t now);
void uiAlertRulesScroll(int delta);
AlertRuleHit uiAlertRulesHitTest(TFT_eSPI& t, int x, int y, int screenW, int screenH);
bool uiAlertRulesInDetail();
void uiAlertRulesBackToList();
DetectionType uiAlertRulesSelected();
