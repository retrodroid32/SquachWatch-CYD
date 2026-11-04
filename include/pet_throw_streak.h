#pragma once
#include <stdint.h>

// One true fling contributes one sample. A rolling one-minute window, not
// calendar-minute buckets: moving the clock or crossing millis() rollover
// cannot accidentally grant a free tenth throw.
class PetThrowStreak {
public:
    static constexpr uint8_t LIMIT = 10;
    static constexpr uint32_t WINDOW_MS = 60000u;

    bool note(uint32_t now) {
        uint8_t kept = 0;
        for (uint8_t i = 0; i < count_; i++) {
            if ((uint32_t)(now - times_[i]) < WINDOW_MS)
                times_[kept++] = times_[i];
        }
        times_[kept++] = now;
        count_ = kept;
        if (count_ < LIMIT) return false;
        reset();
        return true;
    }
    void reset() { count_ = 0; }
    uint8_t count() const { return count_; }
private:
    uint32_t times_[LIMIT] = {};
    uint8_t count_ = 0;
};
