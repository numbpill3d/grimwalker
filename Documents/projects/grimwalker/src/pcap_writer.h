#ifndef PCAP_WRITER_H
#define PCAP_WRITER_H

#include <Arduino.h>

void   initPcapWriter();
void   writePcapPacket(const uint8_t* data, uint16_t len);
void   rotatePcap();
size_t getPcapSize();
bool   isPcapReady();

#endif
