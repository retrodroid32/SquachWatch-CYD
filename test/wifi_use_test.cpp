// Test the USE-button join-only contract through the deterministic emulator.
// This is not hardware Wi-Fi validation; firmware builds cover ESP32 symbols.
#include "ota_wifi.h"
#include <Arduino.h>
#include <cassert>
#include <cstring>
#include <cstdio>

int main() {
    SimClock::virtualTime = true;
    SimClock::nowMs = 1000;
    using namespace OtaWifi;
    assert(savedCount() >= 3);
    assert(savedTestState() == SavedTest::IDLE);
    // USE may recheck an already selected network: its purpose is validation.
    const uint8_t original = savedUse();
    useSaved(original);
    assert(testSavedAt(original));
    assert(savedTestState() == SavedTest::CONNECTING);
    assert(!testSavedAt(original)); // no duplicate radio job
    SimClock::nowMs += 1499;
    assert(savedTestState() == SavedTest::CONNECTING);
    SimClock::nowMs += 2;
    assert(savedTestState() == SavedTest::JOINED);
    assert(savedResult(original) == SavedResult::JOINED);
    clearSavedTest();
    assert(savedTestState() == SavedTest::IDLE);

    // A bad credential should not be stored as a successful join.
    assert(saveNetwork("WrongPassword", "invalid"));
    const int8_t bad = savedIndexOf("WrongPassword");
    assert(bad >= 0);
    useSaved((uint8_t)bad);
    assert(testSavedAt((uint8_t)bad));
    SimClock::nowMs += 1600;
    assert(savedTestState() == SavedTest::BAD_PASSWORD);
    assert(savedResult((uint8_t)bad) == SavedResult::BAD_PASSWORD);
    clearSavedTest();
    assert(savedTestState() == SavedTest::IDLE);
    assert(!testSavedAt(SAVED_MAX));
    std::puts("PASS: USE tests selected WiFi, records join/failure, and prevents overlapping attempts");
    return 0;
}
