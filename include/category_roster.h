// SquachWatch-CYD -- one source of truth for CLEAR's grouped counters
// and the category roster. Categories are presentation groups, not new
// detection identities: every row retains its real type and evidence.
#pragma once
#include "state.h"

namespace CategoryRoster {

// Folds must match CLEAR's displayed counter totals. In particular a
// grouped tracker is never mislabeled as an Apple AirTag in its row.
inline bool matches(DetectionType group, DetectionType actual) {
    if (group == DetectionType::AIRTAG)
        return actual == DetectionType::AIRTAG ||
               actual == DetectionType::SAMSUNG_TAG ||
               actual == DetectionType::GOOGLE_TAG ||
               actual == DetectionType::TILE;
    if (group == DetectionType::CAMERA)
        return actual == DetectionType::CAMERA ||
               actual == DetectionType::RING;
    if (group == DetectionType::HACKER)
        return actual == DetectionType::HACKER ||
               actual == DetectionType::EVILTWIN;
    return group != DetectionType::UNKNOWN && group == actual;
}

// Do not confuse the engine's 60-second active counter with a fresh
// observation. This same wrap-safe predicate drives roster order and labels.
inline bool fresh(const Detection& d, uint32_t now) {
    return !d.restored && d.active &&
           (int32_t)(now - d.lastSeen) <= (d.channel ? 6500 : 4000);
}

inline const char* label(DetectionType group) {
    switch (group) {
        case DetectionType::AIRTAG: return "TRACKER";
        case DetectionType::CAMERA: return "CAMERA";
        case DetectionType::HACKER: return "HACKER";
        case DetectionType::META: return "GLASSES";
        default: return detectionTypeName(group);
    }
}

} // namespace CategoryRoster
