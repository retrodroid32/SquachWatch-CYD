#include "wifi_use_policy.h"
#include <cassert>
#include <cstdio>

int main() {
#if defined(CYD32)
    static_assert(!WifiUsePolicy::SCAN_BEFORE_JOIN, "ST7798 should skip prerequisite WiFi scan");
    static_assert(!WifiUsePolicy::BREAK_ON_EARLY_ERROR, "ST7798 must wait for radio handoff");
    static_assert(!WifiUsePolicy::GENERIC_CONNECT_FAILED_IS_AUTH, "do not mislabel transient failure");
    assert(WifiUsePolicy::retry(false, false, false, false));
    assert(WifiUsePolicy::retry(false, true, true, false));
    assert(!WifiUsePolicy::retry(false, false, false, true));
    assert(!WifiUsePolicy::retry(true, true, true, false));
    puts("PASS: ST7798 WiFi USE direct join and bounded retry");
#else
    static_assert(WifiUsePolicy::SCAN_BEFORE_JOIN, "2.8 existing scan path retained");
    static_assert(WifiUsePolicy::BREAK_ON_EARLY_ERROR, "2.8 early-error path retained");
    static_assert(WifiUsePolicy::GENERIC_CONNECT_FAILED_IS_AUTH, "2.8 auth classification retained");
    assert(WifiUsePolicy::retry(false, true, true, false));
    assert(!WifiUsePolicy::retry(false, false, true, false));
    assert(!WifiUsePolicy::retry(false, true, false, false));
    assert(!WifiUsePolicy::retry(true, true, true, false));
    puts("PASS: 2.8 WiFi USE behavior unchanged");
#endif
    return 0;
}
