// UBX binary GNSS framing and NAV packet decoding.
// Independent of Arduino: compiled by both firmware and host tests.
#pragma once
#include <stdint.h>
#include "gnss.h"
namespace Ubx {
bool feed(uint8_t b, uint32_t nowMs); // true if byte consumed as UBX
const Gnss::Fix& fix();
Gnss::Sky sky();
bool hasSky();
uint32_t utcEpoch();
uint32_t good();
uint32_t bad();
uint32_t pvt();
uint32_t sat();
uint8_t lastClass();
uint8_t lastId();
void reset();
}
