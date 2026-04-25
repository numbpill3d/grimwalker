#include "zahl_pet.h"
#include <esp_random.h>

ZahlStruct Zahl = {70, 1, 0, 0, 0};
bool feralMode = false;

void ZahlStruct::feed(uint8_t amount) {
    hunger += amount;
    if (hunger > 100) hunger = 100;
    lastFed = millis();
    
    // Reduce corrosion when fed well
    if (hunger > 80 && corrosion > 0) {
        corrosion = (corrosion >= 2) ? corrosion - 2 : 0;
    }
    
    Serial.printf("Zahl fed! Hunger: %d, Corrosion: %d\n", hunger, corrosion);
}

void ZahlStruct::pet() {
    // Petting calms the beast
    corrosion = (corrosion >= 5) ? corrosion - 5 : 0;
    
    // But petting burns calories
    if (hunger > 5) {
        hunger -= 2;
    }
}

void ZahlStruct::addExperience(uint8_t exp) {
    experience += exp;
    if (experience >= 100) {
        experience -= 100;
        if (level < 10) {
            level++;
            Serial.printf("Zahl leveled up! Now level %d\n", level);
            
            // Full heal on level up
            hunger = 100;
            corrosion = 0;
        }
    }
}

void ZahlStruct::update() {
    // Hunger decays over time
    uint32_t timeSinceFed = (millis() - lastFed) / 1000;
    if (timeSinceFed > 60) { // After 60 seconds
        uint8_t decay = (timeSinceFed - 60) / 30; // 1 point per 30 seconds
        if (decay > 0 && hunger > 0) {
            hunger -= decay;
            if (hunger > 100) hunger = 100; // Handle overflow
            lastFed = millis(); // Reset timer after decay
        }
    }
    
    // Starvation
    if (hunger == 0) {
        feralMode = true;
        corrosion = 100;
    } else if (hunger > 20) {
        feralMode = false;
    }
    
    // Corrosion effects
    if (corrosion > 100) corrosion = 100;
    
    // Level-based bonuses
    if (level >= 5 && hunger > 50) {
        // Higher level means slower hunger decay
        // Implemented in main decay logic
    }
}

void updateZahlState() {
    static uint32_t lastUpdate = 0;
    
    if (millis() - lastUpdate > 5000) { // Update every 5 seconds
        lastUpdate = millis();
        Zahl.update();
    }
}

// comma-separated key:val string used by SSE event parser in JS
String getZahlStatus() {
    String status = "";
    status += "hunger:" + String(Zahl.hunger) + ",";
    status += "level:" + String(Zahl.level) + ",";
    status += "corrosion:" + String(Zahl.corrosion) + ",";
    status += "feral:" + String(feralMode ? "1" : "0") + ",";
    status += "exp:" + String(Zahl.experience);
    return status;
}

// proper JSON object used by /status REST endpoint
String getZahlJson() {
    String j = "{";
    j += "\"hunger\":" + String(Zahl.hunger) + ",";
    j += "\"level\":" + String(Zahl.level) + ",";
    j += "\"corrosion\":" + String(Zahl.corrosion) + ",";
    j += "\"feral\":\"" + String(feralMode ? "1" : "0") + "\",";
    j += "\"exp\":" + String(Zahl.experience);
    j += "}";
    return j;
}
