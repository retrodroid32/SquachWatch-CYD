// SquachWatch-CYD — persistent per-device alert policies
#pragma once
#include <stdint.h>
#include "state.h"

// A short list of device-specific behavior overrides. Policies affect only
// interruption behavior: detections are still scanned, counted and logged.
// DETECTION FILTER remains authoritative upstream -- a disabled type is never
// resurrected here.
//
// Storage is one packed NVS blob of eight-byte records:
//   MAC[6] + DetectionType + Policy
// At MAX=64 that is 512 bytes, only 64 bytes more than v1.20.1's 7-byte
// IGNORE-only table. Optional aliases deliberately do not live in every record.
namespace IgnoreList {

    static const uint8_t MAX = 64;

    enum class Policy : uint8_t {
        NORMAL = 0,       // no persistent record
        IGNORE = 1,       // suppress normal full-screen alerts
        TRUSTED = 2,      // expected/mine; suppress normal full-screen alerts
        ALWAYS_ALERT = 3  // bypass alert rules/cooldown/snooze, but not type filter/security
    };

    // Loads from NVS and migrates the older "dev" / "macs" formats. Safe to
    // call more than once.
    void begin();

    Policy policy(const uint8_t* mac);
    bool   trusted(const uint8_t* mac);
    bool   alwaysAlert(const uint8_t* mac);

    // Backward-compatible meaning: contains() is true only for IGNORE.
    bool contains(const uint8_t* mac);

    // Create/update/remove a policy. NORMAL removes the record. Returns false
    // when the list is full or persistence fails; RAM is rolled back on a
    // failed write so the running state never claims a change was saved.
    bool setPolicy(const uint8_t* mac, DetectionType type, Policy p);

    // Compatibility helpers used by the existing quick IGNORE actions.
    // add() means "set IGNORE" and returns false when already IGNORE.
    bool add(const uint8_t* mac, DetectionType type = DetectionType::UNKNOWN);
    bool remove(const uint8_t* mac);

    uint8_t count();

    // nullptr when idx is out of range. Points into module storage; valid until
    // the next policy mutation/clear.
    const uint8_t* macAt(uint8_t idx);
    DetectionType typeAt(uint8_t idx);
    Policy        policyAt(uint8_t idx);

    void clear();

    // SNOOZE is RAM-only and lasts until restart. IGNORE/TRUSTED suppress it
    // anyway; ALWAYS_ALERT deliberately bypasses it.
    void snooze(const uint8_t* mac);
    bool snoozed(const uint8_t* mac);
    bool silenced(const uint8_t* mac);
}
