#ifndef ZAHL_PET_H
#define ZAHL_PET_H

#include <Arduino.h>

struct ZahlStruct {
    uint8_t hunger;      // 0-100, 0 = dead, 100 = full
    uint8_t level;       // 1-10
    uint8_t corrosion;   // 0-100, >50 triggers glitches
    uint32_t lastFed;
    uint32_t experience;
    
    void feed(uint8_t amount);
    void pet();
    void addExperience(uint8_t exp);
    void update();
};

extern ZahlStruct Zahl;
extern bool feralMode;

void updateZahlState();
String getZahlStatus();
String getZahlJson();

#endif
