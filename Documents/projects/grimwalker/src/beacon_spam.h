#ifndef BEACON_SPAM_H
#define BEACON_SPAM_H

#include <Arduino.h>

void initBeaconSpam();
void runBeaconCycle();
void setBeaconSpamEnabled(bool en);
void setHauntSsids(const char ssids[][33], int count); // haunt mode: use real SSIDs
bool isBeaconSpamming();

#endif
