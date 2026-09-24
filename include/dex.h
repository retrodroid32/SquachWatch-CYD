// SquachWatch-CYD — the SQUACHY-DEX: one entry per detection type.
//
// BINGO is a card you fill once a week. The DEX never finishes: seventeen
// numbered entries, a silhouette for every type never caught, and a card
// behind each with what it is, where it lives, what Squachy thinks of it,
// and this board's own record against it.
#pragma once
#include <stdint.h>
#include "state.h"

class DetectionEngine;

namespace Dex {

static const uint8_t ENTRIES = (uint8_t)DetectionType::COUNT - 1;

DetectionType typeAt(uint8_t i);
uint8_t       indexOf(DetectionType t);

enum class Rarity : uint8_t { COMMON, UNCOMMON, RARE };
Rarity      rarity(DetectionType t);
const char* rarityName(Rarity r);
uint8_t     stars(Rarity r);

const char* shortName(DetectionType t);
const char* kind(DetectionType t);
const char* lore(DetectionType t);
const char* habitat(DetectionType t);
const char* quip(DetectionType t);
const char* hint(DetectionType t);
const char* radio(DetectionType t);

// note() is callback-safe and only accumulates compact pending data. tick()
// runs on loop(), applies it to the DEX records, and is the only normal path
// that writes the record blob to Preferences.
void begin();
void note(DetectionType t, int8_t rssi);
void tick(uint32_t now);

struct Record {
    uint32_t firstEpoch;
    uint32_t lastEpoch;
    int8_t   bestRssi;
    uint16_t night;
};
const Record& record(DetectionType t);

bool takeNewClosest(DetectionType t);
DetectionType nemesis(const DetectionEngine& eng);
void reset();

}  // namespace Dex
