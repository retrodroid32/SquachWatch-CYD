// SquachWatch-CYD PC emulator — stand-in DetectionEngine + SdLog.
//
// This replaces src/detection.cpp and src/sd_log.cpp in the sim build.
// The real ones are ~900 lines wired directly into WiFi.h, esp_wifi.h,
// NimBLEDevice.h, esp_bt.h and SD.h; faithfully stubbing that whole
// surface would be a large side quest on its own, and none of it
// affects what the emulator exists to show -- how the UI *renders*.
//
// So: same class from the same header (every ui_*.cpp still takes the
// real `const DetectionEngine&`, unchanged), backed here by plain
// in-memory data with no radios behind it. Scanning, signature
// matching, watch/hunt alerting and SD logging are all inert. What the
// screens read back -- log entries, per-type counts, lifetime totals,
// watch/hunt labels and RSSI history -- is whatever seed() puts there.
//
// The trade this makes: the emulator will faithfully show you a LOG
// screen full of detections, but it is NOT exercising the real matching
// logic that decides what counts as a detection in the first place.
#include "detection.h"
#include <cstring>

// ---- SdLog: no card, ever -------------------------------------------
bool SdLog::begin() { _ready = false; return false; }
void SdLog::logEvent(const Detection&) {}
void SdLog::tick() {}
void SdLog::wipe() {}
void SdLog::openDaily() {}

// ---- DetectionEngine -------------------------------------------------
#if SQUACH_MESH
// The emulator has no radio, so the probe has nothing to count. Stubs keep
// the diagnostics screen renderable there without an #if around every row.
namespace MeshProbe {
    void  begin() {}
    void  tick(uint32_t) {}
    void  noteAdvert() {}
    Stats stats() { return Stats{}; }
    bool  concluded() { return false; }
}

// Mesh itself is no longer stubbed: src/mesh.cpp is compiled as-is, and
// sim/meshsim.cpp stands in for the radio underneath it.
#endif

bool DetectionEngine::init() { return true; }
void DetectionEngine::loop() {}
// No radio, so nothing to restart and nothing freed.
ScanFlushStats scanFlushStats() { return ScanFlushStats{ 0, 0, 0 }; }
BootHeap bootHeap()             { return BootHeap{ 0, 0, 0, 0 }; }
bool     scanPassiveNow()       { return false; }
uint32_t advertsDropped()       { return 0; }
uint32_t advertRate()           { return 0; }
uint32_t wifiFramesSeen()       { return 0; }
uint32_t advertsSeen()          { return 0; }
static const volatile uint32_t s_kinds0[5] = { 0, 0, 0, 0, 0 };
const volatile uint32_t* advertKinds() { return s_kinds0; }
void     setScanWindow(uint8_t) {}
void     setScanWindowBase(uint8_t) {}
void     setScanBoost(bool) {}
void     setScanInterval(uint16_t, uint8_t) {}
void     setScanPin(uint8_t)    {}

void DetectionEngine::clearLog() {
    _logCount = 0;
    _logHead = 0;
    _latest = nullptr;
    memset(_typeCounts, 0, sizeof(_typeCounts));
}

const Detection* DetectionEngine::logAt(uint8_t idx) const {
    if (idx >= _logCount) return nullptr;
    // Newest first, matching the real ring-buffer walk order the LOG
    // screen's scrolling assumes.
    uint8_t slot = (uint8_t)((_logHead + LOG_CAP - 1 - idx) % LOG_CAP);
    return &_log[slot];
}

const Detection* DetectionEngine::findDetection(const uint8_t* mac, DetectionType type) const {
    if (!mac) return nullptr;
    for (uint8_t i = 0; i < _logCount; i++) {
        const Detection* d = logAt(i);
        if (d && d->type == type && memcmp(d->mac, mac, 6) == 0) return d;
    }
    return nullptr;
}

bool DetectionEngine::alertCooldownReady(const uint8_t* mac, DetectionType type,
                                         uint16_t cooldownSec, uint32_t now) const {
    if (!cooldownSec) return true;
    const Detection* d = findDetection(mac, type);
    if (!d) return true;
    return !d->lastAlertMs ||
           (uint32_t)(now - d->lastAlertMs) >= (uint32_t)cooldownSec * 1000u;
}

void DetectionEngine::noteAlertRaised(const uint8_t* mac, DetectionType type, uint32_t now) {
    if (!mac) return;
    for (uint8_t i = 0; i < _logCount; i++) {
        const uint8_t slot = (uint8_t)((_logHead + LOG_CAP - 1 - i) % LOG_CAP);
        Detection& d = _log[slot];
        if (d.type == type && memcmp(d.mac, mac, 6) == 0) {
            d.lastAlertMs = now;
            return;
        }
    }
}

void DetectionEngine::resetLifetime() {
    _lifetimeTotal = 0;
    memset(_typeCounts, 0, sizeof(_typeCounts));
}

void DetectionEngine::pushLog(const Detection& d) {
    _log[_logHead] = d;
    _latest = &_log[_logHead];
    _logHead = (uint8_t)((_logHead + 1) % LOG_CAP);
    if (_logCount < LOG_CAP) _logCount++;
    if ((uint8_t)d.type < (uint8_t)DetectionType::COUNT) {
        _typeCounts[(uint8_t)d.type]++;
        _lifetimeByType[(uint8_t)d.type]++;   // the DEX and the outfits read this
    }
    _lifetimeTotal++;
}

// Radio-fed entry points: inert here, nothing calls them in the sim.
void DetectionEngine::postWiFi(const uint8_t*, int8_t, uint8_t, const char*, bool, bool, bool, bool) {}
void DetectionEngine::postDeauth(const uint8_t*, int8_t, uint8_t) {}
void DetectionEngine::postBle(Detection d) { pushLog(d); }

// Not a stub. Everything else in this file is inert because it would need
// a radio, but the Remote ID decoder is pure arithmetic over a byte
// buffer -- so the emulator runs the REAL one, and the drone info panel it
// renders is showing genuinely decoded values rather than a mock-up.
void DetectionEngine::mergeRemoteId(const uint8_t* mac, const uint8_t* payload,
                                    uint8_t len) {
    if (!mac || !payload) return;
    if (memcmp(mac, _ridMac, 6) != 0) {
        RemoteId::reset(_rid);
        memcpy(_ridMac, mac, 6);
    }
    RemoteId::merge(payload, len, _rid, millis());
}
void DetectionEngine::postBtClassic(Detection d) { pushLog(d); }
void DetectionEngine::postRawBle(RawBleResult r) {
    if (_rawBleCount < RAW_BLE_CAP) _rawBle[_rawBleCount++] = r;
}

// Raw scanner: reports "done, nothing found" so the raw-scan screen
// renders its empty-result state rather than spinning forever.
void DetectionEngine::startRawBleScan() { _rawBleCount = 0; }
bool DetectionEngine::rawBleScanDone() const { return true; }
const RawBleResult* DetectionEngine::rawBleAt(uint8_t idx) const {
    return idx < _rawBleCount ? &_rawBle[idx] : nullptr;
}
void DetectionEngine::startRawWifiScan() {}
bool DetectionEngine::rawWifiScanDone() const { return true; }
uint8_t DetectionEngine::rawWifiCount() const { return 0; }
const char* DetectionEngine::rawWifiSsid(uint8_t) const { return ""; }
int8_t DetectionEngine::rawWifiRssi(uint8_t) const { return 0; }
uint8_t DetectionEngine::rawWifiChannel(uint8_t) const { return 0; }
bool DetectionEngine::rawWifiOpen(uint8_t) const { return false; }
const uint8_t* DetectionEngine::rawWifiBssid(uint8_t) const { return nullptr; }
void DetectionEngine::stopRawScan() {}
void DetectionEngine::startUpdateRadio() {}
void DetectionEngine::stopUpdateRadio() {}

// ---- watch / hunt ----------------------------------------------------
int8_t DetectionEngine::findWatchTarget(const uint8_t* mac, WatchKind kind) const {
    if (!mac || kind == WatchKind::NONE) return -1;
    for (uint8_t i = 0; i < _watchTargetCount; i++)
        if (_watchTargets[i].kind == kind && memcmp(_watchTargets[i].mac, mac, 6) == 0) return (int8_t)i;
    return -1;
}
int8_t DetectionEngine::watchEntryRssiAt(const WatchEntry& e, uint8_t idx) const {
    if (idx >= e.rssiCount) return 0;
    const uint8_t first = e.rssiCount < WATCH_RSSI_CAP ? 0 : e.rssiHead;
    return e.rssiHist[(uint8_t)((first + idx) % WATCH_RSSI_CAP)];
}
DetectionEngine::WatchToggle DetectionEngine::toggleWatch(const uint8_t* mac, const char* label, WatchKind kind) {
    const int8_t found = findWatchTarget(mac, kind);
    if (found >= 0) {
        const bool active = _watchKind == kind && memcmp(_watchMac, mac, 6) == 0;
        for (uint8_t i=(uint8_t)found; i+1<_watchTargetCount; i++) _watchTargets[i]=_watchTargets[i+1];
        if (_watchTargetCount) { _watchTargetCount--; _watchTargets[_watchTargetCount] = WatchEntry(); }
        if (active) deactivateWatch();
        return WatchToggle::REMOVED;
    }
    if (_watchTargetCount >= WATCH_TARGET_CAP) return WatchToggle::FULL;
    WatchEntry& e=_watchTargets[_watchTargetCount++]; e=WatchEntry(); e.kind=kind;
    if (mac) memcpy(e.mac,mac,6);
    snprintf(e.label,sizeof e.label,"%s",(label&&label[0])?label:(kind==WatchKind::BLE?"Unnamed device":"(hidden)"));
    return WatchToggle::ADDED;
}
DetectionEngine::WatchToggle DetectionEngine::toggleWatchBle(const uint8_t* mac,const char* name){ return toggleWatch(mac,name,WatchKind::BLE); }
DetectionEngine::WatchToggle DetectionEngine::toggleWatchWifi(const uint8_t* mac,const char* name){ return toggleWatch(mac,name,WatchKind::WIFI); }
bool DetectionEngine::isWatched(const uint8_t* mac,bool ble) const { return findWatchTarget(mac,ble?WatchKind::BLE:WatchKind::WIFI)>=0; }
bool DetectionEngine::watchTargetInfo(uint8_t idx, WatchTargetInfo& out) const {
    if(idx>=_watchTargetCount) return false; const WatchEntry& e=_watchTargets[idx]; out=WatchTargetInfo();
    out.kind=e.kind; memcpy(out.mac,e.mac,6); snprintf(out.label,sizeof out.label,"%s",e.label);
    out.samples=e.rssiCount; out.lastSeenMs=e.lastSeenMs; out.seen=e.rssiCount>0;
    if(e.rssiCount){ out.rssi=watchEntryRssiAt(e,e.rssiCount-1); out.previousRssi=e.rssiCount>=2?watchEntryRssiAt(e,e.rssiCount-2):out.rssi;
      out.trend=classifyRssiTrend((int)watchEntryRssiAt(e,0),(int)out.rssi,e.rssiCount); }
    return true;
}
bool DetectionEngine::activateWatchTarget(uint8_t idx){ if(idx>=_watchTargetCount)return false; const WatchEntry&e=_watchTargets[idx]; _watchKind=e.kind; memcpy(_watchMac,e.mac,6); snprintf(_watchLabel,sizeof _watchLabel,"%s",e.label); return true; }
void DetectionEngine::deactivateWatch(){ _watchKind=WatchKind::NONE; memset(_watchMac,0,sizeof _watchMac); _watchLabel[0]=0; }
void DetectionEngine::clearWatch(){ if(_watchKind==WatchKind::NONE)return; int8_t f=findWatchTarget(_watchMac,_watchKind); if(f>=0){ for(uint8_t i=(uint8_t)f;i+1<_watchTargetCount;i++)_watchTargets[i]=_watchTargets[i+1]; _watchTargetCount--; _watchTargets[_watchTargetCount] = WatchEntry(); } deactivateWatch(); }
void DetectionEngine::clearWatches(){ for(uint8_t i=0;i<WATCH_TARGET_CAP;i++)_watchTargets[i]=WatchEntry(); _watchTargetCount=0; deactivateWatch(); }
void DetectionEngine::watchBle(const uint8_t* mac,const char* name){ int8_t i=findWatchTarget(mac,WatchKind::BLE); if(i<0){ if(toggleWatchBle(mac,name)!=WatchToggle::ADDED)return; i=findWatchTarget(mac,WatchKind::BLE);} if(i>=0)activateWatchTarget((uint8_t)i); }
void DetectionEngine::watchWifi(const uint8_t* mac,const char* name){ int8_t i=findWatchTarget(mac,WatchKind::WIFI); if(i<0){ if(toggleWatchWifi(mac,name)!=WatchToggle::ADDED)return; i=findWatchTarget(mac,WatchKind::WIFI);} if(i>=0)activateWatchTarget((uint8_t)i); }
void DetectionEngine::noteWatchRssi(WatchEntry& e,int8_t rssi){ e.lastSeenMs=millis(); e.rssiHist[e.rssiHead]=rssi; e.rssiHead=(uint8_t)((e.rssiHead+1)%WATCH_RSSI_CAP); if(e.rssiCount<WATCH_RSSI_CAP)e.rssiCount++; }
void DetectionEngine::checkWatchBle(const uint8_t* mac,int8_t rssi){ for(uint8_t i=0;i<_watchTargetCount;i++){ WatchEntry&e=_watchTargets[i]; if(e.kind==WatchKind::BLE&&memcmp(e.mac,mac,6)==0){ noteWatchRssi(e,rssi); e.pending=true; return; } } }
void DetectionEngine::checkWatchWifi(const uint8_t* mac,int8_t rssi){ for(uint8_t i=0;i<_watchTargetCount;i++){ WatchEntry&e=_watchTargets[i]; if(e.kind==WatchKind::WIFI&&memcmp(e.mac,mac,6)==0){ noteWatchRssi(e,rssi); e.pending=true; return; } } }
bool DetectionEngine::watchHitPending(){ for(uint8_t i=0;i<_watchTargetCount;i++) if(_watchTargets[i].pending){ _watchTargets[i].pending=false; return activateWatchTarget(i); } return false; }
uint8_t DetectionEngine::watchRssiCount() const { int8_t f=findWatchTarget(_watchMac,_watchKind); return f>=0?_watchTargets[(uint8_t)f].rssiCount:0; }
int8_t DetectionEngine::watchRssiAt(uint8_t idx) const { int8_t f=findWatchTarget(_watchMac,_watchKind); return f>=0?watchEntryRssiAt(_watchTargets[(uint8_t)f],idx):0; }

int8_t DetectionEngine::findHuntTarget(const uint8_t* mac, WatchKind kind) const {
    if (!mac || kind == WatchKind::NONE) return -1;
    for (uint8_t i = 0; i < _huntTargetCount; i++) {
        if (_huntTargets[i].kind == kind && memcmp(_huntTargets[i].mac, mac, 6) == 0)
            return (int8_t)i;
    }
    return -1;
}

int8_t DetectionEngine::huntEntryRssiAt(const HuntEntry& e, uint8_t idx) const {
    if (idx >= e.rssiCount) return 0;
    const uint8_t first = (e.rssiCount < HUNT_RSSI_CAP) ? 0 : e.rssiHead;
    return e.rssiHist[(uint8_t)((first + idx) % HUNT_RSSI_CAP)];
}

DetectionEngine::HuntToggle DetectionEngine::toggleHunt(const uint8_t* mac,
                                                         const char* label,
                                                         WatchKind kind) {
    const int8_t found = findHuntTarget(mac, kind);
    if (found >= 0) {
        const bool wasActive = (_huntKind == kind && memcmp(_huntMac, mac, 6) == 0);
        for (uint8_t i = (uint8_t)found; i + 1 < _huntTargetCount; i++)
            _huntTargets[i] = _huntTargets[i + 1];
        if (_huntTargetCount) {
            _huntTargetCount--;
            memset(&_huntTargets[_huntTargetCount], 0, sizeof(_huntTargets[0]));
        }
        if (wasActive) deactivateHunt();
        return HuntToggle::REMOVED;
    }

    if (_huntTargetCount >= HUNT_TARGET_CAP) return HuntToggle::FULL;
    HuntEntry& e = _huntTargets[_huntTargetCount++];
    memset(&e, 0, sizeof e);
    e.kind = kind;
    if (mac) memcpy(e.mac, mac, 6);
    const char* fallback = (kind == WatchKind::BLE) ? "Unnamed device" : "(hidden)";
    snprintf(e.label, sizeof e.label, "%s", (label && label[0]) ? label : fallback);
    return HuntToggle::ADDED;
}

DetectionEngine::HuntToggle DetectionEngine::toggleHuntBle(const uint8_t* mac, const char* name) {
    return toggleHunt(mac, name, WatchKind::BLE);
}

DetectionEngine::HuntToggle DetectionEngine::toggleHuntWifi(const uint8_t* bssid, const char* ssid) {
    return toggleHunt(bssid, ssid, WatchKind::WIFI);
}

bool DetectionEngine::isHunted(const uint8_t* mac, bool ble) const {
    return findHuntTarget(mac, ble ? WatchKind::BLE : WatchKind::WIFI) >= 0;
}

bool DetectionEngine::huntTargetInfo(uint8_t idx, HuntTargetInfo& out) const {
    if (idx >= _huntTargetCount) return false;
    const HuntEntry& e = _huntTargets[idx];
    out = HuntTargetInfo();
    out.kind = e.kind;
    memcpy(out.mac, e.mac, 6);
    snprintf(out.label, sizeof out.label, "%s", e.label);
    out.samples = e.rssiCount;
    out.lastSeenMs = e.lastSeenMs;
    out.seen = e.rssiCount > 0;
    if (e.rssiCount) {
        out.rssi = huntEntryRssiAt(e, (uint8_t)(e.rssiCount - 1));
        out.previousRssi = e.rssiCount >= 2
            ? huntEntryRssiAt(e, (uint8_t)(e.rssiCount - 2))
            : out.rssi;
        out.trend = classifyRssiTrend((int)huntEntryRssiAt(e, 0),
                                     (int)out.rssi, e.rssiCount);
    }
    return true;
}

bool DetectionEngine::activateHuntTarget(uint8_t idx) {
    if (idx >= _huntTargetCount) return false;
    const HuntEntry& e = _huntTargets[idx];
    _huntKind = e.kind;
    memcpy(_huntMac, e.mac, 6);
    snprintf(_huntLabel, sizeof _huntLabel, "%s", e.label);
    return true;
}

void DetectionEngine::deactivateHunt() {
    _huntKind = WatchKind::NONE;
    memset(_huntMac, 0, sizeof _huntMac);
    _huntLabel[0] = 0;
}

void DetectionEngine::clearHunt() {
    if (_huntKind == WatchKind::NONE) return;
    const int8_t found = findHuntTarget(_huntMac, _huntKind);
    if (found >= 0) {
        for (uint8_t i = (uint8_t)found; i + 1 < _huntTargetCount; i++)
            _huntTargets[i] = _huntTargets[i + 1];
        _huntTargetCount--;
        memset(&_huntTargets[_huntTargetCount], 0, sizeof(_huntTargets[0]));
    }
    deactivateHunt();
}

void DetectionEngine::clearHunts() {
    memset(_huntTargets, 0, sizeof _huntTargets);
    _huntTargetCount = 0;
    deactivateHunt();
}

bool DetectionEngine::huntBle(const uint8_t* mac, const char* name) {
    int8_t idx = findHuntTarget(mac, WatchKind::BLE);
    if (idx < 0) {
        if (toggleHunt(mac, name, WatchKind::BLE) != HuntToggle::ADDED) return false;
        idx = findHuntTarget(mac, WatchKind::BLE);
    }
    return idx >= 0 && activateHuntTarget((uint8_t)idx);
}

bool DetectionEngine::huntWifi(const uint8_t* bssid, const char* ssid) {
    int8_t idx = findHuntTarget(bssid, WatchKind::WIFI);
    if (idx < 0) {
        if (toggleHunt(bssid, ssid, WatchKind::WIFI) != HuntToggle::ADDED) return false;
        idx = findHuntTarget(bssid, WatchKind::WIFI);
    }
    return idx >= 0 && activateHuntTarget((uint8_t)idx);
}

void DetectionEngine::noteHuntRssi(HuntEntry& e, int8_t rssi) {
    const uint32_t now = millis();
    e.lastSeenMs = now;
    if (e.rssiCount && now - e.rssiLastMs < WATCH_RSSI_SAMPLE_MS) return;
    e.rssiLastMs = now;
    e.rssiHist[e.rssiHead] = rssi;
    e.rssiHead = (uint8_t)((e.rssiHead + 1) % HUNT_RSSI_CAP);
    if (e.rssiCount < HUNT_RSSI_CAP) e.rssiCount++;
}

void DetectionEngine::checkHuntBle(const uint8_t* mac, int8_t rssi) {
    for (uint8_t i = 0; i < _huntTargetCount; i++) {
        HuntEntry& e = _huntTargets[i];
        if (e.kind == WatchKind::BLE && memcmp(e.mac, mac, 6) == 0) {
            noteHuntRssi(e, rssi);
            return;
        }
    }
}

void DetectionEngine::checkHuntWifi(const uint8_t* mac, int8_t rssi) {
    for (uint8_t i = 0; i < _huntTargetCount; i++) {
        HuntEntry& e = _huntTargets[i];
        if (e.kind == WatchKind::WIFI && memcmp(e.mac, mac, 6) == 0) {
            noteHuntRssi(e, rssi);
            return;
        }
    }
}

uint8_t DetectionEngine::huntRssiCount() const {
    const int8_t found = findHuntTarget(_huntMac, _huntKind);
    return found >= 0 ? _huntTargets[(uint8_t)found].rssiCount : 0;
}

int8_t DetectionEngine::huntRssiAt(uint8_t idx) const {
    const int8_t found = findHuntTarget(_huntMac, _huntKind);
    return found >= 0 ? huntEntryRssiAt(_huntTargets[(uint8_t)found], idx) : 0;
}

// ---- housekeeping the real engine runs from loop() -------------------
void DetectionEngine::processWiFiQ(uint32_t, uint8_t) {}
void DetectionEngine::processDeauthQ() {}
void DetectionEngine::expireStale() {}
void DetectionEngine::hopChannel() {}
void DetectionEngine::decayChannelActivity() {}
