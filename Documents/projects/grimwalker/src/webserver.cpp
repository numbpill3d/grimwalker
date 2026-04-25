#include "webserver.h"
#include "zahl_pet.h"
#include "packet_sniffer.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);
AsyncEventSource events("/events");

// log buffers declared in packet_sniffer.h via extern

void handleAPI(AsyncWebServerRequest *request) {
    if (!request->hasParam("action")) {
        request->send(400, "application/json", "{\"error\":\"missing action\"}");
        return;
    }
    String action = request->getParam("action")->value();

    if (action == "feed") {
        if (!request->hasParam("type")) {
            request->send(400, "application/json", "{\"error\":\"missing type\"}");
            return;
        }
        String type = request->getParam("type")->value();
        if (type == "snack") {
            Zahl.feed(5);
        } else if (type == "hash") {
            Zahl.feed(15);
            Zahl.addExperience(5);
        } else if (type == "deauth") {
            Zahl.feed(2);
            Zahl.corrosion += 2;
        }
        
        String response = "{\"status\":\"fed\",\"hunger\":" + String(Zahl.hunger) + "}";
        request->send(200, "application/json", response);
        
    } else if (action == "pet") {
        Zahl.pet();
        request->send(200, "application/json", "{\"status\":\"petted\"}");
        
    } else if (action == "deauth_target") {
        if (!request->hasParam("mac")) {
            request->send(400, "application/json", "{\"error\":\"missing mac\"}");
            return;
        }
        String mac = request->getParam("mac")->value();
        // Parse MAC string to bytes
        uint8_t targetMac[6];
        sscanf(mac.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
               &targetMac[0], &targetMac[1], &targetMac[2],
               &targetMac[3], &targetMac[4], &targetMac[5]);
        
        // Use router MAC (broadcast for demo)
        uint8_t routerMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        sendDeauthFrame(targetMac, routerMac);
        
        request->send(200, "application/json", "{\"status\":\"deauth_sent\"}");
        
    } else {
        request->send(400, "application/json", "{\"error\":\"unknown action\"}");
    }
}

void initWebServer() {
    // Initialize SPIFFS
    if(!SPIFFS.begin(true)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    
    // Serve static files
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    server.serveStatic("/style.css", SPIFFS, "/style.css");
    server.serveStatic("/script.js", SPIFFS, "/script.js");
    
    // API endpoints
    server.on("/api", HTTP_GET, handleAPI);
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{\"zahl\":" + getZahlJson() + "}";
        request->send(200, "application/json", json);
    });
    
    server.on("/captures", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{\"captures\":[";
        for(int i = 0; i < min(10, captureLogIndex); i++) {
            int idx = (captureLogIndex - 1 - i) % MAX_LOG_ENTRIES;
            if(idx < 0) idx += MAX_LOG_ENTRIES;
            if(i > 0) json += ",";
            json += "{\"mac\":\"" + String(captureLog[idx].data) + "\",";
            json += "\"rssi\":" + String(captureLog[idx].rssi) + "}";
        }
        json += "],\"networks\":[";
        for(int i = 0; i < min(10, networkLogIndex); i++) {
            int idx = (networkLogIndex - 1 - i) % MAX_LOG_ENTRIES;
            if(idx < 0) idx += MAX_LOG_ENTRIES;
            if(i > 0) json += ",";
            json += "{\"ssid\":\"" + String(networkLog[idx].data) + "\",";
            json += "\"rssi\":" + String(networkLog[idx].rssi) + "}";
        }
        json += "]}";
        request->send(200, "application/json", json);
    });
    
    // Event source for real-time updates
    events.onConnect([](AsyncEventSourceClient *client){
        if(client->lastId()){
            Serial.printf("Client reconnected! Last ID: %u\n", client->lastId());
        }
        client->send("hello", NULL, millis(), 1000);
    });
    server.addHandler(&events);
    
    // Start server
    server.begin();
    Serial.println("Web server started on 192.168.4.1");
    
    // Start sending events
    xTaskCreate([](void *param){
        while(1) {
            events.send(getZahlStatus().c_str(), "status", millis());
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }, "EventTask", 4096, NULL, 1, NULL);
}
