#ifndef SOUL_LEDGER_H
#define SOUL_LEDGER_H

#include <Arduino.h>

#define MAX_SOULS    100
#define SOUL_PROBE   0   // WiFi device seen
#define SOUL_HAND    1   // WPA handshake captured
#define SOUL_PORTAL  2   // Portal credential captured
#define SOUL_BLE     3   // BLE device

struct Soul {
    char    id[20];       // MAC or BLE address
    char    label[33];    // SSID or name if known
    uint8_t type;
    int8_t  rssi;
    uint32_t capturedAt;
};

extern Soul souls[MAX_SOULS];
extern int  soulCount;
extern bool pendingSoulSave;

void    initSoulLedger();
bool    addSoul(const char* id, const char* label, uint8_t type, int8_t rssi);
bool    soulKnown(const char* id);
void    saveSouls();
void    tickSoulLedger();
String  getSoulsJson(int limit);
String  getJournalEntry(int idx);

#endif
