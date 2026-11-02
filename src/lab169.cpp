// Physical GNSS/NMEA + formatter self-test for PR #169 on CYD LAB firmware.
// GPS module TX -> ESP32 RX GPIO35; GND shared; power per module specification.
// CSV demo uses actual GPS position with a SYNTHETIC WiFi AP, not wardriving.
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
uint32_t baud = 115200, switchedAt = 0, bytes = 0, lastPrint = 0, lastCsv = 0, csvCount = 0;
bool lockedBaud = false, csvPass = false;
const uint8_t RSN[] = {
    48,20, 1,0, 0,15,172,4, 1,0, 0,15,172,4, 1,0, 0,15,172,2, 0,0
};
void start(uint32_t rate, uint32_t now) {
    Serial2.end();
    Serial2.begin(rate, SERIAL_8N1, 35, -1);
    baud = rate;
    switchedAt = now;
    Gnss::reset();
    Serial.printf("[LAB169] GNSS UART2 RX GPIO35 %lu baud; NMEA RMC/GGA/GSV required\n",
                  (unsigned long)baud);
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
    baud = 115200; bytes = lastPrint = lastCsv = csvCount = 0;
    lockedBaud = csvPass = false;
    start(115200, millis());
    Serial.println("[LAB169] Hardware test: SYSTEM > DIAGNOSTICS, GPS169 rows at top. No SD writes.");
}
void tick(uint32_t now) {
    uint16_t budget = 160;
    while (budget-- && Serial2.available() > 0) {
        int ch = Serial2.read();
        if (ch < 0) break;
        ++bytes;
        Gnss::feed((char)ch, now);
    }
    if (Gnss::good()) lockedBaud = true;
    // BN-220 can use a custom 115200 baud or factory 9600 baud.
    if (!lockedBaud && now - switchedAt >= 12000u)
        start(baud == 115200 ? 9600 : 115200, now);
    const Gnss::Fix& f = Gnss::fix();
    if (Gnss::fresh(now) && f.epoch && (!lastCsv || now-lastCsv >= 5000u)) {
        lastCsv = now ? now : 1;
        formatCsv(f);
    }
    if (!lastPrint || now-lastPrint >= 5000u) {
        lastPrint = now ? now : 1;
        const Gnss::Sky sky = Gnss::sky();
        Serial.printf("[LAB169] baud=%lu bytes=%lu NMEA=%lu/%lu fix=%u fresh=%u SV=%u/%u/%u lat7=%ld lon7=%ld UTC=%lu WiGLE=%s rows=%lu\n",
            (unsigned long)baud, (unsigned long)bytes, (unsigned long)Gnss::good(),
            (unsigned long)Gnss::bad(), (unsigned)f.valid, (unsigned)Gnss::fresh(now),
            (unsigned)f.used, (unsigned)sky.view, (unsigned)sky.heard,
            (long)f.lat7, (long)f.lon7, (unsigned long)f.epoch,
            csvCount ? (csvPass ? "PASS" : "FAIL") : "WAIT", (unsigned long)csvCount);
    }
}
Status status(uint32_t now) {
    const Gnss::Fix& f = Gnss::fix();
    const Gnss::Sky sky = Gnss::sky();
    Status s = {};
    s.baud = baud; s.bytes = bytes; s.good = Gnss::good(); s.bad = Gnss::bad();
    s.epoch = f.epoch; s.csvCount = csvCount;
    s.lat7 = f.lat7; s.lon7 = f.lon7;
    s.sats = f.used; s.view = sky.view; s.heard = sky.heard;
    s.valid = f.valid; s.fresh = Gnss::fresh(now); s.csvPass = csvPass;
    return s;
}
} // namespace Lab169
#endif
