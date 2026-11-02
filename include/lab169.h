#pragma once
// LAB-only hardware-test status for PR #169. No SD writes or real wardriving.
#include <stdint.h>
namespace Lab169 {
struct Status {
    uint32_t baud, bytes, good, bad, epoch, csvCount;
    int32_t lat7, lon7;
    uint8_t sats, view, heard;
    bool valid, fresh, csvPass;
};
void begin();
void tick(uint32_t now);
Status status(uint32_t now);
}
