// SquachWatch-CYD: UBX parser used alongside NMEA, PR #169.
#include "ubx.h"
#include <string.h>
namespace Ubx {
namespace {
enum State : uint8_t { IDLE, SYNC2, CLASS, ID, LEN0, LEN1, PAYLOAD, CKA, CKB };
State state = IDLE;
// Store only small navigation messages (largest is NAV-PVT at 92 bytes).
// Large NAV-SAT satellite blocks are counted incrementally as they stream.
constexpr uint16_t MAX_PAYLOAD = 96;
uint8_t payload[MAX_PAYLOAD];
uint8_t satVersion=0, satNum=0, satView=0, satHeard=0;
uint8_t cls = 0, id = 0, sumA = 0, sumB = 0, expectedA = 0;
uint16_t len = 0, pos = 0;
uint32_t lastByteMs = 0, ok = 0, fail = 0, pvtCount = 0, satCount = 0;
uint32_t epoch = 0;
uint8_t seenCls = 0, seenId = 0;
Gnss::Fix f;
Gnss::Sky satellites;
bool skyValid = false, statusValid = false;

uint16_t get16(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
uint32_t get32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
int32_t getI32(const uint8_t* p) { return (int32_t)get32(p); }
void checksum(uint8_t b) {
    sumA = (uint8_t)(sumA + b);
    sumB = (uint8_t)(sumB + sumA);
}
uint8_t meters(uint32_t millimeters) {
    uint32_t m = (millimeters + 500u) / 1000u;
    return (uint8_t)(m < 1 ? 1 : m > 255 ? 255 : m);
}
bool sanePosition(int32_t lon, int32_t lat) {
    return lon >= -1800000000 && lon <= 1800000000 &&
           lat >= -900000000 && lat <= 900000000;
}
void position(int32_t lon, int32_t lat, int32_t heightMm,
              uint32_t accMm, uint32_t nowMs, uint32_t utc) {
    if (!sanePosition(lon, lat)) return;
    f.valid = true;
    f.lon7 = lon;
    f.lat7 = lat;
    const int32_t h = heightMm / 1000;
    f.altM = (int16_t)(h > 32767 ? 32767 : h < -32768 ? -32768 : h);
    f.accM = meters(accMm);
    f.epoch = utc;
    f.atMs = nowMs;
}
void setUtc(int yr, int month, int day, int hh, int mm, int ss) {
    if (yr < 2000 || yr > 2099 || month < 1 || month > 12 ||
        day < 1 || day > 31 || hh > 23 || mm > 59 || ss > 60) return;
    epoch = Gnss::toEpoch(yr, month, day, hh, mm, ss);
}
void navPvt(const uint8_t* p, uint16_t n, uint32_t nowMs) {
    if (n != 92) return;
    ++pvtCount;
    f.used = p[23]; // numSV
    const bool timeValid = (p[11] & 3u) == 3u;
    if (timeValid)
        setUtc(get16(p+4),p[6],p[7],p[8],p[9],p[10]);
    const uint8_t fixType = p[20];
    if (!(p[21] & 1u) || fixType < 2 || fixType > 4) {
        f.valid = false;
        return;
    }
    position(getI32(p+24),getI32(p+28),getI32(p+36),
             get32(p+40),nowMs,timeValid?epoch:0);
}
void navSat(const uint8_t*, uint16_t n) {
    // Counts were computed while payload streamed, even if >96 bytes.
    if (n < 8 || satVersion != 1 ||
        (uint32_t)n != 8u + (uint32_t)satNum*12u) return;
    ++satCount;
    satellites.view = satView;
    satellites.heard = satHeard;
    skyValid = true;
}
void navStatus(const uint8_t* p, uint16_t n) {
    if (n != 16) return;
    statusValid = (p[5] & 1u) && p[4] >= 2 && p[4] <= 4;
    if (!statusValid) f.valid = false;
}
void navPosLlh(const uint8_t* p, uint16_t n, uint32_t nowMs) {
    if (n != 28 || !statusValid) return;
    position(getI32(p+4),getI32(p+8),getI32(p+16),
             get32(p+20),nowMs,epoch);
}
void navTimeUtc(const uint8_t* p, uint16_t n) {
    if (n != 20 || !(p[19] & 4u)) return;
    setUtc(get16(p+12),p[14],p[15],p[16],p[17],p[18]);
}
void dispatch(uint32_t nowMs) {
    ++ok;
    seenCls=cls; seenId=id;
    if (cls != 0x01) return;
    // NAV-SAT is deliberately processed even when its payload exceeds the
    // small cache: its satellite counts were streamed during PAYLOAD.
    if (id == 0x35) { navSat(payload,len); return; }
    if (len > MAX_PAYLOAD) return;
    switch (id) {
        case 0x07: navPvt(payload,len,nowMs); break;
        case 0x03: navStatus(payload,len); break;
        case 0x02: navPosLlh(payload,len,nowMs); break;
        case 0x21: navTimeUtc(payload,len); break;
        default: break;
    }
}
}

bool feed(uint8_t b, uint32_t nowMs) {
    // Incomplete UBX frames must not monopolize the parser after a pause.
    if (state != IDLE && nowMs - lastByteMs > 2000u)
        state = IDLE;
    if (state != IDLE) lastByteMs = nowMs;
    switch (state) {
        case IDLE:
            if (b != 0xb5) return false;
            state = SYNC2; lastByteMs = nowMs; return true;
        case SYNC2:
            if (b == 0x62) { state = CLASS; sumA = sumB = 0; return true; }
            if (b == 0xb5) return true;
            state = IDLE; return false;
        case CLASS: cls=b; checksum(b); state=ID; return true;
        case ID: id=b; checksum(b); state=LEN0; return true;
        case LEN0: len=b; checksum(b); state=LEN1; return true;
        case LEN1:
            len |= (uint16_t)b << 8; checksum(b); pos=0;
            satVersion=satNum=satView=satHeard=0;
            // Invalid/oversize frames cannot consume the UART indefinitely.
            // NAV-SAT messages for all constellations comfortably fit 4096.
            if (len > 4096u) { ++fail; state=IDLE; return true; }
            state = len ? PAYLOAD : CKA; return true;
        case PAYLOAD:
            if (pos < MAX_PAYLOAD) payload[pos] = b;
            if (cls == 0x01 && id == 0x35) {
                if (pos == 4u) satVersion = b;
                else if (pos == 5u) satNum = b;
                else if (pos >= 8u && (pos - 8u) % 12u == 1u && b != 0) {
                    if (satView < 255u) ++satView;
                } else if (pos >= 8u && (pos - 8u) % 12u == 2u && b != 0) {
                    if (satHeard < 255u) ++satHeard;
                }
            }
            ++pos; checksum(b);
            if (pos >= len) state = CKA;
            return true;
        case CKA: expectedA=b; state=CKB; return true;
        case CKB:
            if (expectedA==sumA && b==sumB) dispatch(nowMs);
            else ++fail;
            state=IDLE; return true;
    }
    return false;
}
const Gnss::Fix& fix() { return f; }
Gnss::Sky sky() { return satellites; }
bool hasSky() { return skyValid; }
uint32_t utcEpoch() { return epoch; }
uint32_t good() { return ok; }
uint32_t bad() { return fail; }
uint32_t pvt() { return pvtCount; }
uint32_t sat() { return satCount; }
uint8_t lastClass() { return seenCls; }
uint8_t lastId() { return seenId; }
void reset() {
    state=IDLE; cls=id=sumA=sumB=expectedA=0; len=pos=0;
    lastByteMs=ok=fail=pvtCount=satCount=epoch=0;
    seenCls=seenId=0;
    satVersion=satNum=satView=satHeard=0;
    f=Gnss::Fix(); satellites=Gnss::Sky();
    skyValid=statusValid=false;
}
}
