#pragma once
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "places.h"

// Loop-task only. No flash writes, no per-detection growth or heap allocation.
// Unlike upstream's full per-row history, eight recently observed identities
// share a compact cache to preserve classic CYD's contiguous heap reserve.
namespace TrailHistory {
struct Entry {
    uint8_t mac[6];
    uint8_t type, count, stop[3];
    uint32_t firstMs, lastMs;
};
struct Cache {
    Entry rows[8] = {};
    uint8_t previousPlace = 0;
    void clear() { memset(rows,0,sizeof rows); previousPlace=0; }
    const Entry* find(const uint8_t* mac, uint8_t type) const {
        for (const auto& r: rows)
            if (r.count && r.type==type && !memcmp(r.mac,mac,6)) return &r;
        return nullptr;
    }
    void note(const uint8_t* mac,uint8_t type,uint8_t place,uint32_t now) {
        if (!mac || !place) return;
        if (previousPlace && place < previousPlace) clear();
        previousPlace=place;
        Entry* e=nullptr; Entry* oldest=&rows[0];
        for (auto& r: rows) {
            if (r.count && r.type==type && !memcmp(r.mac,mac,6)) { e=&r; break; }
            if (!r.count || (oldest->count && (uint32_t)(now-r.lastMs)>(uint32_t)(now-oldest->lastMs))) oldest=&r;
        }
        if (!e) {
            e=oldest; *e=Entry{};
            memcpy(e->mac,mac,6); e->type=type; e->firstMs=now;
        }
        if (!e->count || e->stop[e->count-1]!=place) {
            if (e->count==3) {
                e->stop[0]=e->stop[1]; e->stop[1]=e->stop[2]; e->count=2;
            }
            e->stop[e->count++]=place;
        }
        e->lastMs=now;
    }
};
inline Cache& cache() { static Cache c; return c; }
inline void clear() { cache().clear(); Places::reset(); }
inline void note(const uint8_t* mac,uint8_t type,uint32_t now) {
    cache().note(mac,type,Places::current(),now);
}
inline const char* describe(const uint8_t* mac,uint8_t type,bool hidden,bool restored) {
    static char out[220];
    if (hidden) return "Trail hidden while privacy mode or lock is active.";
    if (restored) return "Historical record. Stop history was not stored.";
    const Entry* e=cache().find(mac,type);
    if (!e) return "No recent stop history for this device. Wait for more sightings.";
    unsigned n=0;
    for (uint8_t i=0;i<e->count;i++)
        if (Places::startMs(e->stop[i]) || Places::current()==e->stop[i]) ++n;
    if (!n) return "Recent stops are no longer in memory.";
    snprintf(out,sizeof out,"Observed at %u recent stop%s over %lu min of cached sightings. Stops use nearby WiFi fingerprints or fresh GPS. Approximate observations, not proof of following. RAM only; cleared on reboot or log clear.",
        n,n==1?"":"s",(unsigned long)((e->lastMs-e->firstMs)/60000u));
    return out;
}
} // namespace TrailHistory
