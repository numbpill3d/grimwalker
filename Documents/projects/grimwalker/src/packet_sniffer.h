#ifndef PACKET_SNIFFER_H
#define PACKET_SNIFFER_H

#include <Arduino.h>

#define MAX_LOG_ENTRIES 30

struct LogEntry {
    char     data[32];
    int      rssi;
    uint32_t time;
};

extern LogEntry captureLog[];
extern LogEntry networkLog[];
extern int      captureLogIndex;
extern int      networkLogIndex;

// haunt mode: nearby real SSIDs for beacon_spam
extern char hauntNetworks[][33];
extern int  hauntNetworkCount;

void initPacketSniffer();
void runSnifferCycle();
void resumePromiscuous();
void sendDeauthFrame(uint8_t* targetMac, uint8_t* routerMac);
int  captureHandshake(uint8_t* bssid);
void scanBLEDevices();
void addToCaptureLog(const char* data, int rssi);
void addToNetworkLog(const char* ssid, int rssi);

#endif
