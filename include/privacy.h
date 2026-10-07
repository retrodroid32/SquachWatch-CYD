// SquachWatch-CYD — PRIVACY MODE: what the screen shows of other people.
//
// Screen-only masking for filming/screenshots. Persistent logs, BlackBox,
// wardrive data and internal detection state keep their real values.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "settings.h"

namespace Privacy {
inline bool on() { return Settings::privacyMode(); }

inline void mac(char* out, size_t n, const uint8_t* m) {
    if (on()) snprintf(out, n, "%02X:%02X:%02X:XX:XX:XX", m[0], m[1], m[2]);
    else      snprintf(out, n, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
}

inline const char* name(const char* in, char* buf, size_t n) {
    if (!on() || !in || !in[0] || in[0] == '(' || n < 8) return in;
    if (strcmp(in, "Unnamed device") == 0 || strcmp(in, "UNKNOWN DEVICE") == 0) return in;
    const size_t len = strlen(in);
    snprintf(buf, n, "%.*s***", (int)(len < 3 ? 1 : 3), in);
    return buf;
}
}
