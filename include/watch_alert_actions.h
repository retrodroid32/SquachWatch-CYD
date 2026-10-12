// Shared, host-testable WATCH alert navigation and add-only action.
// In particular: the ordinary ALERT WATCH button must never toggle an
// already-watched device off, and manually opened WATCH alerts must go
// back to their roster rather than the main screen.
#pragma once
#include "detection.h"
#include <stdint.h>

namespace WatchAlertActions {

enum class ReturnPage : uint8_t { HOME, ROSTER };
inline ReturnPage returnPage(bool openedFromRoster) {
    return openedFromRoster ? ReturnPage::ROSTER : ReturnPage::HOME;
}

enum class AddResult : uint8_t { ADDED, ALREADY_WATCHED, FULL };
inline AddResult addOnly(DetectionEngine& engine, const uint8_t* mac,
                         const char* label, bool isBle) {
    if (!mac) return AddResult::FULL;
    if (engine.isWatched(mac, isBle)) return AddResult::ALREADY_WATCHED;
    const DetectionEngine::WatchToggle result = isBle
        ? engine.toggleWatchBle(mac, label)
        : engine.toggleWatchWifi(mac, label);
    if (result == DetectionEngine::WatchToggle::ADDED) return AddResult::ADDED;
    // This path cannot normally return REMOVED because isWatched was
    // checked, but fail closed rather than report a successful add.
    return AddResult::FULL;
}

} // namespace WatchAlertActions
