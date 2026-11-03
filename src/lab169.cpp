// LAB PR #169: Physical UART NMEA capture + verbose on-device diagnostics.
// No SD operations, no GPS command TX, no modification to production boards.
// Real GPS positions drive a synthetic Wi-Fi WiGLE formatter verification row.
#if defined(LAB_GNSS169)
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "lab169.h"
#include "gnss.h"
#include "wardrive.h"
#include "wifi_auth.h"

namespace Lab169 {
namespace {
// User-confirmed receiver speed: stay at 115200. Never reset the UBX parser
// by repeatedly probing other baud rates while binary NAV packets arrive.
constexpr uint32_t GNSS_BAUD = 115200u;
uint32_t baud = GNSS_BAUD;
uint32_t bytes = 0, rateBytes = 0, lastPrint = 0, lastCsv = 0, csvCount = 0;
uint32_t dollar = 0, newlines = 0, printable = 0, binary = 0, ubx = 0;
uint32_t gga = 0, rmc = 0, rxPeak = 0, lastRawPrint = 0;
bool csvPass = false, rawActive = false;
uint8_t lastByte = 0;
char raw[160] = {};
uint8_t rawN = 0;
char prefix[22] = "--";

const uint8_t RSN[] = {
    48,20, 1,0, 0,15,172,4, 1,0, 0,15,172,4, 1,0, 0,15,172,2, 0,0
};

void start(uint32_t now) {
    baud = GNSS_BAUD;
    Serial2.end();
    // The previous 256-byte RX ring and 160-byte-per-frame draining could
    // lose NMEA at high output rates. Configure BEFORE Serial2.begin().
    const size_t actual = Serial2.setRxBufferSize(4096);
    Serial2.begin(baud, SERIAL_8N1, 35, -1);
    switchedAt = now;
    rateBytes = dollar = newlines = printable = binary = ubx = 0;
    gga = rmc = rxPeak = 0;
    rawActive = false; rawN = 0; lastByte = 0;
    memcpy(prefix, "--", 3);
    Gnss::reset();
    Serial.printf("[LAB169] UART2 RX GPIO35: %lu baud, RX buffer=%u; looking for NMEA checksums\n",
                  (unsigned long)baud, (unsigned)actual);
}

void noteRaw(uint8_t b, uint32_t now) {
    if (b == '$') {
        ++dollar;
        rawActive = true;
        rawN = 0;
    }
    if (b == '\n') ++newlines;
    if (b >= 0x20 && b <= 0x7e) ++printable;
    else if (b != '\r' && b != '\n') ++binary;
    if (lastByte == 0xb5 && b == 0x62) ++ubx;
    lastByte = b;
    if (!rawActive) return;
    if (b == '\r' || b == '\n') {
        if (rawN) {
            raw[rawN] = 0;
            if (strstr(raw, "GGA,")) ++gga;
            if (strstr(raw, "RMC,")) ++rmc;
            const size_t n = strlen(raw);
            const size_t keep = (n < sizeof(prefix) - 1) ? n : sizeof(prefix) - 1;
            memcpy(prefix, raw, keep);
            prefix[keep] = 0;
            if (!lastRawPrint || now - lastRawPrint >= 8000u) {
                lastRawPrint = now ? now : 1;
                Serial.printf("[LAB169] RAW NMEA (sanitized, may be truncated): %s\n", raw);
            }
        }
        rawActive = false; rawN = 0;
        return;
    }
    if (rawN < sizeof(raw) - 1)
        raw[rawN++] = b >= 0x20 && b <= 0x7e ? (char)b : '.';
}

void formatCsv(const Gnss::Fix& f) {
    Wardrive::Record row = {};
    row.kind = Wardrive::KIND_WIFI;
    row.mac[0] = 0x02; row.mac[4] = 0x01; row.mac[5] = 0x69;
    memcpy(row.name, "LAB169_GPS", 10);
    row.nameLen = 10;
    row.rssi = -42;
    row.channel = 6;
    row.auth = WifiAuth::parse(0x0011, RSN, sizeof(RSN));
    row.lat7 = f.lat7; row.lon7 = f.lon7; row.altM = f.altM;
    row.accM = f.accM; row.epoch = f.epoch;
    char csv[256];
    size_t n = Wardrive::csvRow(row, csv, sizeof(csv));
    csvPass = n != 0 && strstr(csv, "LAB169_GPS") &&
              strstr(csv, "[WPA2-PSK-CCMP][ESS]") &&
              strstr(csv, ",WIFI\n") && csv[n-1] == '\n';
    if (csvPass) csvCount++;
    Serial.printf("[LAB169] WIGLE %s synthetic AP / real GNSS: %s",
                  csvPass ? "PASS" : "FAIL", n ? csv : "OVERFLOW\n");
}
} // namespace

void begin() {
    bytes = lastPrint = lastCsv = csvCount = lastRawPrint = 0;
    csvPass = false;
    start(millis());
    Serial.println("[LAB169] Hardware test: SYSTEM > DIAGNOSTICS; UART raw counters; no SD writes.");
}

void tick(uint32_t now) {
    // Drain UART thoroughly before drawing the next frame. The old 160-byte
    // budget could not keep up with 115200-baud full-constellation NMEA.
    // Bound by both 2048 bytes and ~4ms to keep touch, SD, BLE/Mesh responsive.
    const uint32_t beginUs = micros();
    uint16_t budget = 2048;
    int queued = Serial2.available();
    if (queued > 0 && (uint32_t)queued > rxPeak) rxPeak = (uint32_t)queued;
    while (budget-- && Serial2.available() > 0 &&
           (uint32_t)(micros() - beginUs) < 4000u) {
        const int ch = Serial2.read();
        if (ch < 0) break;
        ++bytes; ++rateBytes;
        noteRaw((uint8_t)ch, now);
        Gnss::feed((char)ch, now);
    }
    // Combined parser handles both NMEA and UBX on this single fixed UART.

    const Gnss::Fix& f = Gnss::fix();
    if (Gnss::fresh(now) && f.epoch && (!lastCsv || now - lastCsv >= 5000u)) {
        lastCsv = now ? now : 1;
        formatCsv(f);
    }
    if (!lastPrint || now - lastPrint >= 5000u) {
        lastPrint = now ? now : 1;
        const Gnss::Sky sky = Gnss::sky();
        Serial.printf("[LAB169] baud=%lu bytes=%lu rateBytes=%lu NMEA=%lu good/%lu bad fix=%u fresh=%u sat=%u/%u/%u lat7=%ld lon7=%ld UTC=%lu WIGLE=%s\n",
            (unsigned long)baud, (unsigned long)bytes, (unsigned long)rateBytes,
            (unsigned long)Gnss::good(), (unsigned long)Gnss::bad(),
            (unsigned)f.valid, (unsigned)Gnss::fresh(now),
            (unsigned)f.used, (unsigned)sky.view, (unsigned)sky.heard,
            (long)f.lat7, (long)f.lon7, (unsigned long)f.epoch,
            csvCount ? (csvPass ? "PASS" : "FAIL") : "WAIT");
        Serial.printf("[LAB169] UBX good=%lu bad=%lu PVT=%lu SAT=%lu last=%02X/%02X\n",
            (unsigned long)Gnss::ubxGood(), (unsigned long)Gnss::ubxBad(),
            (unsigned long)Gnss::ubxPvt(), (unsigned long)Gnss::ubxSat(),
            (unsigned)Gnss::ubxLastClass(), (unsigned)Gnss::ubxLastId());
        Serial.printf("[LAB169] RX $=%lu LF=%lu ASCII=%lu binary=%lu UBX=%lu GGA=%lu RMC=%lu RXpeak=%lu last=%s\n",
            (unsigned long)dollar, (unsigned long)newlines,
            (unsigned long)printable, (unsigned long)binary,
            (unsigned long)ubx, (unsigned long)gga, (unsigned long)rmc,
            (unsigned long)rxPeak, prefix);
    }
}
Status status(uint32_t now) {
    const Gnss::Fix& f = Gnss::fix();
    const Gnss::Sky sky = Gnss::sky();
    Status s = {};
    s.baud = baud; s.bytes = bytes; s.rateBytes = rateBytes;
    s.good = Gnss::good(); s.bad = Gnss::bad();
    s.ubxGood = Gnss::ubxGood(); s.ubxBad = Gnss::ubxBad();
    s.ubxPvt = Gnss::ubxPvt(); s.ubxSat = Gnss::ubxSat();
    s.ubxClass = Gnss::ubxLastClass(); s.ubxId = Gnss::ubxLastId();
    s.epoch = f.epoch; s.csvCount = csvCount;
    s.lat7 = f.lat7; s.lon7 = f.lon7;
    s.sats = f.used; s.view = sky.view; s.heard = sky.heard;
    s.valid = f.valid; s.fresh = Gnss::fresh(now); s.csvPass = csvPass;
    s.dollar = dollar; s.newlines = newlines;
    s.printable = printable; s.binary = binary; s.ubx = ubx;
    s.gga = gga; s.rmc = rmc; s.rxPeak = rxPeak;
    memcpy(s.prefix, prefix, sizeof(s.prefix));
    return s;
}
} // namespace Lab169
#endif
