#ifndef PACKET_SNIFFER_H
#define PACKET_SNIFFER_H

#include <Arduino.h>

#define MAX_LOG_ENTRIES 20

struct LogEntry {
    char data[32];
    int rssi;
    uint32_t time;
};

extern LogEntry captureLog[];
extern LogEntry networkLog[];
extern int captureLogIndex;
extern int networkLogIndex;

void initPacketSniffer();
void runSnifferCycle();
void sendDeauthFrame(uint8_t* targetMac, uint8_t* routerMac);
int captureHandshake(uint8_t* bssid);
void startBLEBeacon();
void addToCaptureLog(const char* mac, int rssi);
void addToNetworkLog(const char* ssid, int rssi);

#endif
