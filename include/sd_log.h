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
    // The browser holds the filesystem lock for its entire session.
    // Mount/monitor activity must not interleave with file navigation.
    void browseBegin();
    void browseEnd();
    void browseFault();
    bool browseLock();        // nonblocking: never wait for an SD timeout on UI task
    void browseUnlock();
    bool rescan();            // request async remount from SD monitor
    bool formatCard();        // real FAT formatting, double-confirmed by browser
    const char* formatError() const { return _formatError; }
    void startMonitor();       // mount-state monitor on a separate task (CYD boards)  // an explicit browse I/O failure (never polled)
    void logEvent(const Detection& d);
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

    // Latched off on first write/open error; no repeated SPI retries.
    volatile bool _ready = false;
    volatile bool _browsing = false;
    volatile bool _requestRescan = false;
    bool _everMounted = false;
    char _formatError[40] = {0};
    void* _ioMutex = nullptr;   // FreeRTOS semaphore, opaque for PC emulator headers
    void* _monitorTask = nullptr;
    uint32_t _lastFlush = 0;
    uint32_t _flushFailures = 0;
    uint32_t _droppedLines = 0;
    char     _filename[32] = {0};
    char     _pending[WRITE_BUFFER_CAP] = {0};
    uint16_t _pendingLen = 0;

    void openDaily();
    bool flushPending();
    bool flushPendingLocked();
    static void monitorEntry(void* ctx);
    void disableAfterIoFailure(const char* operation);
};
