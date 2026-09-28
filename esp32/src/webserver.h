#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include "activlink.h"
#include "cc1101.h"

class ActivLinkWebServer {
public:
    ActivLinkWebServer(ActivLinkEngine& engine, CC1101Transceiver& radio, uint16_t port = 80);
    void begin();
    void broadcastPacket(const ActivLinkPacket& packet);

private:
    AsyncWebServer _server;
    AsyncEventSource _events;
    ActivLinkEngine& _engine;
    CC1101Transceiver& _radio;

    void setupRoutes();
};

#endif
