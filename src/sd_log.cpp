// SquachWatch-CYD — SD log implementation
#include "sd_log.h"
#include "clock.h"
#include <time.h>
#include <SD.h>
#include <stdio.h>
// The Phantoms define CYD (they ARE a CYD) but still need this reference,
// because their touch shares the display's bus and SdLog::begin() has to hand
// SD the display's own SPI instance -- see the comment on that branch below.
#if !defined(CYD) || defined(RLPHANTOM) || defined(RLPHANTOM_R)
#include <TFT_eSPI.h>
#include <esp_heap_caps.h>
// The single TFT_eSPI instance main.cpp already owns and has already
// init()'d by the time SdLog::begin() runs (see the comment below for
// why AWOK/cyd35 specifically need this reference).
extern TFT_eSPI tft;
#endif

// CYD SD card CS — see docs/PINOUT.md. AWOK: CS=14 on the on-board
// slot, sharing the DISPLAY'S VSPI bus (18/23/19). GPIO5 on this board
// is TFT_RST — reusing the CYD's CS=5 would fight the display. cyd35
// shares its display's VSPI bus too (14/13/12, not 18/19/23) but its
// real SD-slot CS is unconfirmed -- 5 is a placeholder guess (SD has
// failed to mount on every real unit tested so far regardless).
#if defined(AWOK)
    #define SD_CS_PIN 14
#else
    #define SD_CS_PIN 5
#endif

// Room for two open files, not the library's default five. The FAT driver
// reserves a 4 KB sector buffer per file slot up front, so five slots want a
// 25 KB block -- more than is left once both radios are up (largest block
// measured at 18 KB on the RL Phantom). This log has one file open at a time,
// plus a directory handle while it prunes old days.
static const uint8_t SD_MAX_FILES = 2;

bool SdLog::begin() {
    if (_ready) return true;
#if defined(TWATCH_S3)
    return false;   // no card slot; GPIO19/20 are the S3's USB pins
#endif
    Serial.printf("[sd] mounting: heap %lu, largest block %lu\n", (unsigned long)ESP.getFreeHeap(), (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
#if defined(CYD35)
    // (The RL Phantom used to land here too, and its SD card never worked as
    // a result: the card is on 18/19/23, and the display's SPI engine never
    // clocked those pins. Its display now runs on HSPI -- see its user setup
    // -- so it takes the original board's branch below with a bus of its own.)
    // The RL Phantom landed here for the same reason cyd35 does, and it cost a
    // tester an evening: its resistive touch chip sits on the DISPLAY's bus, so
    // the extra pins this attaches corrupt MISO for every touch read afterwards.
    // The symptom is precise and was reported exactly as described below --
    // the 4-corner calibration works (it runs BEFORE engine.init() brings SD up)
    // and touch is dead on the very next screen. Not a bad calibration blob: a
    // corrupted bus underneath a perfectly good one.
    //
    // SD.begin(csPin) defaults its SPIClass& parameter to the Arduino
    // *global* `SPI` object -- a separate, never-begun C++ instance
    // from TFT_eSPI's own internal one, even though both ultimately
    // target the same VSPI hardware. SDFS::begin() (ESP32 core's
    // SD.cpp) unconditionally calls that object's own spi.begin() with
    // NO arguments; for a never-begun SPIClass, SPIClass::begin() falls
    // back to the compiled-in esp32dev board defaults -- SCK=18,
    // MISO=19, MOSI=23 -- regardless of this board's real shared-bus
    // pins (14/13/12 here). Root-caused on real cyd35 hardware: those
    // extra pins get ADDITIONALLY attached to VSPI's signals via the
    // GPIO matrix (spiAttachSCK() etc. are additive, not exclusive),
    // corrupting MISO for every touch read afterward even though the
    // display's write-only path looked completely fine.
    //
    // Passing TFT_eSPI's own already-init()'d SPI instance instead
    // makes SDFS::begin()'s internal spi.begin() call a genuine no-op
    // (SPIClass::begin() returns immediately if already begun -- see
    // its own guard), so nothing extra ever gets attached to the bus.
    //
    // AWOK deliberately does NOT get this treatment despite sharing
    // the same VSPI-bus shape: real hardware regression testing showed
    // its touch stops responding once SD.begin() runs with the shared
    // instance passed in (SD.begin() still attempts real transactions
    // over that peripheral even though `begin()` itself becomes a
    // no-op, and AWOK's touch chip is apparently more sensitive to
    // that than cyd35's) -- so it keeps the plain no-args SD.begin()
    // below, same as before this fix existed.
    if (!SD.begin(SD_CS_PIN, tft.getSPIinstance(), 4000000, "/sd", SD_MAX_FILES)) {
#elif defined(AWOK)
    if (!SD.begin(SD_CS_PIN, SPI, 4000000, "/sd", SD_MAX_FILES)) {
#else
    // Original board only: a genuinely separate, dedicated SD bus (not
    // shared with the display), so it does need its own explicit begin()
    // -- SD.begin()'s internal default-pin fallback happens to match
    // this board's real wiring too, but stay explicit for clarity.
    SPI.begin(18, 19, 23, SD_CS_PIN);  // SCK, MISO, MOSI, CS
    if (!SD.begin(SD_CS_PIN, SPI, 4000000, "/sd", SD_MAX_FILES)) {
#endif
        // Said out loud either way: a board with no card, or a card on the
        // wrong pins, ran exactly like one that was logging, and the only
        // way to tell was to pull the card and look.
        Serial.println("[sd] no card, or it did not answer: nothing will be logged");
        _ready = false;
        return false;
    }
    Serial.printf("[sd] card mounted: %llu MB\n", (unsigned long long)(SD.cardSize() >> 20));
    _ready = true;
    openDaily();
    _lastFlush = millis();
    return true;
}

void SdLog::openDaily() {
    if (!_ready) return;
    if (Clock::trusted()) {
        time_t epoch = (time_t)Clock::nowEpoch();
        struct tm tmv = {};
        localtime_r(&epoch, &tmv);
        snprintf(_filename, sizeof(_filename), "/squachwatch-%04d%02d%02d.log",
                 tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday);
    } else {
        const uint32_t day = millis() / (24UL * 60UL * 60UL * 1000UL);
        snprintf(_filename, sizeof(_filename), "/squachwatch-up-%lu.log", (unsigned long)day);
    }
}

bool SdLog::flushPending() {
    if (!_ready || _pendingLen == 0) return true;
    if (_filename[0] == '\0') openDaily();
    if (_filename[0] == '\0') return false;

    // Rate-limit retry attempts when a card disappears or temporarily stops
    // answering. A successful close() commits FatFS' cached sector metadata.
    _lastFlush = millis();
    File f = SD.open(_filename, FILE_APPEND);
    if (!f) {
        _flushFailures++;
        if (_flushFailures == 1 || ((_flushFailures & (_flushFailures - 1)) == 0)) {
            Serial.printf("[sd] buffered flush open failed (%lu total, %u bytes pending)\n",
                          (unsigned long)_flushFailures, (unsigned)_pendingLen);
        }
        return false;
    }

    const size_t written = f.write((const uint8_t*)_pending, _pendingLen);
    f.close();

    if (written > 0) {
        const size_t consumed = written > _pendingLen ? _pendingLen : written;
        const size_t remain = _pendingLen - consumed;
        if (remain) memmove(_pending, _pending + consumed, remain);
        _pendingLen = (uint16_t)remain;
    }

    if (_pendingLen == 0) return true;

    _flushFailures++;
    if (_flushFailures == 1 || ((_flushFailures & (_flushFailures - 1)) == 0)) {
        Serial.printf("[sd] buffered flush short write (%lu total, %u bytes remain)\n",
                      (unsigned long)_flushFailures, (unsigned)_pendingLen);
    }
    return false;
}

void SdLog::logEvent(const Detection& d) {
    if (!_ready) return;
    if (_filename[0] == '\0') openDaily();

    char line[96];
    char mac[18];
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
             d.mac[0], d.mac[1], d.mac[2], d.mac[3], d.mac[4], d.mac[5]);
    // Sanitize any commas in vendor / name
    char vendorSafe[12], nameSafe[20];
    strncpy(vendorSafe, vendorText(d), sizeof(vendorSafe) - 1); vendorSafe[sizeof(vendorSafe)-1] = 0;
    strncpy(nameSafe,   d.name,   sizeof(nameSafe)   - 1); nameSafe[sizeof(nameSafe)-1]   = 0;
    for (char* p = vendorSafe; *p; p++) if (*p == ',') *p = '.';
    for (char* p = nameSafe;   *p; p++) if (*p == ',') *p = '.';

    const int n = snprintf(line, sizeof(line),
                           "%lu,%s,%d,%s,%u,%s,%s\n",
                           (unsigned long)millis(),
                           detectionTypeName(d.type),
                           d.rssi,
                           mac,
                           d.channel,
                           vendorSafe,
                           nameSafe);
    if (n <= 0) return;
    const size_t lineLen = (size_t)n < sizeof(line) ? (size_t)n : sizeof(line) - 1;

    // The queue feeding this method is already bounded. This second fixed
    // buffer converts several small event writes into one card transaction.
    // If the buffer is full, try to commit the older rows first; if the card
    // is unavailable, retain those older rows and drop the newest one.
    if ((size_t)_pendingLen + lineLen > WRITE_BUFFER_CAP) {
        flushPending();
    }
    if ((size_t)_pendingLen + lineLen > WRITE_BUFFER_CAP) {
        _droppedLines++;
        if (_droppedLines == 1 || ((_droppedLines & (_droppedLines - 1)) == 0)) {
            Serial.printf("[sd] write buffer full; dropped %lu line(s), %u bytes pending\n",
                          (unsigned long)_droppedLines, (unsigned)_pendingLen);
        }
        return;
    }

    memcpy(_pending + _pendingLen, line, lineLen);
    _pendingLen = (uint16_t)(_pendingLen + lineLen);

    // Batch normal traffic, but do not let a busy room fill the whole sector.
    // The 50 ms guard avoids hammering a failing card once per detection.
    if (_pendingLen >= WRITE_BUFFER_FLUSH_AT && millis() - _lastFlush >= 50) {
        flushPending();
    }
}

void SdLog::wipe() {
    if (!_ready) return;

    // A wipe means "forget it", including rows not yet committed to the card.
    // Never flush these first or a detection queued just before a duress wipe
    // would be written immediately before the files are deleted.
    _pendingLen = 0;
    _lastFlush = millis();

    // Walk the root and remove every file this firmware writes. Names are
    // /squachwatch-YYYYMMDD.log; matching on the prefix takes them all rather
    // than only today's, which is the whole point of a wipe.
    File dir = SD.open("/");
    if (!dir) return;
    dir.close();

    // Collect then remove in bounded batches. A card can hold far more than
    // sixteen daily logs; the old one-batch implementation left older files
    // behind during a security/duress wipe.
    for (;;) {
        char victims[16][40];
        int n = 0;
        File pass = SD.open("/");
        if (!pass) break;
        for (File f = pass.openNextFile(); f && n < 16; f = pass.openNextFile()) {
            const char* nm = f.name();
            const char* base = nm;
            for (const char* p = nm; *p; p++) if (*p == '/') base = p + 1;
            if (strncmp(base, "squachwatch-", 12) == 0) {
                snprintf(victims[n], sizeof victims[n], "/%s", base);
                n++;
            }
            f.close();
        }
        pass.close();
        if (!n) break;
        int removed = 0;
        for (int i = 0; i < n; i++) if (SD.remove(victims[i])) removed++;
        if (!removed) break;
    }
    _filename[0] = '\0';       // next buffered event rebuilds the daily path
}

void SdLog::tick() {
    if (!_ready) return;
    const uint32_t now = millis();

    // Commit sparse traffic within one second. Dense traffic usually reaches
    // WRITE_BUFFER_FLUSH_AT first and flushes from logEvent().
    if (_pendingLen && now - _lastFlush >= 1000) {
        flushPending();
    }

    // Re-evaluate the daily filename once an hour, preserving the previous
    // behavior. Buffered rows must be committed under the OLD filename before
    // switching; if a card error prevents that, leave the check due so the
    // next successful retry can rotate immediately afterward.
    static uint32_t lastDayCheck = 0;
    if (now - lastDayCheck >= 3600000) {
        if (_pendingLen == 0 || (now - _lastFlush >= 1000 && flushPending())) {
            lastDayCheck = now;
            openDaily();
        }
    }
}
