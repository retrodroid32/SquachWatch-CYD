// SquachWatch-CYD — the regulars: devices he sees day after day.
//
// A device seen on three different days gets a name from a short list. The
// table contains real MAC addresses, so it is encounter history and is
// deliberately removed by a security/duress wipe.
#pragma once
#include <stdint.h>
#include "state.h"

namespace Regulars {

static const uint8_t CAP        = 16;
static const uint8_t DAYS_TO_BE = 3;

void begin();

// Callback-safe sighting handoff. This only queues compact MAC/type data;
// tick() applies it to the table on loop() and performs delayed persistence.
void note(const uint8_t* mac, DetectionType type);

// Immediate loop/test helper for a known day number.
void noteOnDay(const uint8_t* mac, DetectionType type, uint32_t day);
void tick(uint32_t now);

const char* nameFor(const uint8_t* mac);
uint8_t     daysFor(const uint8_t* mac);
bool takeNewRegular(const uint8_t* mac);
uint8_t count();

void reset();

}  // namespace Regulars
