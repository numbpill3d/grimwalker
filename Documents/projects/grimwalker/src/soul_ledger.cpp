#include "soul_ledger.h"
#include "zahl_pet.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

Soul souls[MAX_SOULS];
int  soulCount       = 0;
bool pendingSoulSave = false;

static const char* SOUL_FILE = "/souls.json";

static const char* journalLines[4][3] = {
    { "A wandering signal, now bound.",   "Its packets echo in the void.",      "Feed." },
    { "The handshake broke like bone.",   "Keys laid bare before the Daedroth.", "Consume." },
    { "They typed their truth into maw.", "The portal swallowed it whole.",      "Savor." },
    { "A ghost of bluetooth, traced.",    "Frequencies bleed into Zahl.",        "Devour." },
};

void initSoulLedger() {
    soulCount       = 0;
    pendingSoulSave = false;

    if (!SPIFFS.exists(SOUL_FILE)) return;

    File f = SPIFFS.open(SOUL_FILE, "r");
    if (!f) return;

    StaticJsonDocument<8192> doc;
    if (deserializeJson(doc, f) == DeserializationError::Ok) {
        JsonArray arr = doc["souls"].as<JsonArray>();
        for (JsonObject s : arr) {
            if (soulCount >= MAX_SOULS) break;
            strlcpy(souls[soulCount].id,    s["id"]    | "", sizeof(souls[0].id));
            strlcpy(souls[soulCount].label, s["label"] | "", sizeof(souls[0].label));
            souls[soulCount].type       = s["type"]  | 0;
            souls[soulCount].rssi       = s["rssi"]  | 0;
            souls[soulCount].capturedAt = s["at"]    | 0;
            soulCount++;
        }
    }
    f.close();
    Serial.printf("Soul ledger loaded: %d souls\n", soulCount);
}

bool soulKnown(const char* id) {
    for (int i = 0; i < soulCount; i++)
        if (strncmp(souls[i].id, id, 19) == 0) return true;
    return false;
}

bool addSoul(const char* id, const char* label, uint8_t type, int8_t rssi) {
    if (soulKnown(id)) return false;
    if (soulCount >= MAX_SOULS) {
        // evict oldest
        memmove(souls, souls + 1, sizeof(Soul) * (MAX_SOULS - 1));
        soulCount = MAX_SOULS - 1;
    }
    strlcpy(souls[soulCount].id,    id,    sizeof(souls[0].id));
    strlcpy(souls[soulCount].label, label, sizeof(souls[0].label));
    souls[soulCount].type       = type;
    souls[soulCount].rssi       = rssi;
    souls[soulCount].capturedAt = millis();
    soulCount++;
    pendingSoulSave = true;
    Serial.printf("Soul: %s [%s] t=%d\n", id, label, type);
    return true;
}

void saveSouls() {
    File f = SPIFFS.open(SOUL_FILE, "w");
    if (!f) return;
    f.print("{\"souls\":[");
    for (int i = 0; i < soulCount; i++) {
        if (i > 0) f.print(",");
        f.printf("{\"id\":\"%s\",\"label\":\"%s\",\"type\":%d,\"rssi\":%d,\"at\":%lu}",
            souls[i].id, souls[i].label, souls[i].type, souls[i].rssi, souls[i].capturedAt);
    }
    f.print("]}");
    f.close();
    pendingSoulSave = false;
}

void tickSoulLedger() {
    static uint32_t lastSave = 0;
    if (pendingSoulSave && millis() - lastSave > 30000) {
        saveSouls();
        lastSave = millis();
    }
}

String getSoulsJson(int limit) {
    String out = "{\"count\":" + String(soulCount) + ",\"souls\":[";
    int start = max(0, soulCount - limit);
    bool first = true;
    for (int i = soulCount - 1; i >= start; i--) {
        if (!first) out += ",";
        first = false;
        out += "{\"id\":\"";   out += souls[i].id;
        out += "\",\"label\":\""; out += souls[i].label;
        out += "\",\"type\":"; out += souls[i].type;
        out += ",\"rssi\":";   out += souls[i].rssi;
        out += "}";
    }
    out += "]}";
    return out;
}

String getJournalEntry(int idx) {
    if (idx < 0 || idx >= soulCount) return "The void is silent.";
    const char** lines = journalLines[souls[idx].type % 4];
    uint8_t pick = (souls[idx].capturedAt / 1000) % 3;
    String e = String(lines[pick]) + " [" + String(souls[idx].id) + "]";
    return e;
}
