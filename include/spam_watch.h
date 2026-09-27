// SquachWatch-CYD — tracker flood suppression.
//
// A BLE spammer can emit AirTag / SmartTag / Google-tag / Tile signatures
// from a fresh address every burst. Without a flood guard that can turn one
// room into dozens of full-screen alerts while the useful signal is simply
// "someone is flooding tracker identities here".
//
// This is deliberately narrower than the upstream implementation:
// only tracker classes are eligible, and the alert gate in main.cpp still
// owns every user policy (LOG ONLY, confidence, repeats, cooldown, WATCH,
// ALWAYS ALERT, device snooze, AUTO SNOOZE, lock-screen rules).
#pragma once
#include <stdint.h>
#include "state.h"

struct SpamWatch {
    static constexpr uint8_t TYPES = (uint8_t)DetectionType::COUNT;
    static constexpr uint32_t SHORT_MS   = 10000;   // one short visit, then gone
    static constexpr uint16_t TRIP16     = 112;     // ~8 quick one-off identities
    static constexpr uint32_t HALF_MS    = 90000;   // evidence halves every 90 s
    static constexpr uint16_t BURST_TRIP = 40;      // new tracker MACs in one minute
    static constexpr uint32_t QUIET_MS   = 300000;  // 5 min without evidence ends flood

    struct Type {
        uint16_t score16   = 0;
        uint16_t fakes     = 0;
        uint32_t lastMs    = 0;
        uint8_t  active    = 0;
        uint8_t  announced = 0;
    };
    Type t[TYPES];

    static bool eligible(DetectionType type) {
        switch (type) {
            case DetectionType::AIRTAG:
            case DetectionType::SAMSUNG_TAG:
            case DetectionType::GOOGLE_TAG:
            case DetectionType::TILE:
                return true;
            default:
                return false;
        }
    }

    // A tracker identity went stale after one short visit. visits is
    // Detection::hits in this fork: BLE increments it only when an address
    // disappears and later returns, so visits > 1 is evidence of a real,
    // recurring device rather than a throw-away spoof identity.
    bool noteVanish(DetectionType type, uint32_t heardMs, uint16_t visits, uint32_t now) {
        if (!eligible(type) || heardMs >= SHORT_MS || visits > 1) return false;
        return evidence((uint8_t)type, 1, now, false);
    }

    // A flood too fast for rows to age out can still be seen from the count of
    // genuinely new tracker identities created by pushLog() in one minute.
    bool noteBurst(DetectionType type, uint16_t newInAMinute, uint32_t now) {
        if (!eligible(type) || newInAMinute < BURST_TRIP) return false;
        return evidence((uint8_t)type, newInAMinute, now, true);
    }

    bool active(DetectionType type, uint32_t now) {
        if (!eligible(type)) return false;
        Type& x = t[(uint8_t)type];
        if (x.active && (uint32_t)(now - x.lastMs) > QUIET_MS) x = Type();
        return x.active != 0;
    }

    bool takeAnnounce(DetectionType type) {
        if (!eligible(type)) return false;
        Type& x = t[(uint8_t)type];
        if (!x.active || x.announced) return false;
        x.announced = 1;
        return true;
    }

    uint16_t fakes(DetectionType type) const {
        return eligible(type) ? t[(uint8_t)type].fakes : 0;
    }

private:
    bool evidence(uint8_t type, uint16_t n, uint32_t now, bool burst) {
        Type& x = t[type];
        // A new piece of evidence after the quiet timeout belongs to a new
        // flood even if no alert candidate happened to call active() during
        // the gap.
        if (x.active && x.lastMs && (uint32_t)(now - x.lastMs) > QUIET_MS)
            x = Type();
        uint32_t s = x.score16;
        uint32_t el = x.lastMs ? (uint32_t)(now - x.lastMs) : 0;

        while (el >= HALF_MS && s) {
            s >>= 1;
            el -= HALF_MS;
        }
        if (el >= HALF_MS) el = 0;
        s -= s * el / (2u * HALF_MS);

        if (!x.active && s < 16) x.fakes = 0;
        s += (uint32_t)n * 16u;
        x.score16 = (uint16_t)(s > 0xFFFFu ? 0xFFFFu : s);
        const uint32_t f = (uint32_t)x.fakes + n;
        x.fakes = (uint16_t)(f > 0xFFFFu ? 0xFFFFu : f);
        x.lastMs = now ? now : 1;

        if (x.active) return false;
        if (burst || s >= TRIP16) {
            x.active = 1;
            x.announced = 0;
            return true;
        }
        return false;
    }
};
