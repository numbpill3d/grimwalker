#ifndef ZAHL_PET_H
#define ZAHL_PET_H

#include <Arduino.h>

// Ritual types
#define RITUAL_CHANNEL_BURST 0  // cost 2 — fast channel hop 30s
#define RITUAL_BEACON_STORM  1  // cost 3 — beacon spam 60s
#define RITUAL_HAUNT         2  // cost 5 — beacon spam as real nearby SSIDs
#define RITUAL_SUMMON_PORTAL 3  // cost 8 — activate evil portal
#define RITUAL_PACKET_STORM  4  // cost 4 — rapid deauth + beacon 30s

struct ZahlStruct {
    uint8_t  hunger;        // 0-100
    uint8_t  level;         // 1-10
    uint8_t  corrosion;     // 0-100, >50 triggers glitches
    uint8_t  entropy;       // 0-100, chaos/variety
    uint8_t  corruption;    // 0-100, cumulative evil — never decreases
    uint32_t lastFed;
    uint32_t experience;    // 0-99, rolls over → level up
    uint16_t soulBalance;   // spendable souls
    uint16_t soulsTotal;    // all-time souls captured

    void feed(uint8_t amount);
    void pet();
    void addExperience(uint8_t exp);
    void update();
    void consumeSoul(uint8_t soulType);
    bool performRitual(uint8_t type);
};

extern ZahlStruct Zahl;
extern bool       feralMode;
extern bool       ritualActive;
extern uint8_t    activeRitual;
extern uint32_t   ritualExpiry;

void   updateZahlState();
String getZahlStatus();
String getZahlJson();

#endif
