// SquachWatch-CYD — SD log implementation
#include "sd_log.h"
#include "clock.h"
#include "sd_timestamp.h"
#include "theme.h"
#include "sd_io_policy.h"
#include <time.h>
#include <SD.h>
#include <sd_diskio.h>
#include <ff.h> // defines FatFs BYTE/DWORD/UINT used by diskio.h
#include <diskio.h>
#include <stdio.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_system.h>
#include <esp_idf_version.h>
#include <esp_vfs_fat.h>
#include <esp_err.h>
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
//
// NM-CYD-C5: CS=10, on the DISPLAY's bus (SCK 6, MISO 2, MOSI 7), from
// RockBase's connections.md. Not 5 -- GPIO5 on this board is the GPS header's
// UART, and the card simply never answers.
#if defined(AWOK)
    #define SD_CS_PIN 14
#elif defined(NM_CYD_C5)
    #define SD_CS_PIN 10
#else
    #define SD_CS_PIN 5
#endif

// A single open FILE is sufficient for all SD operations in this firmware.
// Directory iterators use FatFs DIR (opendir/readdir), not VFS FIL slots.
// The browser holds a directory while opening at most one child FILE;
// the logger pauses while browsing and every other SD operation is
// serialized by the SD mutex. A second VFS FIL slot demands a much larger
// contiguous allocation and has been demonstrated to fail on reinsert
// even though one slot can be registered (R5 hardware diagnosis).
// Do not increase without redesigning and verifying hot-insert allocation.
static const uint8_t SD_MAX_FILES = 1;

bool SdLog::begin() {
    if (_ready) return true;
    ++_mountAttempts;
    _statusDetail = "TRYING SD MOUNT";
    // Do not turn a detected unplug (red) back to gray during unsuccessful
    // remount attempts: the user must see the latched error until recovery.
#if defined(TWATCH_S3)
    return false;   // no card slot; GPIO19/20 are the S3's USB pins
#endif
#if defined(FREENOVE_S3)
    return false;   // this board's slot is SDMMC; the SPI logger has no validated mapping
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
#elif defined(NM_CYD_C5)
    // Same shape as cyd35 above, and for the same reason: the card is on the
    // DISPLAY's SPI bus here (SCK 6, MISO 2, MOSI 7), so it gets TFT_eSPI's
    // already-begun instance and nothing extra is ever attached to the bus.
    //
    // It matters more on this board than the comment above suggests, because
    // -DRLPHANTOM_R sends it to the #else branch otherwise, and that branch
    // runs SPI.begin(18, 19, 23, 5) -- pins chosen for the original CYD's
    // dedicated SD bus. Two separate faults on a C5: those are not this
    // board's SD pins, so the card never answers (observed:
    // "Card Failed! cmd: 0x00" and then "no card, or it did not answer"
    // with a working card inserted); and GPIO23 is this board's TFT_CS, so
    // the additive GPIO-matrix attach the comment above warns about lands on
    // the display's own chip select.
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
        _statusDetail = "MOUNT FAILED";
        Serial.printf("[sd] mount failed: attempt %lu, card previously seen=%d\n",
                      (unsigned long)_mountAttempts, (int)_everMounted);
        return false;
    }
    Serial.printf("[sd] card mounted: %llu MB\n", (unsigned long long)(SD.cardSize() >> 20));
#if defined(CYD) && !defined(AWOK) && !defined(CYD35) && !defined(RLPHANTOM) && !defined(RLPHANTOM_R) && !defined(TWATCH_S3)
    // Capture the FATFS object SDFS already registered. Never create
    // another one on card removal: repeated malloc/free of this FAT/VFS
    // context was exhausting or fragmenting the small internal heap.
    // f_getfree also ensures the selected drive is a mounted FAT SD card.
    if (!_retainedFs) {
        for (uint8_t pd = 0; pd < FF_VOLUMES; ++pd) {
            if (sdcard_type(pd) == CARD_NONE) continue;
            char drive[3] = {static_cast<char>('0' + pd), ':', 0};
            FATFS* fs = nullptr;
            DWORD freeClusters = 0;
            const FRESULT fr = f_getfree(drive, &freeClusters, &fs);
            if (fr == FR_OK && fs) {
                _retainedPdrv = pd;
                _retainedFs = fs;
                _retainedDetached = false;
                Serial.printf("[sd] retained FAT/VFS context on drive %u for hot swap\n", (unsigned)pd);
                break;
            }
            Serial.printf("[sd] retained FAT probe drive %u result %d\n", (unsigned)pd, (int)fr);
        }
        if (!_retainedFs)
            Serial.println("[sd] WARNING: no retained FAT context; legacy full remount only");
    }
#endif
    _ready = true;
    _everMounted = true;
    _statusDetail = "SD READY";
    _pendingLen = 0;   // old RAM queue was associated with the old card
    _filename[0] = '\0';
    Theme::setSdStatus(Theme::SdStatus::READY);
    openDaily();
    _lastFlush = millis();
    return true;
}

// A removed card can make SD.open()/File.write()/File.close() wait for SPI
// timeouts. Repeated retry in the UI loop makes the whole device stutter.
// A failed flush therefore latches logging off until reboot. There is no
// reliable card-detect input on the supported CYDs; guessing that the card
// has been reinserted would reintroduce the stalls during normal scanning.
// Do not tear down and reallocate SDFS/VFS when the card is physically
// swapped. sdcard_unmount() only marks the registered disk uninitialized and
// detaches the logical FAT volume; it does NOT sdcard_uninit(), unregister
// /sd, or free the existing SDFS object. disk_initialize() then performs
// the full new-card handshake, and f_mount() reuses the retained FATFS.
bool SdLog::remountRetained() {
#if defined(CYD) && !defined(AWOK) && !defined(CYD35) && !defined(RLPHANTOM) && !defined(RLPHANTOM_R) && !defined(TWATCH_S3)
    if (!_retainedFs || !_retainedDetached || _retainedPdrv >= FF_VOLUMES)
        return false;
    const uint8_t pd = _retainedPdrv;
    const char drive[3] = {static_cast<char>('0' + pd), ':', 0};
    const DSTATUS state = disk_initialize(pd);
    Serial.printf("[sd] retained drive %u disk_initialize status=0x%02X\n",
                  (unsigned)pd, (unsigned)state);
    if (state & STA_NOINIT) {
        _statusDetail = "SD REINSERT INIT FAILED";
        return false;
    }
    const FRESULT fr = f_mount(static_cast<FATFS*>(_retainedFs), drive, 1);
    Serial.printf("[sd] retained FAT mount result=%d, freeheap=%lu\n",
                  (int)fr, (unsigned long)ESP.getFreeHeap());
    if (fr != FR_OK) {
        // A failed f_mount must leave the existing context detached.
        // Nothing is freed and no volume data is modified.
        sdcard_unmount(pd);
        _statusDetail = fr == FR_NO_FILESYSTEM ? "SD FAT NOT RECOGNIZED"
                     : fr == FR_NOT_READY ? "SD CARD NOT READY"
                     : "SD RETAINED FAT FAIL";
        return false;
    }
    _retainedDetached = false;
    _ready = true;
    _everMounted = true;
    _pendingLen = 0;
    _filename[0] = '\0';
    _statusDetail = "SD READY (REUSED VFS)";
    Theme::setSdStatus(Theme::SdStatus::READY);
    _lastFlush = millis();
    openDaily();
    Serial.println("[sd] hot reinsertion succeeded without VFS realloc");
    return true;
#else
    return false;
#endif
}

void SdLog::disableAfterIoFailure(const char* operation) {
    if (!_ready) return;
    ++_flushFailures;
    Serial.printf("[sd] %s failed; disabling SD logging until reboot (pending %u bytes discarded)\n",
                  operation, (unsigned)_pendingLen);
    _ready = false;
#if defined(CYD) && !defined(AWOK) && !defined(CYD35) && !defined(RLPHANTOM) && !defined(RLPHANTOM_R) && !defined(TWATCH_S3)
    if (_retainedFs && !_retainedDetached && _retainedPdrv < FF_VOLUMES) {
        // This path runs inside the SD worker or under the existing SD
        // mutex after a completed file close. No other task may be
        // using a file while FatFs is detached.
        sdcard_unmount(_retainedPdrv);
        _retainedDetached = true;
        Serial.printf("[sd] hot-swap: kept VFS allocation for drive %u\n",
                      (unsigned)_retainedPdrv);
    }
#endif
    _statusDetail = "CARD REMOVED / IO FAILED";
    Theme::setSdStatus(Theme::SdStatus::ERROR);
    _pendingLen = 0;
    _filename[0] = '\0';
}

void SdLog::browseFault() {
    disableAfterIoFailure("browser I/O");
}



// Send a real low-speed SD SPI CMD0 before attempting SDFS's full mount.
// Merely sending 0xFF idle clocks (R7) cannot distinguish an empty socket,
// a card that does not answer, and a card with an invalid FAT volume.
// This is used ONLY in the background remount worker, under _ioMutex.
// Return 0x01 for SD-idle ACK, or the actual response/0xFF for no ACK.
#if defined(CYD) && !defined(RLPHANTOM) && !defined(RLPHANTOM_R) && !defined(CYD35)
static uint8_t sdRemountProbeCmd0() {
    static const uint8_t cmd0[6] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
    uint8_t reply = 0xFF;
    SPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
    digitalWrite(SD_CS_PIN, HIGH);
    for (int i = 0; i < 16; ++i) SPI.transfer(0xFF); // >=128 idle clocks
    for (int attempt = 0; attempt < 3; ++attempt) {
        digitalWrite(SD_CS_PIN, LOW);
        SPI.transfer(0xFF);
        for (uint8_t byte : cmd0) SPI.transfer(byte);
        reply = 0xFF;
        for (int i = 0; i < 12; ++i) {
            const uint8_t b = SPI.transfer(0xFF);
            if ((b & 0x80u) == 0) { reply = b; break; }
        }
        digitalWrite(SD_CS_PIN, HIGH);
        SPI.transfer(0xFF);
        if (reply == 0x01) break;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    SPI.endTransaction();
    return reply;
}
#endif


// R9 post-failure differential diagnosis, performed only when the ordinary
// Arduino SDFS mount fails AFTER the physical CMD0 probe answered 0x01.
// The Arduino core's SD.begin() returns the same false for failures in
// card initialization, sector reads, FAT and VFS registration. The former
// "CMD0 OK; FAT MOUNT FAIL" label falsely blamed FAT for all four.
//
// This probe registers exactly the CYD SD CS/SPI as a TEMPORARY diskio drive,
// initializes it through the pinned Arduino-ESP32 SD driver's own dispatch,
// reads sector zero, mounts that SAME raw drive read-only via FatFs, unmounts
// it again, then unregisters the temporary drive. No writes or formatting.
// SD SPI and FS operations are serialized by the worker's _ioMutex.
// Returns a durable literal for rendering by the UI without a live buffer.
static const char* sdDiagnosePostCmd0MountFailure(uint8_t* sector) {
#if defined(CYD) && !defined(CYD35) && !defined(RLPHANTOM) && !defined(RLPHANTOM_R)
    const uint8_t pdrv = sdcard_init(SD_CS_PIN, &SPI, 4000000);
    if (pdrv == 0xFF || pdrv >= FF_VOLUMES || pdrv > 9) {
        Serial.printf("[sd diag] raw driver registration FAILED: pdrv=%u\n", (unsigned)pdrv);
        return "RAW DRIVER REGISTER FAIL";
    }
    const DSTATUS status = disk_initialize(pdrv);
    Serial.printf("[sd diag] physical disk_initialize(pdrv=%u) status=0x%02X\n",
                  (unsigned)pdrv, (unsigned)status);
    const char* stage = nullptr;
    if (status & STA_NOINIT) {
        stage = "SD FULL INIT FAILED";
    } else if (disk_read(pdrv, sector, 0, 1) != RES_OK) {
        stage = "SD SECTOR READ FAILED";
        Serial.println("[sd diag] sector 0 read FAILED");
    } else {
        // Read-only FAT test: f_mount() may read metadata but does not
        // format or alter the user card. Unregister it before leaving.
        FATFS* fs = static_cast<FATFS*>(calloc(1, sizeof(FATFS)));
        if (!fs) {
            stage = "FAT TEST NO MEMORY";
        } else {
            const char drive[3] = {char('0' + pdrv), ':', '\0'};
            const FRESULT fr = f_mount(fs, drive, 1);
            Serial.printf("[sd diag] raw f_mount(%s) result=%d, MBR/boot signature=%02X%02X\n",
                          drive, (int)fr, (unsigned)sector[511], (unsigned)sector[510]);
            if (fr == FR_OK)
                stage = "FAT OK; VFS MOUNT FAIL";
            else if (fr == FR_NO_FILESYSTEM)
                stage = "FAT NOT RECOGNIZED";
            else if (fr == FR_DISK_ERR || fr == FR_NOT_READY)
                stage = "FAT READ/READY ERROR";
            else
                stage = "FAT MOUNT ERROR";
            // Avoid unregistering the diskio driver while the temporary
            // FatFs instance is still registered. This never calls mkfs.
            f_mount(nullptr, drive, 0);
            free(fs);
        }
    }
    sdcard_uninit(pdrv);
    return stage ? stage : "MOUNT DIAG UNKNOWN";
#else
    (void)sector;
    return "SD MOUNT FAILED";
#endif
}

// Monitor once per few seconds in its own FreeRTOS task. No SD SPI
// transaction occurs in title-bar painting or the main animation loop.
// SD.readRAW() is the SDFS wrapper around a real 512-byte sector read:
// unlike SD.cardType()/cardSize(), it is NOT a cached presence flag.
void SdLog::monitorEntry(void* ctx) {
    SdLog* self = static_cast<SdLog*>(ctx);
    uint8_t* sector = static_cast<uint8_t*>(malloc(512));
    // Avoid repeated full SDFS allocations after a failed *first*
    // mount. Re-arm only after a physical absence is observed.
    bool coldCardPresenceLatched = false;
    for (;;) {
        if (!self) { vTaskDelay(pdMS_TO_TICKS(2500)); continue; }
        // RESCAN interrupts the regular 2.5 second delay. It does not
        // call synchronous FAT/SPI APIs on the touchscreen/UI thread.
        const uint32_t signalled = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2500));
        if (!sector) continue;
        const SemaphoreHandle_t m = static_cast<SemaphoreHandle_t>(self->_ioMutex);
        if (!m || xSemaphoreTake(m, pdMS_TO_TICKS(150)) != pdTRUE) continue;
        // A file browser can be open while a card is removed/reinserted.
        // Skip raw reads while browsing, but ALWAYS allow an offline remount.
        // The browser borrows the same mutex only around individual I/O.
        // R8: cold-boot with no SD used to stay offline until RESCAN,
        // because repeatedly calling SD.begin() without a card allocates
        // and frees a large FAT/VFS context. On the independent SPI CYDs,
        // probe physical presence via low-speed CMD0 ONLY (no FAT or VFS
        // allocations) every 2.5 seconds. First positive response triggers
        // the existing first-mount path; once it succeeds R7b's retained
        // FAT/VFS hot-swap takes over unchanged.
        //
        // If the first full mount fails with the card present, do NOT
        // repeatedly allocate/free VFS on a timer. An explicit RESCAN or
        // a confirmed absence followed by reinsertion permits another try.
        if (!self->_ready && !self->_everMounted &&
            !self->_requestRescan && !signalled) {
#if defined(CYD) && !defined(AWOK) && !defined(CYD35) && !defined(RLPHANTOM) && !defined(RLPHANTOM_R) && !defined(TWATCH_S3)
            pinMode(SD_CS_PIN, OUTPUT);
            digitalWrite(SD_CS_PIN, HIGH);
            SPI.begin(18, 19, 23, SD_CS_PIN);
            const uint8_t coldAck = sdRemountProbeCmd0();
            if (coldAck != 0x01) {
                // No card: do not call SD.end(), SD.begin(), or f_mount().
                // Re-arm auto detect when the slot becomes empty again.
                coldCardPresenceLatched = false;
                self->_statusDetail = "SD OFFLINE - AUTO DETECT";
                xSemaphoreGive(m);
                continue;
            }
            if (coldCardPresenceLatched) {
                self->_statusDetail = "SD SEEN - TAP RESCAN";
                xSemaphoreGive(m);
                continue;
            }
            coldCardPresenceLatched = true;
            Serial.println("[sd] first SD insertion detected by CMD0; attempting initial mount");
#else
            // Shared-SPI variants retain their prior manual RESCAN policy.
            self->_statusDetail = "SD OFFLINE - TAP RESCAN";
            xSemaphoreGive(m);
            continue;
#endif
        }
        if (!self->_browsing || !self->_ready) {
            if (self->_ready) {
                const uint32_t started = millis();
                const bool ok = SD.readRAW(sector, 0);
                const uint32_t elapsed = (uint32_t)(millis() - started);
                if (!ok || elapsed >= 500u) {
                    self->disableAfterIoFailure(ok ? "SD health timeout" : "card removed");
                }
            } else {
                // When the card is put back, clear Arduino's stale disk
                // handle before mounting it again. Never do this inside
                // the frame or from the title-bar icon.
                self->_remounting = true;
                self->_statusDetail = "REINITIALIZING SD";
                // The old path called SD.end() on EVERY hot-remount,
                // freeing the FAT/VFS context and forcing another large
                // contiguous allocation amid active BLE/WiFi/display work.
                // A detached retained drive already owns its FAT context;
                // do NOT call SD.end() or SD.begin() for that drive.
                const bool retained = self->_retainedDetached &&
                                      self->_retainedFs != nullptr;
                if (!retained) SD.end();
#if defined(CYD) && !defined(RLPHANTOM) && !defined(RLPHANTOM_R) && !defined(CYD35)
                // Do not forcibly unregister /sd from here. The Arduino
                // SDFS owner must release its own FAT/VFS bookkeeping; a
                // second unregister from this worker can invalidate a FATFS
                // object still referenced by SDFS after an I/O failure.
                // R10's unconditional external cleanup is rolled back until
                // there is verified proof that a stale registration exists.
                // This is a dedicated SPI bus on CYD 2.8/3.2, not the
                // TFT/touch instance. Reset the bus after a disappeared card,
                // as Arduino SDFS::end() only detaches the FAT volume.
                SPI.end();
                // A physically reinserted card needs its CS deasserted and
                // >= 74 clocks before CMD0; allow the socket contacts to
                // settle and issue the idle clocks on the dedicated SD bus.
                pinMode(SD_CS_PIN, OUTPUT);
                digitalWrite(SD_CS_PIN, HIGH);
                vTaskDelay(pdMS_TO_TICKS(120));
                SPI.begin(18, 19, 23, SD_CS_PIN);
                const uint8_t ack = sdRemountProbeCmd0();
                if (ack != 0x01) {
                    self->_remounting = false;
                    self->_statusDetail = ack == 0xFF ? "NO CMD0 RESPONSE" : "CMD0 REJECTED";
                    ++self->_probeFailures;
                    Serial.printf("[sd] remount probe FAILED: CMD0=0x%02X, count=%lu (no FAT attempt)\n",
                                  (unsigned)ack, (unsigned long)self->_probeFailures);
                    // Preserve the retained VFS context even if no card is
                    // in the slot. SPI is reinitialized on the next probe.
                    SPI.end();
                    self->_requestRescan = false;
                    xSemaphoreGive(m);
                    continue;
                }
                self->_statusDetail = "CMD0 OK; MOUNTING";
                Serial.println("[sd] remount probe: CMD0=0x01 (card SPI responsive)");
#endif
                bool mounted = retained
                    ? self->remountRetained()
                    : self->begin();
                if (!mounted && !retained) {
                    // Raw FAT success only proves the card is readable. It
                    // does NOT prove that the previous SD.begin failed at the
                    // ESP-IDF VFS step. R3's alternate path was a false fix.
                    const char* stage = sdDiagnosePostCmd0MountFailure(sector);
                    self->_statusDetail = stage;
                    Serial.printf("[sd diag] raw remount stage: %s\n", stage);

                    if (strcmp(stage, "FAT OK; VFS MOUNT FAIL") == 0) {
                        // R5 hardware proved two VFS FILE slots fail to
                        // allocate while one succeeds. This R6 build uses
                        // one slot for both SD.begin and this diagnostic.
                        // A directory handle does not use a VFS FIL slot.
                        // No f_mount, card writes, or formatting here.
                        FATFS* probeFs = nullptr;
                        const size_t largestBefore =
                            heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
                        const esp_err_t reg = esp_vfs_fat_register(
                            "/sd", "0:", SD_MAX_FILES, &probeFs);
                        Serial.printf("[sd diag] VFS /sd probe files=%u, result=0x%x, FIL=%u B, heap=%lu, largest=%lu B\n",
                                      (unsigned)SD_MAX_FILES, (unsigned)reg,
                                      (unsigned)sizeof(FIL),
                                      (unsigned long)ESP.getFreeHeap(),
                                      (unsigned long)largestBefore);

                        if (reg == ESP_ERR_INVALID_STATE) {
                            // Re-check SD ownership before cleaning: a live
                            // Arduino SDFS mount must never be invalidated.
                            // SD.begin() failed and reset _pdrv to 0xFF;
                            // CARD_NONE is a defensive second check.
                            if (SD.cardType() != CARD_NONE) {
                                self->_statusDetail = "VFS OWNED - NO CLEANUP";
                                Serial.println("[sd diag] SD still owns media; refusing VFS cleanup");
                            } else {
                                const esp_err_t cleared = esp_vfs_fat_unregister_path("/sd");
                                Serial.printf("[sd diag] stale /sd context cleanup=0x%x\n",
                                              (unsigned)cleared);
                                if (cleared == ESP_OK) {
                                    // A confirmed orphaned /sd context was
                                    // removed. Retry only the canonical /sd
                                    // mount, never another namespace, and only
                                    // once in this worker cycle.
                                    mounted = self->begin();
                                    self->_statusDetail = mounted
                                        ? "SD READY (VFS REPAIRED)"
                                        : "VFS CLEARED; MOUNT FAIL";
                                    Serial.printf("[sd diag] canonical /sd retry %s\n",
                                                  mounted ? "SUCCESS" : "FAILED");
                                } else {
                                    self->_statusDetail = "VFS CLEANUP FAILED";
                                }
                            }
                        } else if (reg == ESP_OK) {
                            // It was NOT an occupied /sd mountpoint. Remove
                            // only the probe context we just allocated.
                            const esp_err_t unreg = esp_vfs_fat_unregister_path("/sd");
                            Serial.printf("[sd diag] VFS probe unregister=0x%x\n",
                                          (unsigned)unreg);
                            self->_statusDetail = unreg == ESP_OK
                                ? "VFS PATH FREE; SD FAIL"
                                : "VFS PROBE CLEANUP FAIL";
                        } else if (reg == ESP_ERR_NO_MEM) {
                            self->_statusDetail = "SD 1-FILE ALLOC FAILED";
                        } else {
                            self->_statusDetail = "VFS PROBE ERROR";
                        }
                    }
                }
                self->_remounting = false;
                if (!mounted && (self->_requestRescan || signalled))
                    Serial.println("[sd] RESCAN attempted, SD still offline");
                self->_requestRescan = false;
            }
        }
        xSemaphoreGive(m);
    }
}

void SdLog::startMonitor() {
#if defined(CYD) && !defined(TWATCH_S3)
    if (_monitorTask) return;
    if (!_ioMutex) _ioMutex = xSemaphoreCreateMutex();
    if (!_ioMutex) { Serial.println("[sd] monitor unavailable: no mutex"); return; }
    TaskHandle_t task = nullptr;
    if (xTaskCreatePinnedToCore(monitorEntry, "sd_presence", 4096, this, 1,
                                &task, 0) == pdPASS)
        _monitorTask = task;
    else Serial.println("[sd] monitor task allocation failed");
#endif
}

void SdLog::browseBegin() {
    // No SD operation here: opening the menu must not wait for a hot-unplug
    // SPI timeout in the worker. Each actual browser file operation takes
    // the mutex separately (try-lock, never blocks the display task).
    _browsing = true;
    _pendingLen = 0;
}
void SdLog::browseEnd() {
    _browsing = false;
}
bool SdLog::browseLock() {
    const SemaphoreHandle_t m = static_cast<SemaphoreHandle_t>(_ioMutex);
    return !m || xSemaphoreTake(m, 0) == pdTRUE;
}
void SdLog::browseUnlock() {
    const SemaphoreHandle_t m = static_cast<SemaphoreHandle_t>(_ioMutex);
    if (m) xSemaphoreGive(m);
}
bool SdLog::rescan() {
    // Do not manipulate SPI/FAT on the UI thread. The monitor wakes up
    // independently and makes the next remount attempt under its own lock.
    _requestRescan = true;
    if (_monitorTask) xTaskNotifyGive(static_cast<TaskHandle_t>(_monitorTask));
    return _monitorTask != nullptr;
}

// File deletion deliberately replaces FAT formatting. It operates only on an
// already mounted card, using the same SD.remove() path the browser verifies.
// It never touches the partition table, FAT geometry, NVS, or SPI registration.
//
// The parent uses a DIR iterator; openNextFile opens at most one FIL.
// Close both before deleting or descending; the VFS needs only one FILE
// slot. Re-open the parent each iteration to find the first remaining
// entry after delete.
static bool sdEraseDirectory(const char* folder, unsigned depth,
                             uint32_t& files, uint32_t& dirs,
                             char* failure, size_t failCap) {
    if (depth >= 12) {
        snprintf(failure, failCap, "TOO MANY FOLDER LEVELS");
        return false;
    }
    for (;;) {
        File parent = SD.open(folder, FILE_READ);
        if (!parent || !parent.isDirectory()) {
            if (parent) parent.close();
            snprintf(failure, failCap, "CANNOT OPEN FOLDER");
            return false;
        }
        File item = parent.openNextFile();
        if (!item) {
            parent.close();
            return true; // all entries deleted, including nested folders
        }
        const char* rawName = item.name();
        const char* base = rawName ? strrchr(rawName, '/') : nullptr;
        base = base ? base + 1 : rawName;
        const bool isDir = item.isDirectory();
        char path[192];
        const bool safeName = base && *base && strcmp(base, ".") != 0 &&
                              strcmp(base, "..") != 0 && !strchr(base, '\\');
        const int n = safeName
            ? snprintf(path, sizeof path, "%s%s%s",
                       strcmp(folder, "/") == 0 ? "" : folder, "/", base)
            : -1;
        item.close();
        parent.close();
        if (!safeName || n <= 0 || (size_t)n >= sizeof path) {
            snprintf(failure, failCap, "INVALID OR LONG FILENAME");
            return false; // never use a truncated or synthetic path
        }
        if (isDir) {
            if (!sdEraseDirectory(path, depth + 1, files, dirs, failure, failCap))
                return false;
            if (!SD.rmdir(path)) {
                snprintf(failure, failCap, "REMOVE FOLDER FAILED");
                Serial.printf("[sd wipe] rmdir failed: %s\n", path);
                return false;
            }
            ++dirs;
        } else {
            if (!SD.remove(path)) {
                snprintf(failure, failCap, "DELETE FILE FAILED");
                Serial.printf("[sd wipe] remove failed: %s\n", path);
                return false;
            }
            ++files;
        }
        // A large card can take a while. Yield so the ESP32 watchdog and radio
        // tasks continue running instead of resetting during file deletion.
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// Double-confirmed from the browser. A card with a broken/unmounted FAT
// volume cannot be wiped by SD.remove(); request a computer reformat instead.
bool SdLog::wipeAllFiles() {
    const SemaphoreHandle_t m = static_cast<SemaphoreHandle_t>(_ioMutex);
    if (m && xSemaphoreTake(m, pdMS_TO_TICKS(500)) != pdTRUE) {
        snprintf(_wipeError, sizeof _wipeError, "SD BUSY - RETRY");
        return false;
    }
    struct Unlock { SemaphoreHandle_t m; ~Unlock() { if (m) xSemaphoreGive(m); } } unlock{m};
    if (!_browsing || !_ready) {
        snprintf(_wipeError, sizeof _wipeError, "INSERT / MOUNT SD FIRST");
        return false;
    }

    // Clear buffered rows: don't recreate a just-deleted log on the next
    // timer tick. A new detection may start a fresh file later, as expected.
    _pendingLen = 0;
    _filename[0] = '\0';
    _lastFlush = millis();

    uint32_t files = 0, dirs = 0;
    char failure[48] = {0};
    const bool ok = sdEraseDirectory("/", 0, files, dirs, failure, sizeof failure);
    if (!ok) {
        snprintf(_wipeError, sizeof _wipeError, "%s", failure);
        Serial.printf("[sd wipe] failed after %lu files, %lu dirs: %s\n",
                      (unsigned long)files, (unsigned long)dirs, failure);
        // Do not claim that nothing was deleted: the operation is NOT atomic.
        // Keep the mounted card available to the browser for inspection.
        return false;
    }
    snprintf(_wipeError, sizeof _wipeError, "%lu FILES, %lu FOLDERS",
             (unsigned long)files, (unsigned long)dirs);
    Serial.printf("[sd wipe] complete: %lu files, %lu folders deleted\n",
                  (unsigned long)files, (unsigned long)dirs);
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

// UI thread never waits for the monitor task's raw read/remount:
 // any flush that would contend simply stays queued until the next tick.
bool SdLog::flushPending() {
    const SemaphoreHandle_t m = static_cast<SemaphoreHandle_t>(_ioMutex);
    if (m && xSemaphoreTake(m, 0) != pdTRUE) return false;
    const bool ok = flushPendingLocked();
    if (m) xSemaphoreGive(m);
    return ok;
}
bool SdLog::flushPendingLocked() {
    if (!_ready || _pendingLen == 0 || _browsing) return true;
    if (_filename[0] == '\0') openDaily();
    if (_filename[0] == '\0') return false;

    // Rate-limit retry attempts when a card disappears or temporarily stops
    // answering. A successful close() commits FatFS' cached sector metadata.
    _lastFlush = millis();
    const uint32_t ioStartedAt = _lastFlush;
    File f = SD.open(_filename, FILE_APPEND);
    if (!f) {
        disableAfterIoFailure("open");
        return false;
    }

    const size_t written = f.write((const uint8_t*)_pending, _pendingLen);
    // The Arduino File::close() API returns no error status, although the FAT
    // sync may be the very operation that stalls after hot removal. Measure
    // the COMPLETE append, including close(), to catch this case as well.
    f.close();
    const uint32_t elapsedMs = (uint32_t)(millis() - ioStartedAt);
    const auto verdict = SdIoPolicy::evaluate(true, _pendingLen, written, elapsedMs);
    if (verdict == SdIoPolicy::Verdict::SHORT_WRITE) {
        disableAfterIoFailure("write");
        return false;
    }
    if (verdict == SdIoPolicy::Verdict::SLOW_IO) {
        disableAfterIoFailure("slow SD append");
        return false;
    }
    _pendingLen = 0;
    return true;
}

void SdLog::logEvent(const Detection& d) {
    if (!_ready || _browsing) return;
    // Monitor may invalidate/remount media on core 0. Protect the entire
    // formatting/enqueue operation, not just the eventual flush: it also
    // clears the pending buffer and filename on a card-state transition.
    const SemaphoreHandle_t m = static_cast<SemaphoreHandle_t>(_ioMutex);
    if (m && xSemaphoreTake(m, 0) != pdTRUE) return;
    struct Unlock {
        SemaphoreHandle_t m;
        ~Unlock() { if (m) xSemaphoreGive(m); }
    } unlock{m};
    if (!_ready || _browsing) return;
    if (_filename[0] == '\0') openDaily();

    char line[128];
    char timestamp[28];
    SdTimestamp::format(timestamp, sizeof timestamp, Clock::trusted(),
                        Clock::nowEpoch(), millis());
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
                           "%s,%s,%d,%s,%u,%s,%s\n",
                           timestamp,
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
        flushPendingLocked(); // logEvent already owns the SD mutex
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
        flushPendingLocked(); // mutex already held by logEvent()
    }
}

// One evidence file per watched/hunted MAC, or classified Evil Twin MAC.
// A successful evidence write is capped at one per target/type every five
// seconds. No radio callback ever invokes this method or touches SD.
void SdLog::logTargetEvidence(const Detection& d, bool watched, bool hunted) {
    const bool rogue = d.type == DetectionType::EVILTWIN;
    if ((!watched && !hunted && !rogue) || !_ready || _browsing) return;
    const uint32_t now = millis();
    struct Recent { uint32_t key, at; bool used; };
    static Recent recent[8] = {};
    uint32_t key = 2166136261u ^ (uint32_t)d.type ^ (rogue ? 0xEEu : watched ? 0x57u : 0x48u);
    for (uint8_t i = 0; i < 6; i++) key = (key ^ d.mac[i]) * 16777619u;
    Recent& slot = recent[key % 8];
    if (slot.used && slot.key == key && now - slot.at < 5000u) return;

    char path[40];
    snprintf(path, sizeof path,
             rogue ? "/evilportal-%02X%02X%02X%02X%02X%02X.log"
                   : "/scan-%02X%02X%02X%02X%02X%02X.log",
             d.mac[0], d.mac[1], d.mac[2], d.mac[3], d.mac[4], d.mac[5]);
    char ts[28];
    SdTimestamp::format(ts, sizeof ts, Clock::trusted(), Clock::nowEpoch(), now);
    char mac[18];
    snprintf(mac, sizeof mac, "%02X:%02X:%02X:%02X:%02X:%02X",
             d.mac[0], d.mac[1], d.mac[2], d.mac[3], d.mac[4], d.mac[5]);
    char safe[sizeof d.name] = {};
    for (size_t i = 0; i + 1 < sizeof safe && d.name[i]; ++i) {
        const unsigned char c = (unsigned char)d.name[i];
        safe[i] = c < 32 || c > 126 || c == ',' || c == '"' ? '.' : (char)c;
    }
    if (safe[0] == '=' || safe[0] == '+' || safe[0] == '-' || safe[0] == '@')
        safe[0] = '_';
    const char* mode = rogue ? "EVILTWIN" : watched && hunted ? "WATCH+HUNT"
                       : watched ? "WATCH" : "HUNT";
    char row[160];
    const int n = snprintf(row, sizeof row, "%s,%s,%s,%s,%d,%u,%s\n",
                           ts, mode, detectionTypeName(d.type), mac,
                           (int)d.rssi, (unsigned)d.channel, safe);
    if (n <= 0 || (size_t)n >= sizeof row) return;

    // The monitor owns the dedicated SD SPI bus on another task. A failed
    // immediate lock acquisition drops optional evidence, never stalls UI.
    const SemaphoreHandle_t mutex = static_cast<SemaphoreHandle_t>(_ioMutex);
    if (mutex && xSemaphoreTake(mutex, 0) != pdTRUE) return;
    struct Unlock { SemaphoreHandle_t mutex; ~Unlock() { if (mutex) xSemaphoreGive(mutex); } } unlock{mutex};
    if (!_ready || _browsing) return;
    const uint32_t started = millis();
    const bool existed = SD.exists(path);
    File out = SD.open(path, FILE_APPEND);
    if (!out) { disableAfterIoFailure("evidence open"); return; }
    bool ok = true;
    if (!existed) {
        static const char hdr[] = "timestamp,mode,type,mac,rssi,channel,name\n";
        ok = out.write((const uint8_t*)hdr, sizeof hdr - 1) == sizeof hdr - 1;
    }
    if (ok) ok = out.write((const uint8_t*)row, (size_t)n) == (size_t)n;
    out.close();
    if (!ok || (uint32_t)(millis() - started) >= 500u) {
        disableAfterIoFailure(ok ? "slow evidence write" : "evidence write");
        return;
    }
    slot = {key, now, true};
}

// Best-effort evidence from the bounded NimBLE mailbox. SD I/O never runs
// inside NimBLE callbacks. Exact duplicate payloads are paced for five seconds
// to limit flash churn and keep scanning/UI responsive.
void SdLog::logWatchAdvertisement(const uint8_t mac[6], const uint8_t* bytes,
                                   uint8_t len, uint16_t originalLen, int8_t rssi) {
    if (!_ready || _browsing || !mac || !bytes || len == 0 || len > 64) return;
    const uint32_t now = millis();
    static struct { uint32_t hash, at; bool used; } recent[8] = {};
    uint32_t hash = 2166136261u;
    for (uint8_t i = 0; i < 6; i++) hash = (hash ^ mac[i]) * 16777619u;
    for (uint8_t i = 0; i < len; i++) hash = (hash ^ bytes[i]) * 16777619u;
    hash = (hash ^ len) * 16777619u;
    hash = (hash ^ originalLen) * 16777619u;
    auto& slot = recent[hash % 8];
    if (slot.used && slot.hash == hash && now - slot.at < 5000u) return;

    char path[40];
    snprintf(path, sizeof path, "/watchadv-%02X%02X%02X%02X%02X%02X.log",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    char timestamp[28];
    SdTimestamp::format(timestamp, sizeof timestamp, Clock::trusted(), Clock::nowEpoch(), now);
    static const char hexDigits[] = "0123456789ABCDEF";
    char hex[129];
    for (uint8_t i = 0; i < len; i++) {
        hex[2 * i] = hexDigits[bytes[i] >> 4];
        hex[2 * i + 1] = hexDigits[bytes[i] & 0x0F];
    }
    hex[2 * len] = '\0';
    char row[210];
    const int n = snprintf(row, sizeof row, "%s,%d,%u,%u,%s\n",
                           timestamp, (int)rssi, (unsigned)originalLen, (unsigned)len, hex);
    if (n <= 0 || (size_t)n >= sizeof row) return;

    const SemaphoreHandle_t mutex = static_cast<SemaphoreHandle_t>(_ioMutex);
    if (mutex && xSemaphoreTake(mutex, 0) != pdTRUE) return;
    struct Unlock { SemaphoreHandle_t mutex; ~Unlock() { if (mutex) xSemaphoreGive(mutex); } } unlock{mutex};
    if (!_ready || _browsing) return;
    const uint32_t started = millis();
    const bool existed = SD.exists(path);
    File out = SD.open(path, FILE_APPEND);
    if (!out) { disableAfterIoFailure("watched BLE advert open"); return; }
    bool ok = true;
    if (!existed) {
        static const char hdr[] = "timestamp,rssi,original_bytes,captured_bytes,ad_hex\n";
        ok = out.write((const uint8_t*)hdr, sizeof hdr - 1) == sizeof hdr - 1;
    }
    if (ok) ok = out.write((const uint8_t*)row, (size_t)n) == (size_t)n;
    out.close();
    if (!ok || (uint32_t)(millis() - started) >= 500u) {
        disableAfterIoFailure(ok ? "slow watched BLE advert write" : "watched BLE advert write");
        return;
    }
    slot = {hash, now, true};
}

// Duress wipe only touches exact application-owned file patterns. It never
// deletes a human's file such as scan-notes.log.
static bool isOwnedEvidenceFilename(const char* name, const char* prefix) {
    const size_t p = strlen(prefix);
    if (strncmp(name, prefix, p) != 0 || strlen(name) != p + 12u + 4u) return false;
    for (size_t i = 0; i < 12u; i++) {
        const char c = name[p + i];
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) return false;
    }
    return strcmp(name + p + 12u, ".log") == 0;
}

void SdLog::wipe() {
    if (!_ready || _browsing) return;
    const SemaphoreHandle_t m = static_cast<SemaphoreHandle_t>(_ioMutex);
    if (m && xSemaphoreTake(m, portMAX_DELAY) != pdTRUE) return;
    // RAII releases mutex through every exit from this multi-pass wipe.
    struct Unlock { SemaphoreHandle_t sem; ~Unlock() { if (sem) xSemaphoreGive(sem); } } unlock{m};

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
            if (strncmp(base, "squachwatch-", 12) == 0 ||
                isOwnedEvidenceFilename(base, "scan-") ||
                isOwnedEvidenceFilename(base, "evilportal-") ||
                isOwnedEvidenceFilename(base, "watchadv-")) {
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
    if (!_ready || _browsing) return;
    const SemaphoreHandle_t m = static_cast<SemaphoreHandle_t>(_ioMutex);
    if (m && xSemaphoreTake(m, 0) != pdTRUE) return;
    struct Unlock { SemaphoreHandle_t m; ~Unlock() { if (m) xSemaphoreGive(m); } } unlock{m};
    if (!_ready || _browsing) return;
    const uint32_t now = millis();

    // Commit sparse traffic within one second. Dense traffic usually reaches
    // WRITE_BUFFER_FLUSH_AT first and flushes from logEvent().
    if (_pendingLen && now - _lastFlush >= 1000) {
        flushPendingLocked();
    }

    // Re-evaluate the daily filename once an hour, preserving the previous
    // behavior. Buffered rows must be committed under the OLD filename before
    // switching; if a card error prevents that, leave the check due so the
    // next successful retry can rotate immediately afterward.
    static uint32_t lastDayCheck = 0;
    if (now - lastDayCheck >= 3600000) {
        if (_pendingLen == 0 || (now - _lastFlush >= 1000 && flushPendingLocked())) {
            lastDayCheck = now;
            openDaily();
        }
    }
}
