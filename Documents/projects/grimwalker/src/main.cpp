#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include "webserver.h"
#include "packet_sniffer.h"
#include "zahl_pet.h"

void snifferLoop(void *pvParameters);
void petMaintenanceLoop(void *pvParameters);

// RTC memory for deep sleep preservation
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR uint8_t savedHunger = 100;
RTC_DATA_ATTR uint8_t savedLevel = 1;
RTC_DATA_ATTR uint8_t savedCorrosion = 0;

TaskHandle_t snifferTaskHandle = NULL;
TaskHandle_t petTaskHandle = NULL;

// Touch pin interrupt
void IRAM_ATTR touchInterrupt() {
    // Just set a flag - actual handling in main loop
    static bool touched = false;
    touched = true;
}

void setup() {
    Serial.begin(115200);
    delay(100);
    
    bootCount++;
    Serial.printf("GRIMWALKER v1.0 - Boot #%d\n", bootCount);
    
    // Load saved state
    Zahl.hunger = savedHunger;
    Zahl.level = savedLevel;
    Zahl.corrosion = savedCorrosion;
    Zahl.lastFed = millis();
    
    // Initialize WiFi in AP+STA mode
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("GRIMWALKER", NULL, 6, 0, 4);
    Serial.println("SoftAP started - IP: 192.168.4.1");
    
    // Start promiscuous sniffing
    initPacketSniffer();
    
    // Start web server
    initWebServer();
    
    // Setup touch pin
    touchAttachInterrupt(T0, touchInterrupt, 40);
    
    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(
        snifferLoop,      // Task function
        "SnifferTask",    // Name
        8192,             // Stack size
        NULL,             // Parameters
        2,                // Priority
        &snifferTaskHandle, // Handle
        0                 // Core 0
    );
    
    xTaskCreatePinnedToCore(
        petMaintenanceLoop,
        "PetTask",
        4096,
        NULL,
        1,
        &petTaskHandle,
        1                 // Core 1
    );
    
    Serial.println("GRIMWALKER ready - Feed your Daedroth!");
}

void loop() {
    // Check touch pin
    static uint32_t lastTouchTime = 0;
    if (touchRead(T0) < 20 && (millis() - lastTouchTime) > 1000) {
        lastTouchTime = millis();
        Zahl.pet();
        Serial.println("Zahl has been petted");
        
        // Flash LED if available
        pinMode(2, OUTPUT);
        digitalWrite(2, HIGH);
        delay(50);
        digitalWrite(2, LOW);
    }
    
    // Deep sleep after 10 minutes of no interaction
    static uint32_t lastActivity = millis();
    if ((millis() - lastActivity) > 600000) { // 10 min
        // Save state before sleep
        savedHunger = Zahl.hunger;
        savedLevel = Zahl.level;
        savedCorrosion = Zahl.corrosion;
        
        Serial.println("Entering deep sleep...");
        esp_sleep_enable_touchpad_wakeup();
        esp_deep_sleep_start();
    }
    
    delay(100);
}

void snifferLoop(void *pvParameters) {
    while(1) {
        runSnifferCycle();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void petMaintenanceLoop(void *pvParameters) {
    while(1) {
        updateZahlState();
        
        // Broadcast hunger via BLE
        if (millis() % 30000 < 100) { // Every 30 seconds
            startBLEBeacon();
        }
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
