// SquachWatch-CYD — optional buffered SD card event log
// If the SD card is mounted at boot, each Detection is formatted as one CSV
// line and queued in a small fixed RAM buffer before being appended to
// /squachwatch-YYYYMMDD.log.
// If the card is absent, every call is a silent no-op.
#pragma once
#include <Arduino.h>
#include "state.h"

class SdLog {
public:
    bool begin();              // returns true if card mounted
    bool ready() const { return _ready; }
    void logEvent(const Detection& d);
    // Optional per-device detection-event evidence for selected WATCH/HUNT
    // targets and confirmed EVIL TWIN events. Never called from radio ISRs.
    void logTargetEvidence(const Detection& d, bool watched, bool hunted);
    // Record bounded raw BLE advertising data for deliberately watched targets.
    // Queued by radio callbacks, written only by the loop task.
    void logWatchAdvertisement(const uint8_t mac[6], const uint8_t* bytes,
                               uint8_t len, uint16_t originalLen, int8_t rssi);
    void tick();               // buffered flush / housekeeping (called from loop)
    // Deletes every squachwatch log file on the card. For the security wipe --
    // the phrase and the ignore list live in NVS, but the detection history a
    // wipe must also erase is here. A no-op when no card is mounted.
    void wipe();
private:
    // One sector of fixed RAM is enough to batch several CSV rows without
    // materially changing the memory profile of the radio/display-heavy builds.
    // Flush before the buffer is completely full so one normal row can still
    // arrive while an earlier batch is waiting for a retry.
    static constexpr size_t WRITE_BUFFER_CAP = 512;
    static constexpr size_t WRITE_BUFFER_FLUSH_AT = 384;

    bool     _ready = false;
    uint32_t _lastFlush = 0;
    uint32_t _flushFailures = 0;
    uint32_t _droppedLines = 0;
    char     _filename[32] = {0};
    char     _pending[WRITE_BUFFER_CAP] = {0};
    uint16_t _pendingLen = 0;

    void openDaily();
    bool flushPending();
};
