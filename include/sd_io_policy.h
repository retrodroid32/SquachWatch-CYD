// Stop retrying an SD card after a failed or excessively slow append.
// Host-testable; SPI/FAT operations remain in sd_log.cpp.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace SdIoPolicy {
// Limit is deliberately longer than normal FAT appends, so an ordinary
// slow card is not disabled during routine logging. A missing card can
// otherwise make file-close repeatedly stall the UI task.
static constexpr uint32_t MAX_APPEND_MS = 500u;
enum class Verdict : uint8_t { OK, OPEN_FAILED, SHORT_WRITE, SLOW_IO };
inline Verdict evaluate(bool opened, size_t expected, size_t written, uint32_t elapsedMs) {
    if (!opened) return Verdict::OPEN_FAILED;
    if (written != expected) return Verdict::SHORT_WRITE;
    if (elapsedMs >= MAX_APPEND_MS) return Verdict::SLOW_IO;
    return Verdict::OK;
}
}
