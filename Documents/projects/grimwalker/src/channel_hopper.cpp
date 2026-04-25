#include "channel_hopper.h"
#include <esp_wifi.h>

static const uint8_t HOP_CHANNELS[] = {1,2,3,4,5,6,7,8,9,10,11,12,13};
static uint8_t  chIdx       = 0;
static uint16_t hopInterval = 200;
static bool     hopEnabled  = true;
static uint32_t lastHop     = 0;
static uint8_t  currentCh   = 1;

void initChannelHopper() {
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    currentCh = 1;
    Serial.println("Channel hopper ready");
}

void hopChannel() {
    if (!hopEnabled) return;
    if (millis() - lastHop < hopInterval) return;
    lastHop  = millis();
    chIdx    = (chIdx + 1) % 13;
    currentCh = HOP_CHANNELS[chIdx];
    esp_wifi_set_channel(currentCh, WIFI_SECOND_CHAN_NONE);
}

void    setHopInterval(uint16_t ms) { hopInterval = max((uint16_t)50, ms); }
void    setHopEnabled(bool en)      { hopEnabled  = en; }
uint8_t getCurrentChannel()         { return currentCh; }
bool    isHopping()                 { return hopEnabled; }
