#include <Arduino.h>
#include <WiFi.h>
#include "cc1101.h"
#include "activlink.h"
#include "webserver.h"

CC1101Transceiver radio;
ActivLinkEngine engine(radio);
ActivLinkWebServer webServer(engine, radio);

const char* AP_SSID = "Honeywell-ActivLink-AP";
const char* AP_PASS = "activlink123";

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- Honeywell ActivLink ESP32 Gateway ---");

    // Initialize CC1101 Transceiver
    if (!radio.begin(916.8)) {
        Serial.println("ERROR: Failed to initialize CC1101 module via SPI!");
    } else {
        Serial.println("CC1101 initialized successfully at 916.8 MHz.");
    }

    engine.begin();

    // WiFi SoftAP or Station setup
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("Access Point started. SSID: ");
    Serial.println(AP_SSID);
    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP());

    webServer.begin();
    Serial.println("Web server started successfully.");
}

void loop() {
    ActivLinkPacket rxPkt;
    if (engine.checkRxPacket(rxPkt)) {
        Serial.print("Received ActivLink Frame: ");
        Serial.println(rxPkt.frameHex);
        webServer.broadcastPacket(rxPkt);
    }
    delay(10);
}
