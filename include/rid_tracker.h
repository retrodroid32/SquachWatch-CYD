// SquachWatch-CYD — bounded, no-allocation ASTM Remote ID contact assembler.
//
// A flight broadcasts Basic ID, Location, System and other records separately.
// The old single Info object was reset whenever *any* different MAC appeared;
// two nearby aircraft consequently overwrote each other's partial messages.
// Keep eight independent contacts on classic CYD without allocating in radio
// callbacks. BLE Legacy and Wi-Fi Beacon are independent source identities;
// an identical MAC on different transports is NOT proof of the same aircraft.
#pragma once
#include "remote_id.h"
#include <stdint.h>
#include <string.h>

namespace RemoteId {

class Tracker {
public:
    static constexpr uint8_t kCapacity = 8;
    static constexpr uint32_t kExpiryMs = 30000;

    enum Transport : uint8_t { BLE_LEGACY = 1, WIFI_BEACON = 2 };

    struct Contact {
        uint8_t mac[6] = {};
        Info info;
        uint32_t firstSeen = 0;
        uint32_t lastSeen = 0;
        uint16_t observations = 0;
        Transport transport = BLE_LEGACY;
        bool occupied = false;
    };

    // Call ONLY on the normal loop / BLE host side, never the ISR/radio
    // Wi-Fi callback. Wi-Fi captures already go through a bounded queue.
    const Contact* ingestBle(const uint8_t* mac, const uint8_t* ad,
                             uint8_t len, uint32_t now) {
        if (!mac || !ad) return nullptr;
        const Contact* old = lookup(mac, BLE_LEGACY, now);
        Info candidate = old ? old->info : Info();
        if (!merge(ad, len, candidate, now)) return nullptr;
        return store(mac, BLE_LEGACY, candidate, now);
    }

    const Contact* ingestWifiPack(const uint8_t* mac, const uint8_t* pack,
                                  uint16_t len, uint32_t now) {
        if (!mac || !pack) return nullptr;
        const Contact* old = lookup(mac, WIFI_BEACON, now);
        Info candidate = old ? old->info : Info();
        if (!mergeWifiPack(pack, len, candidate, now)) return nullptr;
        return store(mac, WIFI_BEACON, candidate, now);
    }

    uint8_t liveCount(uint32_t now) const {
        uint8_t n = 0;
        for (uint8_t i = 0; i < kCapacity; ++i)
            if (isFresh(_contacts[i], now)) ++n;
        return n;
    }

    // Enumerate live contacts densely in [0, liveCount(now)); holes left by
    // expiration/eviction must not make a future list silently skip aircraft.
    const Contact* contactAt(uint8_t index, uint32_t now) const {
        uint8_t seen = 0;
        for (uint8_t slot = 0; slot < kCapacity; ++slot) {
            if (!isFresh(_contacts[slot], now)) continue;
            if (seen++ == index) return &_contacts[slot];
        }
        return nullptr;
    }

    // Latest is a physical slot, not a dense live ordinal.
    const Contact* latest(uint32_t now) const {
        if (_latest >= kCapacity || !isFresh(_contacts[_latest], now)) return nullptr;
        return &_contacts[_latest];
    }

    const Contact* find(const uint8_t* mac, Transport source, uint32_t now) const {
        return lookup(mac, source, now);
    }

private:
    Contact _contacts[kCapacity] = {};
    uint8_t _latest = kCapacity;

    static bool isFresh(const Contact& c, uint32_t now) {
        return c.occupied && (uint32_t)(now - c.lastSeen) <= kExpiryMs;
    }

    const Contact* lookup(const uint8_t* mac, Transport source, uint32_t now) const {
        if (!mac) return nullptr;
        for (uint8_t i = 0; i < kCapacity; ++i) {
            const Contact& c = _contacts[i];
            if (isFresh(c, now) && c.transport == source &&
                memcmp(c.mac, mac, sizeof(c.mac)) == 0) return &c;
        }
        return nullptr;
    }

    const Contact* store(const uint8_t* mac, Transport source,
                         const Info& info, uint32_t now) {
        uint8_t slot = kCapacity;
        uint8_t reclaim = kCapacity;
        uint8_t oldest = 0;
        uint32_t oldestAge = 0;
        // Prefer a matching live contact, then an empty/expired slot.
        // Only evict the least recently seen live contact if table is full.
        for (uint8_t i = 0; i < kCapacity; ++i) {
            const Contact& c = _contacts[i];
            if (isFresh(c, now) && c.transport == source &&
                memcmp(c.mac, mac, sizeof(c.mac)) == 0) {
                slot = i;
                break;
            }
            if (!isFresh(c, now) && reclaim == kCapacity) reclaim = i;
            if (isFresh(c, now)) {
                uint32_t age = (uint32_t)(now - c.lastSeen);
                if (age >= oldestAge) { oldestAge = age; oldest = i; }
            }
        }
        if (slot == kCapacity) slot = reclaim == kCapacity ? oldest : reclaim;
        Contact& c = _contacts[slot];
        const bool same = isFresh(c, now) && c.transport == source &&
                          memcmp(c.mac, mac, sizeof(c.mac)) == 0;
        if (!same) c = Contact();
        memcpy(c.mac, mac, sizeof(c.mac));
        c.transport = source;
        c.info = info;
        if (!same) c.firstSeen = now;
        c.lastSeen = now;
        if (c.observations != UINT16_MAX) ++c.observations;
        c.occupied = true;
        _latest = slot;
        return &c;
    }
};

}  // namespace RemoteId
