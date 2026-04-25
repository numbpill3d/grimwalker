#ifndef GRIMWALKER_WEBSERVER_H
#define GRIMWALKER_WEBSERVER_H

#include <Arduino.h>

void initWebServer();
void addToCaptureLog(const char* data, int rssi);
void addToNetworkLog(const char* data, int rssi);

#endif // GRIMWALKER_WEBSERVER_H
