// SquachWatch-CYD — the GNSS sentence parser, src/gnss.cpp.
//
// A position a few hundred metres out is not an error anybody would see on a
// watch: it would just put every network on WiGLE's map in the wrong street.
// So the coordinates are checked to the last digit the format carries, and
// the date arithmetic against dates worked out by hand.
#include "gnss.h"
#include "test_util.h"
#include <cstdio>
#include <cstring>
#include <vector>

// "$" + body + "*hh", with the checksum computed, as the module sends it.
static const char* nmea(const char* body) {
    static char buf[128];
    uint8_t sum = 0;
    for (const char* p = body; *p; p++) sum ^= (uint8_t)*p;
    snprintf(buf, sizeof buf, "$%s*%02X\r\n", body, sum);
    return buf;
}


// Deterministic UBX frames, with Fletcher checksum calculated independently
// of the production decoder. No hardware/Arduino dependency.
static void put16(std::vector<uint8_t>& p, size_t off, uint16_t v) {
    p[off] = (uint8_t)v; p[off+1] = (uint8_t)(v>>8);
}
static void put32(std::vector<uint8_t>& p, size_t off, uint32_t v) {
    for (unsigned i=0;i<4;i++) p[off+i] = (uint8_t)(v>>(i*8));
}
static std::vector<uint8_t> packet(uint8_t cls,uint8_t id,
                                   const std::vector<uint8_t>& p) {
    std::vector<uint8_t> f = {0xb5,0x62,cls,id,(uint8_t)p.size(),(uint8_t)(p.size()>>8)};
    f.insert(f.end(),p.begin(),p.end());
    uint8_t a=0,b=0;
    for (size_t i=2;i<f.size();i++) { a=(uint8_t)(a+f[i]); b=(uint8_t)(b+a); }
    f.push_back(a); f.push_back(b);
    return f;
}
static void uart(const std::vector<uint8_t>& bytes,uint32_t timeMs) {
    for (uint8_t b: bytes) Gnss::feed((char)b,timeMs);
}

int main() {
    suite("Checksums");
    Gnss::reset();
    ck("a good sentence is taken", Gnss::sentence(nmea("GNRMC,123519.00,A,4807.03800,N,01131.00000,E,0.0,0.0,280926,,,A"), 0));
    ck("a flipped character is refused", !Gnss::sentence("$GNRMC,123519.00,A,4807.03800,N,01131.00000,E,0.0,0.0,280926,,,A*00", 0));
    ck("no checksum at all is refused", !Gnss::sentence("$GNGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,", 0));
    ck("counted", Gnss::good() == 1 && Gnss::bad() == 2);

    suite("A fix: GGA after RMC");
    Gnss::reset();
    Gnss::sentence(nmea("GNRMC,123519.00,A,4807.03800,N,01131.00000,E,0.0,0.0,280926,,,A"), 1000);
    ck("RMC gives UTC", Gnss::utcEpoch() == Gnss::toEpoch(2026, 9, 28, 12, 35, 19));
    ck("no fix from RMC alone", !Gnss::fix().valid);
    Gnss::sentence(nmea("GNGGA,123520.00,4807.03800,N,01131.00000,E,1,08,0.9,545.4,M,46.9,M,,"), 2000);
    const Gnss::Fix& f = Gnss::fix();
    ck("valid", f.valid);
    // 48 deg 07.038 min = 48.1173 deg; 11 deg 31.000 min = 11.5166666 deg.
    ck("latitude to the 7th place", f.lat7 == 481173000);
    ck("longitude to the 7th place", f.lon7 == 115166666);
    ck("altitude", f.altM == 545);
    ck("used", f.used == 8);
    ck("accuracy is HDOP x 4, rounded", f.accM == 4);
    ck("the fix is dated by the RMC and timed by the GGA", f.epoch == Gnss::toEpoch(2026, 9, 28, 12, 35, 20));
    ck("fresh at once", Gnss::fresh(2000 + 100));
    ck("stale after FRESH_MS", !Gnss::fresh(2000 + Gnss::FRESH_MS + 1));

    suite("Hemispheres");
    Gnss::sentence(nmea("GNGGA,000000.00,3351.00000,S,15112.00000,W,1,05,1.2,10.0,M,0.0,M,,"), 3000);
    ck("south is negative", Gnss::fix().lat7 == -338500000);
    ck("west is negative", Gnss::fix().lon7 == -1512000000);
    Gnss::sentence(nmea("GNGGA,000000.00,0000.00060,S,00000.00060,W,1,05,1.2,10.0,M,0.0,M,,"), 3000);
    ck("a tiny south latitude keeps its sign", Gnss::fix().lat7 == -100);

    suite("No fix");
    Gnss::sentence(nmea("GNGGA,000001.00,,,,,0,03,99.9,,,,,,"), 4000);
    ck("quality 0 is no fix", !Gnss::fix().valid);
    ck("but the count of satellites used still updates", Gnss::fix().used == 3);
    Gnss::sentence(nmea("GNRMC,000002.00,V,,,,,,,280926,,,N"), 4000);
    ck("a void RMC does not move the clock", Gnss::utcEpoch() == Gnss::toEpoch(2026, 9, 28, 12, 35, 19));

    suite("Satellites in view and heard (GSV)");
    Gnss::reset();
    // GPS: 6 in view over two messages, four with a signal.
    Gnss::sentence(nmea("GPGSV,2,1,06,01,40,083,46,02,17,308,,03,07,344,39,04,22,228,42"), 0);
    Gnss::sentence(nmea("GPGSV,2,2,06,05,50,100,,06,10,200,33"), 0);
    // Galileo: 2 in view, none heard.
    Gnss::sentence(nmea("GAGSV,1,1,02,11,30,090,,12,20,180,"), 0);
    Gnss::Sky k = Gnss::sky();
    ck("in view, all constellations", k.view == 8);
    ck("heard: only the ones with an SNR", k.heard == 4);
    // A new cycle replaces the old one rather than adding to it.
    Gnss::sentence(nmea("GPGSV,1,1,01,01,40,083,40"), 0);
    k = Gnss::sky();
    ck("a new GPS cycle replaces the last", k.view == 3 && k.heard == 1);

    suite("A character at a time, as the UART delivers it");
    Gnss::reset();
    const char* a = nmea("GNRMC,010203.00,A,5130.00000,N,00007.00000,W,0.0,0.0,280926,,,A");
    for (const char* p = a; *p; p++) Gnss::feed(*p, 0);
    const char* b = nmea("GNGGA,010204.00,5130.00000,N,00007.00000,W,1,09,0.7,20.0,M,45.0,M,,");
    for (const char* p = b; *p; p++) Gnss::feed(*p, 5000);
    ck("fixed", Gnss::fix().valid && Gnss::fix().lat7 == 515000000 && Gnss::fix().lon7 == -1166666);
    ck("2026-09-28 01:02:04 UTC", Gnss::fix().epoch == 1790557324u);

    suite("Dates");
    ck("the Unix epoch", Gnss::toEpoch(1970, 1, 1, 0, 0, 0) == 0);
    ck("2000-03-01 (after a leap day)", Gnss::toEpoch(2000, 3, 1, 0, 0, 0) == 951868800u);
    ck("2026-09-28 00:00", Gnss::toEpoch(2026, 9, 28, 0, 0, 0) == 1790553600u);

    suite("A bench fix");
    Gnss::reset();
    Gnss::fake(407128000, -740060000, 1790553600u, 100);
    ck("fake is a fix", Gnss::fix().valid && Gnss::faked());
    Gnss::sentence(nmea("GNGGA,000000.00,4042.76800,N,07400.36000,W,1,06,1.0,5.0,M,0.0,M,,"), 200);
    ck("a real fix ends the fake", !Gnss::faked() && Gnss::fix().valid);


    suite("UBX NAV-PVT: binary-only valid position, time and accuracy");
    Gnss::reset();
    std::vector<uint8_t> pvt(92,0);
    put16(pvt,4,2026); pvt[6]=11; pvt[7]=2;
    pvt[8]=18; pvt[9]=31; pvt[10]=45;
    pvt[11]=3; // UTC date and time valid
    pvt[20]=3; pvt[21]=1; pvt[23]=11; // 3D GPS fix, gpsFixOk, 11 SV used
    put32(pvt,24,(uint32_t)(int32_t)-837654321);
    put32(pvt,28,(uint32_t)401234567);
    put32(pvt,36,(uint32_t)(int32_t)-12500); // -12 m MSL
    put32(pvt,40,2300); // 2.3 m hAcc
    uart(packet(0x01,0x07,pvt),8000);
    ck("UBX checksum accepted", Gnss::ubxGood()==1 && Gnss::ubxBad()==0);
    ck("valid fix from UBX without NMEA", Gnss::fix().valid && Gnss::fresh(8001));
    ck("signed UBX coordinates", Gnss::fix().lat7==401234567 &&
                                  Gnss::fix().lon7==-837654321);
    ck("UBX MSL height and accuracy", Gnss::fix().altM==-12 &&
                                      Gnss::fix().accM==2);
    ck("UBX satellites used", Gnss::fix().used==11);
    ck("UBX valid date/time", Gnss::fix().epoch==
                                Gnss::toEpoch(2026,11,2,18,31,45));
    ck("UBX PVT counter", Gnss::ubxPvt()==1);

    suite("UBX NAV-SAT visibility and signal");
    std::vector<uint8_t> sat(32,0);
    sat[4]=1; sat[5]=2; // version, numSvs
    sat[9]=1; sat[10]=42; // svId 1, cno=42
    sat[21]=2; sat[22]=0; // svId 2, cno=0
    uart(packet(0x01,0x35,sat),8100);
    ck("NAV-SAT counted", Gnss::ubxSat()==1);
    ck("UBX satellites in view and heard", Gnss::sky().view==2 &&
                                            Gnss::sky().heard==1);

    suite("Large UBX NAV-SAT frame streaming without a 1536-byte buffer");
    std::vector<uint8_t> lotsOfSat(8 + 40*12, 0);
    lotsOfSat[4]=1; lotsOfSat[5]=40;
    for (unsigned i=0; i<40; ++i) {
        lotsOfSat[8+12*i+1] = (uint8_t)(i+1);  // svId
        lotsOfSat[8+12*i+2] = (i%2 == 0) ? 40 : 0; // cno
    }
    uart(packet(0x01,0x35,lotsOfSat),8120);
    ck("large NAV-SAT frame accepted with full checksum", Gnss::ubxSat()==2);
    ck("40 viewed and 20 heard from streamed NAV-SAT", Gnss::sky().view==40 &&
                                                       Gnss::sky().heard==20);


    suite("UBX corrupted checksum cannot corrupt fix");
    auto badUbx = packet(0x01,0x07,pvt);
    badUbx.back() ^= 0x40;
    uart(badUbx,8200);
    ck("UBX bad checksum counted", Gnss::ubxBad()==1);
    ck("valid earlier position preserved", Gnss::fix().lat7==401234567);

    suite("NMEA after binary and unknown binary containing newline and dollar");
    uart(packet(0x0a,0x09,std::vector<uint8_t>{0x24,0x0a,0xb5,0x62}),8300);
    ck("unknown UBX frame accepted, payload not fed to NMEA",
       Gnss::ubxGood()==3 && Gnss::bad()==0);
    const char* rmcMix=nmea("GNRMC,123519.00,A,4807.03800,N,01131.00000,E,0.0,0.0,021126,,,A");
    for (const char* p=rmcMix;*p;p++) Gnss::feed(*p,9000);
    const char* ggaMix=nmea("GNGGA,123520.00,4807.03800,N,01131.00000,E,1,08,0.9,545.4,M,46.9,M,,");
    for (const char* p=ggaMix;*p;p++) Gnss::feed(*p,9100);
    ck("NMEA also accepted in mixed protocol stream", Gnss::good()==2);
    ck("newest valid position chosen", Gnss::fix().lat7==481173000 &&
                                       Gnss::fix().lon7==115166666);
    ck("NMEA time still valid", Gnss::fix().epoch==
                                Gnss::toEpoch(2026,11,2,12,35,20));

    suite("UBX bad PVT quality does not claim a fix on fresh reset");
    Gnss::reset();
    pvt[21]=0; // no gpsFixOk
    uart(packet(0x01,0x07,pvt),10000);
    ck("UBX quality reported as no fix", !Gnss::fix().valid);
    ck("UBX frame itself accepted", Gnss::ubxGood()==1);

    return report();
}
