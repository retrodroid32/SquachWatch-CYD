// UTC timestamps for SD detection rows; host-testable with no ESP dependencies.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

namespace SdTimestamp {
// Preserve numeric uptime when no trusted wall clock exists.
inline bool format(char* out, size_t cap, bool trusted, uint32_t epoch, uint32_t uptimeMs) {
    if (!out || !cap) return false;
    out[0] = '\0';
    if (trusted && epoch >= 946684800u) {
        const time_t when = static_cast<time_t>(epoch);
        struct tm utc = {};
        if (gmtime_r(&when, &utc)) {
            const int n = snprintf(out, cap, "%04d-%02d-%02dT%02d:%02d:%02dZ",
                                   utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
                                   utc.tm_hour, utc.tm_min, utc.tm_sec);
            if (n > 0 && static_cast<size_t>(n) < cap) return true;
        }
    }
    const int n = snprintf(out, cap, "%lu", (unsigned long)uptimeMs);
    if (n < 0 || static_cast<size_t>(n) >= cap) out[0] = '\0';
    return false;
}
}
