#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include "webserver.h"
#include "packet_sniffer.h"
#include "zahl_pet.h"
#include "soul_ledger.h"
#include "evil_portal.h"
#include "beacon_spam.h"
#include "pcap_writer.h"
#include "channel_hopper.h"

void snifferLoop(void* pv);
void petMaintenanceLoop(void* pv);
void portalDnsLoop(void* pv);

RTC_DATA_ATTR int     bootCount      = 0;
RTC_DATA_ATTR uint8_t savedHunger    = 100;
RTC_DATA_ATTR uint8_t savedLevel     = 1;
RTC_DATA_ATTR uint8_t savedCorrosion = 0;
RTC_DATA_ATTR uint8_t savedEntropy   = 10;
RTC_DATA_ATTR uint8_t savedCorrupt   = 0;
RTC_DATA_ATTR uint16_t savedSoulBal  = 0;
RTC_DATA_ATTR uint16_t savedSoulsAll = 0;

TaskHandle_t snifferTaskHandle = NULL;
TaskHandle_t petTaskHandle     = NULL;
TaskHandle_t dnsTaskHandle     = NULL;

void IRAM_ATTR touchInterrupt() {}

void setup() {
    Serial.begin(115200);
    delay(100);
    bootCount++;
    Serial.printf("GRIMWALKER boot #%d\n", bootCount);

    // Restore state from RTC memory
    Zahl.hunger     = savedHunger;
    Zahl.level      = savedLevel;
    Zahl.corrosion  = savedCorrosion;
    Zahl.entropy    = savedEntropy;
    Zahl.corruption = savedCorrupt;
    Zahl.soulBalance = savedSoulBal;
    Zahl.soulsTotal  = savedSoulsAll;
    Zahl.lastFed    = millis();

    // WiFi AP+STA
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("GRIMWALKER", NULL, 6, 0, 4);
    Serial.println("AP: 192.168.4.1");

    // Init subsystems
    initSoulLedger();
    initBeaconSpam();
    initEvilPortal();
    initPcapWriter();
    initPacketSniffer(); // also inits channel hopper
    initWebServer();

    touchAttachInterrupt(T0, touchInterrupt, 40);

    // Core 0: sniffer + channel hop + beacon spam
    xTaskCreatePinnedToCore(snifferLoop, "sniffer", 8192, NULL, 2, &snifferTaskHandle, 0);
    // Core 1: pet maintenance + soul ledger saves
    xTaskCreatePinnedToCore(petMaintenanceLoop, "petmaint", 4096, NULL, 1, &petTaskHandle, 1);
    // Core 1: DNS server for evil portal
    xTaskCreatePinnedToCore(portalDnsLoop, "dns", 2048, NULL, 1, &dnsTaskHandle, 1);

    Serial.println("GRIMWALKER ready");
}

void loop() {
    // Touch pet
    static uint32_t lastTouch = 0;
    if (touchRead(T0) < 20 && millis() - lastTouch > 1000) {
        lastTouch = millis();
        Zahl.pet();
        pinMode(2, OUTPUT);
        digitalWrite(2, HIGH); delay(50); digitalWrite(2, LOW);
        Serial.println("Touch pet");
    }

    // Deep sleep after 10 min idle
    static uint32_t lastActivity = millis();
    if (millis() - lastActivity > 600000) {
        savedHunger    = Zahl.hunger;
        savedLevel     = Zahl.level;
        savedCorrosion = Zahl.corrosion;
        savedEntropy   = Zahl.entropy;
        savedCorrupt   = Zahl.corruption;
        savedSoulBal   = Zahl.soulBalance;
        savedSoulsAll  = Zahl.soulsTotal;
        saveSouls();
        Serial.println("Deep sleep...");
        esp_sleep_enable_touchpad_wakeup();
        esp_deep_sleep_start();
    }

    delay(100);
}

void snifferLoop(void*) {
    while (1) {
        runSnifferCycle();
        runBeaconCycle();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void petMaintenanceLoop(void*) {
    while (1) {
        updateZahlState();
        tickSoulLedger();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void portalDnsLoop(void*) {
    while (1) {
        runPortalDns();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
