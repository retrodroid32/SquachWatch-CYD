#include "wifi_use_policy.h"
#include <cassert>
#include <cstdio>

int main() {
    // A rendered WiFi screen is already present on the LCD when USE loans
    // the framebuffer. A touch edge alone must not blank/repaint it.
    assert(!WifiScreenPolicy::networksNeedsFullPaint(false, true, true));
    assert(!WifiScreenPolicy::networksNeedsFullPaint(true, true, true));
    assert(WifiScreenPolicy::networksNeedsFullPaint(false, false, true));
    assert(WifiScreenPolicy::networksNeedsFullPaint(false, true, false));
    assert(!WifiScreenPolicy::networksRowChanged(2, 1, 1, 0, 0, false, false));
    // Only selection, USE or result state changes should repaint a row.
    assert(WifiScreenPolicy::networksRowChanged(1, 1, 2, 0, 0, false, false));
    assert(WifiScreenPolicy::networksRowChanged(2, 1, 2, 0, 0, false, false));
    assert(WifiScreenPolicy::networksRowChanged(3, 1, 1, 0, 0, false, true));
    assert(!WifiScreenPolicy::networksRowChanged(3, 1, 2, 0, 0, false, false));
    assert(WifiScreenPolicy::networksRowChanged(0, 1, 1, 0, 0, true, false));

#if defined(CYD32)
    static_assert(!WifiUsePolicy::SCAN_BEFORE_JOIN, "ST7798 should skip prerequisite WiFi scan");
    static_assert(!WifiUsePolicy::BREAK_ON_EARLY_ERROR, "ST7798 must wait for radio handoff");
    static_assert(!WifiUsePolicy::GENERIC_CONNECT_FAILED_IS_AUTH, "do not mislabel transient failure");
    assert(WifiUsePolicy::retry(false, false, false, false));
    assert(WifiUsePolicy::retry(false, true, true, false));
    assert(!WifiUsePolicy::retry(false, false, false, true));
    assert(!WifiUsePolicy::retry(true, true, true, false));
    assert(!WifiScreenPolicy::keyboardDirectPanel(true, 1));
    assert(WifiScreenPolicy::keyboardDirectPanel(true, 2));
    assert(WifiScreenPolicy::keyboardDirectPanel(true, 4));
    assert(WifiScreenPolicy::keyboardDirectPanel(true, 16));
    assert(WifiScreenPolicy::keyboardDirectPanel(false, 1));
    assert(WifiScreenPolicy::keyboardDirectPanel(false, 2));
    // Direct LCD must not erase/repaint all 40 keys during the 400ms
    // settle period; buffered screens preserve their old behavior.
    assert(WifiScreenPolicy::passwordFullPaint(true, true, 10, 400));
    assert(!WifiScreenPolicy::passwordFullPaint(false, true, 10, 400));
    assert(!WifiScreenPolicy::passwordFullPaint(false, true, 399, 400));
    assert(WifiScreenPolicy::passwordFullPaint(false, false, 10, 400));
    assert(!WifiScreenPolicy::passwordFullPaint(false, false, 400, 400));
    assert(!WifiScreenPolicy::networksDirectPanel(true, 1));
    assert(WifiScreenPolicy::networksDirectPanel(false, 1));
    assert(WifiScreenPolicy::networksDirectPanel(true, 2));
    assert(WifiScreenPolicy::networksDirectPanel(true, 16));
    puts("PASS: ST7798 WiFi USE and partial-buffer UI render policy");
#else
    static_assert(WifiUsePolicy::SCAN_BEFORE_JOIN, "2.8 existing scan path retained");
    static_assert(WifiUsePolicy::BREAK_ON_EARLY_ERROR, "2.8 early-error path retained");
    static_assert(WifiUsePolicy::GENERIC_CONNECT_FAILED_IS_AUTH, "2.8 auth classification retained");
    assert(WifiUsePolicy::retry(false, true, true, false));
    assert(!WifiUsePolicy::retry(false, false, true, false));
    assert(!WifiUsePolicy::retry(false, true, false, false));
    assert(!WifiUsePolicy::retry(true, true, true, false));
    const uint8_t bandCounts[] = { 1, 2, 4, 16 };
    for (uint8_t bands : bandCounts) {
        assert(!WifiScreenPolicy::keyboardDirectPanel(true, bands));
        assert(!WifiScreenPolicy::networksDirectPanel(true, bands));
    }
    assert(!WifiScreenPolicy::networksDirectPanel(false, 1));
    // 2.8 uses only the buffered path: unchanged 400ms settle semantics.
    assert(WifiScreenPolicy::passwordFullPaint(true, false, 0, 400));
    assert(WifiScreenPolicy::passwordFullPaint(false, false, 399, 400));
    assert(!WifiScreenPolicy::passwordFullPaint(false, false, 400, 400));
    puts("PASS: 2.8 WiFi USE and UI render policy unchanged");
#endif
    return 0;
}
