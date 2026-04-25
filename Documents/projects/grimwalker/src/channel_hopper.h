#ifndef CHANNEL_HOPPER_H
#define CHANNEL_HOPPER_H

#include <Arduino.h>

void    initChannelHopper();
void    hopChannel();
void    setHopInterval(uint16_t ms);
void    setHopEnabled(bool en);
uint8_t getCurrentChannel();
bool    isHopping();

#endif
