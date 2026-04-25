#include "beacon_spam.h"
#include <esp_wifi.h>

static bool    spamEnabled  = false;
static uint32_t lastBeacon  = 0;
static int     ssidIdx      = 0;
static bool    hauntMode    = false;

static const char* DEMON_SSIDS[] = {
    "DAGOTH_UR_NETWORK",
    "HOUSE_DAGOTH_WIFI",
    "CORPUS_DISEASE_5G",
    "SIXTH_HOUSE_CELL",
    "NEREVARINE_TRAP",
    "TRIBUNAL_VOID_AP",
    "ZAHL_IS_HUNGRY",
    "GHOST_FENCE_ROUTER",
    "VIVEC_DARK_NET",
    "FEED_THE_DAEDROTH",
};
static const int DEMON_COUNT = 10;

#define MAX_HAUNT_SSIDS 16
static char hauntSsids[MAX_HAUNT_SSIDS][33];
static int  hauntSsidCount = 0;

// Raw 802.11 beacon frame template (up to SSID element)
static const uint8_t BEACON_HDR[] = {
    0x80, 0x00,                               // Frame ctrl: beacon
    0x00, 0x00,                               // Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,       // DA: broadcast
    0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00,       // SA: fake (bytes 14-15 = idx)
    0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00,       // BSSID: same
    0x00, 0x00,                               // Seq
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // Timestamp
    0x64, 0x00,                               // Beacon interval (100 TU)
    0x01, 0x04,                               // Capability
};

static const uint8_t RATES[] = {
    0x01, 0x08, 0x82,0x84,0x8b,0x96,0x0c,0x12,0x18,0x24, // rates
    0x03, 0x01, 0x06, // DS param: ch 6
};

void initBeaconSpam() {
    spamEnabled = false;
    hauntMode   = false;
}

void setHauntSsids(const char ssids[][33], int count) {
    hauntSsidCount = min(count, MAX_HAUNT_SSIDS);
    for (int i = 0; i < hauntSsidCount; i++)
        strlcpy(hauntSsids[i], ssids[i], 33);
    hauntMode = (hauntSsidCount > 0);
}

void runBeaconCycle() {
    if (!spamEnabled) return;
    if (millis() - lastBeacon < 80) return;
    lastBeacon = millis();

    const char* ssid;
    if (hauntMode && hauntSsidCount > 0) {
        ssid = hauntSsids[ssidIdx % hauntSsidCount];
    } else {
        ssid = DEMON_SSIDS[ssidIdx % DEMON_COUNT];
    }
    ssidIdx++;

    uint8_t ssidLen = strnlen(ssid, 32);
    uint8_t frame[128];
    memcpy(frame, BEACON_HDR, sizeof(BEACON_HDR));

    // Vary SA/BSSID per ssidIdx
    frame[14] = ssidIdx & 0xFF;
    frame[15] = (ssidIdx >> 8) & 0xFF;
    frame[20] = frame[14];
    frame[21] = frame[15];

    int pos = sizeof(BEACON_HDR);
    frame[pos++] = 0x00;       // SSID element ID
    frame[pos++] = ssidLen;
    memcpy(frame + pos, ssid, ssidLen);
    pos += ssidLen;
    memcpy(frame + pos, RATES, sizeof(RATES));
    pos += sizeof(RATES);

    esp_wifi_80211_tx(WIFI_IF_AP, frame, pos, false);
}

void setBeaconSpamEnabled(bool en) { spamEnabled = en; if (!en) hauntMode = false; }
bool isBeaconSpamming()            { return spamEnabled; }
