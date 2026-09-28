#ifndef ACTIVLINK_H
#define ACTIVLINK_H

#include <Arduino.h>
#include <vector>
#include "cc1101.h"

struct ActivLinkPacket {
    String frameHex;
    String keyIdHex;
    uint8_t deviceType;
    String deviceTypeStr;
    uint8_t alert;
    uint8_t secretKnock;
    uint8_t relay;
    uint8_t lowbat;
    uint8_t parity;
    bool parityValid;
    unsigned long timestamp;
};

class ActivLinkEngine {
public:
    ActivLinkEngine(CC1101Transceiver& radio, uint8_t txRxPin = CC1101_GDO0);

    void begin();
    String buildFrameHex(uint32_t keyId, uint8_t secretKnock = 0, uint8_t alert = 0, uint8_t lowbat = 0, uint8_t relay = 0, uint8_t deviceType = 2);
    void transmitFrame(const String& frameHex, uint16_t burstCount = 50);

    static ActivLinkPacket decodeFrameHex(const String& frameHex);
    bool checkRxPacket(ActivLinkPacket& outPacket);

private:
    CC1101Transceiver& _radio;
    uint8_t _pin;

    void sendBit(bool bit);
    void sendPreamble();
    void sendPostamble();
};

#endif
