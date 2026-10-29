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
