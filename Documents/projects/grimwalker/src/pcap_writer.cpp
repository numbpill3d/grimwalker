#include "pcap_writer.h"
#include <SPIFFS.h>

static const char* PCAP_PATH = "/capture.pcap";
static const size_t MAX_PCAP = 512 * 1024; // 512 KB cap

static File   pcapFile;
static bool   pcapReady    = false;
static uint32_t pktCount   = 0;

// PCAP global header (little-endian, 802.11 link type = 105)
static const uint8_t PCAP_GLOBAL_HDR[24] = {
    0xd4,0xc3,0xb2,0xa1, // magic
    0x02,0x00,           // major ver
    0x04,0x00,           // minor ver
    0x00,0x00,0x00,0x00, // timezone
    0x00,0x00,0x00,0x00, // sigfigs
    0x00,0x08,0x00,0x00, // snaplen 2048
    0x69,0x00,0x00,0x00, // link type 105 = IEEE802_11
};

void initPcapWriter() {
    pcapFile = SPIFFS.open(PCAP_PATH, "w");
    if (!pcapFile) {
        Serial.println("PCAP: open failed");
        return;
    }
    pcapFile.write(PCAP_GLOBAL_HDR, sizeof(PCAP_GLOBAL_HDR));
    pcapFile.flush();
    pcapReady = true;
    pktCount  = 0;
    Serial.println("PCAP writer ready");
}

void writePcapPacket(const uint8_t* data, uint16_t len) {
    if (!pcapReady) return;
    if (pcapFile.size() > MAX_PCAP) return;

    uint32_t ts   = millis();
    uint32_t sec  = ts / 1000;
    uint32_t usec = (ts % 1000) * 1000;
    uint16_t caplen = min((int)len, 2048);

    // Per-packet header (16 bytes)
    pcapFile.write((uint8_t*)&sec,    4);
    pcapFile.write((uint8_t*)&usec,   4);
    pcapFile.write((uint8_t*)&caplen, 4);
    pcapFile.write((uint8_t*)&len,    4);  // orig_len (same type width, fine)
    pcapFile.write(data, caplen);
    pcapFile.flush();
    pktCount++;
}

void rotatePcap() {
    if (pcapReady) pcapFile.close();
    SPIFFS.remove(PCAP_PATH);
    initPcapWriter();
}

size_t getPcapSize() {
    if (!SPIFFS.exists(PCAP_PATH)) return 0;
    File f = SPIFFS.open(PCAP_PATH, "r");
    size_t s = f.size();
    f.close();
    return s;
}

bool isPcapReady() { return pcapReady; }
