#pragma once
#include <stdint.h>
#include <string.h>

// Identity-only FIFO. No allocation and no pointers into a mutable log.
template<class Type, uint8_t Capacity>
class AlertQueue {
public:
    struct Entry { uint8_t mac[6]; Type type; };
    uint8_t size() const { return count_; }
    void clear() { count_ = 0; }
    bool push(const uint8_t* mac, Type type) {
        if (!mac || count_ == Capacity) return false;
        for (uint8_t i = 0; i < count_; ++i)
            if (entries_[i].type == type && memcmp(entries_[i].mac, mac, 6) == 0)
                return false;
        memcpy(entries_[count_].mac, mac, 6);
        entries_[count_++].type = type;
        return true;
    }
    bool pop(Entry& out) {
        if (!count_) return false;
        out = entries_[0];
        for (uint8_t i = 1; i < count_; ++i) entries_[i-1] = entries_[i];
        --count_;
        return true;
    }
private:
    Entry entries_[Capacity] = {};
    uint8_t count_ = 0;
};
