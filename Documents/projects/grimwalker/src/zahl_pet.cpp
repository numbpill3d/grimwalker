#include "zahl_pet.h"
#include "beacon_spam.h"
#include "channel_hopper.h"
#include "evil_portal.h"

ZahlStruct Zahl = {70, 1, 0, 10, 0, 0, 0, 0, 0};
bool       feralMode    = false;
bool       ritualActive = false;
uint8_t    activeRitual = 0xFF;
uint32_t   ritualExpiry = 0;

static const uint16_t RITUAL_COSTS[]    = {2, 3, 5, 8, 4};
static const uint32_t RITUAL_DURATIONS[]= {30000, 60000, 60000, 0, 30000}; // ms; 0=toggle

void ZahlStruct::feed(uint8_t amount) {
    hunger = min(100, (int)hunger + amount);
    lastFed = millis();
    if (hunger > 80 && corrosion > 0)
        corrosion = (corrosion >= 2) ? corrosion - 2 : 0;
    Serial.printf("Zahl fed: hunger=%d\n", hunger);
}

void ZahlStruct::pet() {
    corrosion = (corrosion >= 5) ? corrosion - 5 : 0;
    if (hunger > 5) hunger -= 2;
    if (entropy < 100) entropy = min(100, (int)entropy + 2);
}

void ZahlStruct::addExperience(uint8_t exp) {
    experience += exp;
    if (experience >= 100) {
        experience -= 100;
        if (level < 10) {
            level++;
            hunger     = 100;
            corrosion  = 0;
            Serial.printf("Zahl level up: %d\n", level);
        }
    }
}

void ZahlStruct::consumeSoul(uint8_t soulType) {
    soulBalance = min(999, (int)soulBalance + 1);
    soulsTotal  = min(9999, (int)soulsTotal + 1);

    switch (soulType) {
        case 0: // PROBE
            feed(3);
            if (entropy < 100) entropy = min(100, (int)entropy + 1);
            break;
        case 1: // HANDSHAKE
            feed(15);
            addExperience(10);
            if (corruption < 100) corruption = min(100, (int)corruption + 5);
            if (entropy   < 100) entropy    = min(100, (int)entropy    + 3);
            break;
        case 2: // PORTAL
            feed(20);
            addExperience(15);
            if (corruption < 100) corruption = min(100, (int)corruption + 10);
            if (entropy   < 100) entropy    = min(100, (int)entropy    + 5);
            break;
        case 3: // BLE
            if (entropy < 100) entropy = min(100, (int)entropy + 2);
            feed(2);
            break;
    }
}

bool ZahlStruct::performRitual(uint8_t type) {
    if (type > 4) return false;
    if (soulBalance < RITUAL_COSTS[type]) return false;

    soulBalance -= RITUAL_COSTS[type];
    corruption   = min(100, (int)corruption + 8);

    switch (type) {
        case RITUAL_CHANNEL_BURST:
            setHopInterval(50);
            setHopEnabled(true);
            ritualActive = true;
            activeRitual = type;
            ritualExpiry = millis() + RITUAL_DURATIONS[type];
            Serial.println("Ritual: CHANNEL BURST");
            break;

        case RITUAL_BEACON_STORM:
            setHauntSsids(nullptr, 0);
            setBeaconSpamEnabled(true);
            ritualActive = true;
            activeRitual = type;
            ritualExpiry = millis() + RITUAL_DURATIONS[type];
            Serial.println("Ritual: BEACON STORM");
            break;

        case RITUAL_HAUNT: {
            // copy nearby SSIDs from network log — populated by packet_sniffer
            extern char hauntNetworks[][33];
            extern int  hauntNetworkCount;
            setHauntSsids((const char(*)[33])hauntNetworks, hauntNetworkCount);
            setBeaconSpamEnabled(true);
            ritualActive = true;
            activeRitual = type;
            ritualExpiry = millis() + RITUAL_DURATIONS[type];
            Serial.println("Ritual: HAUNT");
            break;
        }

        case RITUAL_SUMMON_PORTAL:
            setPortalEnabled(!isPortalActive());
            ritualActive = false;
            Serial.println("Ritual: SUMMON PORTAL");
            break;

        case RITUAL_PACKET_STORM:
            setHopInterval(50);
            setBeaconSpamEnabled(true);
            ritualActive = true;
            activeRitual = type;
            ritualExpiry = millis() + RITUAL_DURATIONS[type];
            Serial.println("Ritual: PACKET STORM");
            break;
    }

    return true;
}

void ZahlStruct::update() {
    uint32_t timeSinceFed = (millis() - lastFed) / 1000;
    if (timeSinceFed > 60) {
        uint8_t decay = (timeSinceFed - 60) / 30;
        // corruption slows hunger decay
        uint8_t decayMod = (corruption > 50) ? max(1, (int)decay / 2) : decay;
        if (decayMod > 0 && hunger > 0) {
            hunger   = (hunger >= decayMod) ? hunger - decayMod : 0;
            lastFed  = millis();
        }
    }
    if (hunger == 0) {
        feralMode  = true;
        corrosion  = 100;
    } else if (hunger > 20) {
        feralMode  = false;
    }
    if (corrosion > 100) corrosion = 100;

    // Entropy slow decay
    static uint32_t lastEntropyDecay = 0;
    if (millis() - lastEntropyDecay > 60000 && entropy > 0) {
        entropy--;
        lastEntropyDecay = millis();
    }
}

void updateZahlState() {
    static uint32_t lastUpdate = 0;
    if (millis() - lastUpdate < 5000) return;
    lastUpdate = millis();
    Zahl.update();

    // End timed rituals
    if (ritualActive && millis() > ritualExpiry) {
        switch (activeRitual) {
            case RITUAL_CHANNEL_BURST:
                setHopInterval(200);
                break;
            case RITUAL_BEACON_STORM:
            case RITUAL_HAUNT:
                setBeaconSpamEnabled(false);
                break;
            case RITUAL_PACKET_STORM:
                setHopInterval(200);
                setBeaconSpamEnabled(false);
                break;
        }
        ritualActive = false;
        activeRitual = 0xFF;
        Serial.println("Ritual expired");
    }
}

String getZahlStatus() {
    String s = "";
    s += "hunger:"     + String(Zahl.hunger)      + ",";
    s += "level:"      + String(Zahl.level)        + ",";
    s += "corrosion:"  + String(Zahl.corrosion)    + ",";
    s += "entropy:"    + String(Zahl.entropy)      + ",";
    s += "corruption:" + String(Zahl.corruption)   + ",";
    s += "souls:"      + String(Zahl.soulBalance)  + ",";
    s += "feral:"      + String(feralMode ? "1" : "0") + ",";
    s += "exp:"        + String(Zahl.experience);
    return s;
}

String getZahlJson() {
    String j = "{";
    j += "\"hunger\":"     + String(Zahl.hunger)      + ",";
    j += "\"level\":"      + String(Zahl.level)        + ",";
    j += "\"corrosion\":"  + String(Zahl.corrosion)    + ",";
    j += "\"entropy\":"    + String(Zahl.entropy)      + ",";
    j += "\"corruption\":" + String(Zahl.corruption)   + ",";
    j += "\"soulBalance\":" + String(Zahl.soulBalance) + ",";
    j += "\"soulsTotal\":" + String(Zahl.soulsTotal)   + ",";
    j += "\"feral\":\""    + String(feralMode ? "1" : "0") + "\",";
    j += "\"exp\":"        + String(Zahl.experience)   + ",";
    j += "\"ritual\":"     + String(activeRitual == 0xFF ? -1 : (int)activeRitual) + ",";
    j += "\"beacon\":"     + String(isBeaconSpamming() ? "true" : "false") + ",";
    j += "\"portal\":"     + String(isPortalActive()   ? "true" : "false") + ",";
    j += "\"hopping\":"    + String(isHopping()        ? "true" : "false");
    j += "}";
    return j;
}
