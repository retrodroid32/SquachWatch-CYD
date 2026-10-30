// WiFi USE choices are deliberately board-specific.
// CYD32/ST7798 fails in the pre-join survey/BSSID-pinned path while the
// same saved SSID succeeds in bootCheck's direct station join.
// The established 2.8-inch protocol remains unchanged.
#pragma once

namespace WifiUsePolicy {
#if defined(CYD32)
static constexpr bool SCAN_BEFORE_JOIN = false;
static constexpr bool BREAK_ON_EARLY_ERROR = false;
static constexpr bool GENERIC_CONNECT_FAILED_IS_AUTH = false;
inline bool retry(bool connected, bool apSeen, bool absentAp, bool authRejected) {
    (void)apSeen; (void)absentAp;
    return !connected && !authRejected;
}
#else
static constexpr bool SCAN_BEFORE_JOIN = true;
static constexpr bool BREAK_ON_EARLY_ERROR = true;
static constexpr bool GENERIC_CONNECT_FAILED_IS_AUTH = true;
inline bool retry(bool connected, bool apSeen, bool absentAp, bool authRejected) {
    (void)authRejected;
    return !connected && apSeen && absentAp;
}
#endif
}

#include <stdint.h>

namespace WifiScreenPolicy {
// Incremental keyboard painting is stateful, and measures the rendering
// canvas's height. The recovery sprite covers only a band, not the full
// display, so it must never receive a full-screen keyboard layout pass.
inline bool keyboardDirectPanel(bool framebufferReady, uint8_t bands) {
#if defined(CYD32)
    return framebufferReady && bands > 1;
#else
    (void)framebufferReady; (void)bands;
    return false;  // preserve working 2.8-inch rendering path
#endif
}

// After saved USE, the framebuffer may have been lent out or recovered in
// narrow bands. A network list doesn't need 30 full-screen repaints/sec:
// paint straight to the display when the selected row or test state changes.
// The full-screen image was just painted into the framebuffer before USE
// lent its RAM. The physical LCD still holds that scene; adopt it instead of
// clearing the whole panel on the first direct-rendered frame.
inline bool networksNeedsFullPaint(bool directReady, bool physicalSceneKnown,
                                   bool sizeMatches) {
    return !sizeMatches || (!directReady && !physicalSceneKnown);
}

// A tap without a selection/use/status change must not redraw a network row.
inline bool networksRowChanged(int row, int selected, int priorSelected,
                               int use, int priorUse, bool testChanged,
                               bool resultChanged) {
    if (resultChanged) return true;
    if (selected == priorSelected && use == priorUse && !testChanged) return false;
    return row == selected || row == priorSelected ||
           row == use || row == priorUse;
}

inline bool networksDirectPanel(bool framebufferReady, uint8_t bands) {
#if defined(CYD32)
    return !framebufferReady || bands > 1;
#else
    (void)framebufferReady; (void)bands;
    return false;
#endif
}
}
