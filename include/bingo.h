// SquachWatch-CYD — detection bingo
//
// A card of sixteen detection types. A type marks its square the first time
// the board sees one this week; four in a row is a line; sixteen is a full
// card. A fresh card every week, and a streak for weeks finished.
//
// Cheap on purpose: the card is sixteen bytes, the marks sixteen more, and
// nothing here runs per frame. note() is safe from radio callback context:
// it only ORs a bit into a critical-section-protected handoff. tick(), from
// loop(), atomically takes those bits, mutates the card, and writes flash.
#pragma once
#include <stdint.h>
#include "state.h"

class DetectionEngine;

namespace Bingo {

static const uint8_t CELLS = 16;   // 4x4
static const uint8_t LINES = 10;   // four rows, four columns, two diagonals

enum class Event : uint8_t { NONE, MARKED, LINE, FULL, NEW_CARD };

void begin(const DetectionEngine& eng);
void tick(uint32_t now);

// Callback-safe sighting handoff. No Preferences or card mutation here.
void note(DetectionType t);

DetectionType typeAt(uint8_t i);
bool     marked(uint8_t i);
uint8_t  markDay(uint8_t i);
uint8_t  markedCount();
bool     inCalledLine(uint8_t i);
uint8_t  linesCalled();
uint32_t weekNumber();

uint16_t cardsFilled();
uint16_t linesEver();
uint8_t  streak();
uint8_t  bestStreak();
uint8_t  bestFilled();

void newCard();
Event takeEvent(DetectionType& type);

}  // namespace Bingo
