// SquachWatch-CYD — shared state types
// Standalone header; only <stdint.h> dependency.
#pragma once
#include <stdint.h>

enum class DetectionType : uint8_t {
    UNKNOWN = 0,
    FLOCK   = 1,   // Flock Safety camera / sensor
    AXON    = 2,   // Axon body camera / LE equipment
    META    = 3,   // Ray-Ban Meta smart glasses
    SKIMMER = 4,   // HC-05/06/03 Bluetooth skimmer
    RAVEN   = 5,   // Raven gunshot detector
    AIRTAG  = 6,   // Apple AirTag / FindMy
    DRONE   = 7,   // OpenDroneID drone
    ALPR    = 8,   // Motorola / Vigilant ALPR
    CAMERA  = 9,   // Generic camera (existing OUI list)
    SAMSUNG_TAG = 10,  // Samsung Galaxy SmartTag / SmartTag+
    GOOGLE_TAG  = 11,  // Google Find My Device network tracker (Chipolo/Pebblebee/Moto Tag)
    TILE    = 12,  // Tile BLE tracker (was previously bucketed under AIRTAG)
    RING    = 13,  // Ring doorbell/camera (was previously bucketed under CAMERA)
    DEAUTH  = 14,  // WiFi deauth/disassoc flood -- rate-detected, not a signature match (see DetectionEngine)
    EVILTWIN = 15, // One SSID beaconing from a second BSSID whose OUI differs -- rogue/spoofed AP (see DetectionEngine)
    IBEACON = 16,  // Apple iBeacon proximity beacon -- retail/venue tracking, not police kit
    // Pentest and wireless-audit hardware: Flipper Zero, Pwnagotchi, WiFi
    // Pineapple, ESP deauthers. One bucket rather than four types because
    // what matters to somebody reading the screen is that a tool for
    // attacking radios is in the room, not which model it is -- the
    // specific device goes in the vendor label and, where it announces
    // one, its own name goes in the name field.
    //
    // Deliberately NOT in this bucket: bare Espressif and other generic
    // silicon. A nyanBOX, an ESP32 Marauder and a SquachWatch are the same
    // chip, and sixteen Espressif prefixes already sit under FLOCK. HACKER
    // takes exact signatures only, which is what keeps it worth alerting on.
    HACKER  = 17,
    COUNT   = 18
};
static_assert((uint8_t)DetectionType::COUNT <= 32,
              "Detection type mask is uint32_t; widen it before adding more types");

inline const char* detectionTypeDisplayName(DetectionType t) {
    switch (t) {
        case DetectionType::FLOCK:       return "FLOCK CAM";
        case DetectionType::AXON:        return "AXON BODY";
        case DetectionType::META:        return "META GLASSES";
        case DetectionType::SKIMMER:     return "CARD SKIMMER";
        case DetectionType::RAVEN:       return "RAVEN";
        case DetectionType::AIRTAG:      return "AIRTAG";
        case DetectionType::DRONE:       return "DRONE";
        case DetectionType::ALPR:        return "ALPR";
        case DetectionType::CAMERA:      return "CAMERA";
        case DetectionType::SAMSUNG_TAG: return "SAMSUNG TAG";
        case DetectionType::GOOGLE_TAG:  return "GOOGLE TAG";
        case DetectionType::TILE:        return "TILE";
        case DetectionType::RING:        return "RING CAM";
        case DetectionType::DEAUTH:      return "DEAUTH";
        case DetectionType::EVILTWIN:    return "EVIL TWIN AP";
        case DetectionType::IBEACON:     return "PROXIMITY BEACON";
        case DetectionType::HACKER:      return "HACKER HARDWARE";
        default:                         return "UNKNOWN";
    }
}

inline const char* detectionTypeName(DetectionType t) {
    switch (t) {
        case DetectionType::FLOCK:       return "FLOCK";
        case DetectionType::AXON:        return "AXON";
        case DetectionType::META:        return "META";
        case DetectionType::SKIMMER:     return "SKIMMER";
        case DetectionType::RAVEN:       return "RAVEN";
        case DetectionType::AIRTAG:      return "AIRTAG";
        case DetectionType::DRONE:       return "DRONE";
        case DetectionType::ALPR:        return "ALPR";
        case DetectionType::CAMERA:      return "CAMERA";
        case DetectionType::SAMSUNG_TAG: return "SAMSUNG_TAG";
        case DetectionType::GOOGLE_TAG:  return "GOOGLE_TAG";
        case DetectionType::TILE:        return "TILE";
        case DetectionType::RING:        return "RING";
        case DetectionType::DEAUTH:      return "DEAUTH";
        case DetectionType::EVILTWIN:    return "EVIL TWIN";
        case DetectionType::IBEACON:     return "IBEACON";
        case DetectionType::HACKER:      return "HACKER";
        default:                         return "UNKNOWN";
    }
}

// How sure we are that a match is what it claims to be.
//
// Moved here from signatures.h because it is now a property of the
// SIGHTING rather than of the type. A FLOCK hit off Flock Safety's own
// registered OUI and a FLOCK hit off a generic Espressif module block are
// the same DetectionType and are not remotely the same claim.
//
// Note: plain LOW/MEDIUM/HIGH collide with the Arduino core's pin-state
// macros through the preprocessor, which enum class scoping does not
// protect against -- hence the _CONF suffix.
enum class Confidence : uint8_t { LOW_CONF, MED_CONF, HIGH_CONF };


// Compact description of the concrete radio evidence that produced a
// detection. Numeric details such as BLE company/service IDs live in
// Detection::evidenceCode; Wi-Fi OUI evidence is already in mac[0..2].
enum class EvidenceKind : uint8_t {
    UNKNOWN = 0,
    BLE_MFG,
    BLE_UUID,
    BLE_NAME,
    BLE_FINDMY,
    BLE_IBEACON,
    WIFI_OUI,
    WIFI_SSID,
    WIFI_PWNAGOTCHI,
    WIFI_EVILTWIN,
    WIFI_DEAUTH,
};

inline const char* evidenceKindName(EvidenceKind e) {
    switch (e) {
        case EvidenceKind::BLE_MFG:         return "BLE MFG ID";
        case EvidenceKind::BLE_UUID:        return "BLE SERVICE UUID";
        case EvidenceKind::BLE_NAME:        return "BLE NAME";
        case EvidenceKind::BLE_FINDMY:      return "FIND MY PAYLOAD";
        case EvidenceKind::BLE_IBEACON:     return "IBEACON PAYLOAD";
        case EvidenceKind::WIFI_OUI:        return "WIFI OUI";
        case EvidenceKind::WIFI_SSID:       return "WIFI SSID";
        case EvidenceKind::WIFI_PWNAGOTCHI: return "PWNAGOTCHI FRAME";
        case EvidenceKind::WIFI_EVILTWIN:   return "SSID/SECURITY CONFLICT";
        case EvidenceKind::WIFI_DEAUTH:     return "DEAUTH BURST";
        default:                            return "UNKNOWN";
    }
}

inline const char* evidenceKindShortName(EvidenceKind e) {
    switch (e) {
        case EvidenceKind::BLE_MFG:         return "MFG";
        case EvidenceKind::BLE_UUID:        return "UUID";
        case EvidenceKind::BLE_NAME:        return "NAME";
        case EvidenceKind::BLE_FINDMY:      return "FINDMY";
        case EvidenceKind::BLE_IBEACON:     return "IBEACON";
        case EvidenceKind::WIFI_OUI:        return "OUI";
        case EvidenceKind::WIFI_SSID:       return "SSID";
        case EvidenceKind::WIFI_PWNAGOTCHI: return "PWN";
        case EvidenceKind::WIFI_EVILTWIN:   return "EVIL";
        case EvidenceKind::WIFI_DEAUTH:     return "DEAUTH";
        default:                            return "";
    }
}

enum class RssiTrend : uint8_t { UNKNOWN = 0, APPROACHING, STEADY, MOVING_AWAY };

inline const char* rssiTrendName(RssiTrend t) {
    switch (t) {
        case RssiTrend::APPROACHING: return "APPROACHING";
        case RssiTrend::STEADY:      return "STEADY";
        case RssiTrend::MOVING_AWAY: return "MOVING AWAY";
        default:                     return "LEARNING";
    }
}

struct Detection {
    uint8_t        mac[6];
    int8_t         rssi;
    uint8_t        channel;        // 0 if N/A
    DetectionType  type;
    // Kept from before this boot (the black box): firstSeen then holds the
    // wall-clock second it was seen, not a millis() stamp, and lastSeen is 0.
    // Cleared the moment the device is seen again. Sits in what was padding,
    // so the log costs no more RAM than it did.
    uint8_t        restored;
    // The signal a couple of seconds ago, and when that was in units of two
    // seconds (wrapping every eight and a half minutes, which is far longer
    // than a row stays fresh). The LOG's closer/further arrow reads these:
    // comparing against the last ADVERT would only show the wobble a still
    // device makes, which is why the raw scan's arrow samples too.
    int8_t         prevRssi;
    uint8_t        prevAt;
    // Who made it, when the match knew: a pointer into the signature
    // tables or a literal, never a copy. Every vendor string in this
    // firmware is a compile-time constant, so a log of two hundred rows
    // was carrying two hundred copies of "Apple" -- eight bytes a row
    // against a pointer, and the log is the largest thing the board
    // allocates. nullptr when nothing named it: read it through
    // vendorText() rather than testing it at every use.
    const char*    vendor;
    char           name[20];
    uint32_t       firstSeen;
    uint32_t       lastSeen;
    uint16_t       hits;
    // AUTO SNOOZE (Settings). `alerts` counts the full-screen alerts this
    // device has actually raised; `quietBar` is the strongest RSSI it ever
    // alerted at. Past the threshold it has to beat that bar by a margin to
    // interrupt again -- so the board stops repeating itself about the
    // doorbell across the street without ever going deaf to it coming
    // closer, which a plain mute would not manage.
    //
    // NOT `hits`: that counts every sighting on the WiFi path and only the
    // returns on the Bluetooth one, so it means two different things
    // depending on which radio found the device.
    uint8_t        alerts;
    int8_t         quietBar;
    // When it last spent one. NOT lastSeen: by the time the gate runs, the
    // reactivation path has already moved lastSeen to now, so "how long has
    // it been gone" measured from it is always zero and the allowance would
    // never be handed back.
    uint32_t       lastAlertMs;
    // The grade of the signature that actually matched, not the grade of
    // the type. See lookupOui().
    Confidence     conf;
    // Why the classifier believed this sighting. evidenceCode is used for
    // compact numeric evidence (BLE company/service ID); restored historical
    // rows from releases that predate this field naturally remain UNKNOWN.
    EvidenceKind   evidence;
    uint16_t       evidenceCode;

    // Uniform raw sighting count. Unlike hits, this has the same meaning for
    // BLE, Wi-Fi and deauth detections and saturates at 255. Stage B can use
    // it for minimum-repeat alert rules without changing Stage A semantics.
    uint8_t        repeats;

    // Eight roughly-two-second RSSI samples: about fifteen seconds of signal
    // history without dynamic allocation. Existing prevRssi/prevAt semantics
    // remain intact for the LOG's legacy up/down indicator.
    int8_t         rssiHist[8];
    uint8_t        rssiHistHead;
    uint8_t        rssiHistCount;

    bool           active;
    // When it last asked to interrupt, allowed or not, in minutes of uptime
    // (wrapping after 45 days). A device that keeps coming back keeps asking,
    // so the gap since the last ask is how long it was really gone. Sits in
    // what was padding.
    uint16_t       askedMin;
};

inline void detectionRssiInit(Detection& d, int8_t rssi, uint32_t nowMs) {
    d.rssi = rssi;
    d.prevRssi = rssi;
    d.prevAt = (uint8_t)(nowMs >> 11);
    d.rssiHist[0] = rssi;
    d.rssiHistHead = 1;
    d.rssiHistCount = 1;
}

// Sample at the same ~2.048 s cadence used by the existing closer/further
// indicator. repeats is updated on every sighting by the detection engine;
// history is intentionally cadence-limited so chatty beacons cannot fill the
// whole window in milliseconds.
inline void detectionRssiSample(Detection& d, int8_t rssi, uint32_t nowMs) {
    const uint8_t nowAt = (uint8_t)(nowMs >> 11);
    if (d.rssiHistCount == 0) {
        detectionRssiInit(d, rssi, nowMs);
        return;
    }
    if (d.prevAt != nowAt) {
        d.prevRssi = d.rssi;
        d.prevAt = nowAt;
        d.rssiHist[d.rssiHistHead] = rssi;
        d.rssiHistHead = (uint8_t)((d.rssiHistHead + 1u) & 7u);
        if (d.rssiHistCount < 8) d.rssiHistCount++;
    }
    d.rssi = rssi;
}

inline int8_t detectionRssiAt(const Detection& d, uint8_t idx) {
    if (d.rssiHistCount == 0 || idx >= d.rssiHistCount) return d.rssi;
    const uint8_t first =
        (uint8_t)((d.rssiHistHead + 8u - d.rssiHistCount) & 7u);
    return d.rssiHist[(uint8_t)((first + idx) & 7u)];
}

inline RssiTrend detectionRssiTrend(const Detection& d) {
    if (d.rssiHistCount < 3) return RssiTrend::UNKNOWN;
    const int oldest = (int)detectionRssiAt(d, 0);
    const int newest = (int)detectionRssiAt(d, (uint8_t)(d.rssiHistCount - 1));
    const int delta = newest - oldest;
    if (delta >= 6) return RssiTrend::APPROACHING;
    if (delta <= -6) return RssiTrend::MOVING_AWAY;
    return RssiTrend::STEADY;
}

// A detection's vendor, safe to print. A record is zeroed before it is
// filled, so "no vendor" arrives here as a null pointer.
inline const char* vendorText(const Detection& d) { return d.vendor ? d.vendor : ""; }

enum class AppState : uint8_t {
    BOOT     = 0,
    CLEAR    = 1,
    ALERT    = 2,
    LOG      = 3,
    SETTINGS = 4,
    DIARY    = 5,
    OUTFIT   = 6,
    RAWSCAN     = 7,  // manual BLE/WiFi scanner, reached via CLEAR's SCAN picker
    WATCH_ALERT = 8,  // a watched target (see DetectionEngine::watchBle/watchWifi) came back in range
    DIAGNOSTICS = 9,  // on-device diagnostics screen, reached via Settings
    HUNT        = 10, // live signal-strength gauge for the watched target,
                       // reached via raw-scan's long-press confirm panel
                       // (the "HUNT" choice alongside WATCH/CANCEL)
    COLOR_CHECK = 11, // first-boot RED/GREEN/BLUE display sanity check,
                       // also reachable later via Settings' "CHECK COLORS" row
    DETECTION_FILTER = 12, // per-type detection on/off, reached via
                            // Settings' "DETECTION FILTER" row
    OUTFIT_UNLOCK    = 13, // "OUTFIT UNLOCKED" celebration, pushed
                            // automatically whenever Squachy earns a new
                            // costume; returns to CLEAR when dismissed
    IGNORE_LIST      = 14, // persistent per-device behavior, reached via
                            // Settings' POLICIES row; name retained for compatibility
    POWER_SAVER      = 15, // battery settings, reached via Settings'
                            // "POWER SAVER" row
    PHONE            = 16, // the payphone: type a name for Squachy. Reached
                            // from the SquachMesh menu. Only ever entered on
                            // a SquachMesh build -- the row is not offered
                            // otherwise -- but the state costs nothing.
    MESH_MENU        = 17, // SquachMesh's own screen: detect, transmit, name
    MESH_WARN        = 18, // the consent gate in front of it. Stands before
                            // the MENU rather than before the TRANSMIT row:
                            // a warning read next to a switch reads as an
                            // obstacle, one read before there is anything to
                            // click reads as information.
    MESH_PHRASE      = 19, // roll, show or enter the five-word phrase
    MESH_COMPOSE     = 20, // send a message; opened from CLEAR's little bubble
    BEACON_WARN      = 21, // what switching iBeacons on means, asked from
                            // DETECTION FILTER before it happens
    SECURITY         = 22, // the SECURITY submenu (PIN lock + the rest)
    LOCKED           = 23, // the lock screen: the payphone, digits only
    PIN_ENTRY        = 24, // setting, changing or checking a PIN, from SECURITY
    SQUAD            = 25, // every SquachWatch in range, and the inbox
    UPDATE           = 26, // UPDATE FIRMWARE, from Settings' SYSTEM page:
                            // WiFi or Bluetooth update, or switch slots
    WIFI_PASS        = 27, // typing a WiFi password, from UPDATE's network list
    STATUS_LIGHT     = 28, // the RGB LED's settings, from the APPEARANCE page
    NUDGE            = 29, // another board asked the squad to update: the countdown
    SQUAD_UPDATE     = 30, // UPDATE SQUAD, from the UPDATE FIRMWARE screen
    INVITE           = 31, // ADD TO SQUAD, either side of it
    DESK             = 32, // desk mode: the clock, the date, the focus timer
    WIFI_NETS        = 33, // WIFI NETWORKS, from the SYSTEM page: the saved list
    WIFI_ADD         = 34, // ...and the scan to add one from
    SYS_PROPS        = 35, // SYSTEM PROPERTIES: the window that says an update
                            // is out, in front of the main screen on the first
                            // frame after the intro. See ui_sysprops.h.
    // 36 was CROWD, a page holding one setting; the SquachMesh menu's CROWD
    // row steps it in place now.
    BINGO            = 36, // the detection bingo card, from Settings' BINGO
                            // row. See ui_bingo.h.
    DEX              = 37, // the SQUACHY-DEX, from Settings' row. See ui_dex.h.
    ALERT_RULES      = 38  // per-type alert/log-only/confidence/repeats/cooldown/wake
};

enum class ButtonId : uint8_t {
    NONE  = 255,
    SCAN  = 0,
    LOG   = 1,
    CLR   = 2
};
