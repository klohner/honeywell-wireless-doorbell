#include "activlink.h"

#define SYMBOL_US 160

static volatile unsigned long lastInterruptUs = 0;
static volatile uint16_t pulseBuffer[300];
static volatile uint16_t pulseCount = 0;
static volatile bool frameReady = false;

void IRAM_ATTR gdo0InterruptHandler() {
    unsigned long now = micros();
    unsigned long duration = now - lastInterruptUs;
    lastInterruptUs = now;

    if (duration > 3000) {
        // Gap detected, reset buffer
        pulseCount = 0;
    } else if (pulseCount < 300 && !frameReady) {
        pulseBuffer[pulseCount++] = (uint16_t)duration;
        if (pulseCount >= 144 * 2) {
            frameReady = true;
        }
    }
}

ActivLinkEngine::ActivLinkEngine(CC1101Transceiver& radio, uint8_t txRxPin)
    : _radio(radio), _pin(txRxPin) {}

void ActivLinkEngine::begin() {
    pinMode(_pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(_pin), gdo0InterruptHandler, CHANGE);
}

void ActivLinkEngine::sendBit(bool bit) {
    if (bit) {
        // Bit 1: HIGH-HIGH-LOW (320 us HIGH, 160 us LOW)
        digitalWrite(_pin, HIGH);
        delayMicroseconds(SYMBOL_US * 2);
        digitalWrite(_pin, LOW);
        delayMicroseconds(SYMBOL_US);
    } else {
        // Bit 0: HIGH-LOW-LOW (160 us HIGH, 320 us LOW)
        digitalWrite(_pin, HIGH);
        delayMicroseconds(SYMBOL_US);
        digitalWrite(_pin, LOW);
        delayMicroseconds(SYMBOL_US * 2);
    }
}

void ActivLinkEngine::sendPreamble() {
    // Preamble: LOW-LOW-LOW (480 us LOW)
    digitalWrite(_pin, LOW);
    delayMicroseconds(SYMBOL_US * 3);
}

void ActivLinkEngine::sendPostamble() {
    // Postamble: HIGH-HIGH-HIGH (480 us HIGH)
    digitalWrite(_pin, HIGH);
    delayMicroseconds(SYMBOL_US * 3);
    digitalWrite(_pin, LOW);
}

String ActivLinkEngine::buildFrameHex(uint32_t keyId, uint8_t secretKnock, uint8_t alert, uint8_t lowbat, uint8_t relay, uint8_t deviceType) {
    uint8_t bits[48] = {0};

    // Bits 0..31: Key ID
    for (int i = 0; i < 32; i++) {
        bits[i] = (keyId >> (31 - i)) & 1;
    }

    // Bits 34..35: Device Type (0b10 for doorbell)
    bits[34] = (deviceType >> 1) & 1;
    bits[35] = deviceType & 1;

    // Bits 36..37: Constant padding (0b10)
    bits[36] = 1;
    bits[37] = 0;

    // Bits 38..39: Alert mode
    bits[38] = (alert >> 1) & 1;
    bits[39] = alert & 1;

    // Bit 43: Secret knock
    bits[43] = secretKnock & 1;

    // Bit 44: Relay
    bits[44] = relay & 1;

    // Bit 46: Low battery
    bits[46] = lowbat & 1;

    // Calculate parity over first 47 bits
    int onesCount = 0;
    for (int i = 0; i < 47; i++) {
        if (bits[i]) onesCount++;
    }
    bits[47] = onesCount % 2;

    // Convert bits array to Hex string
    String hexStr = "";
    for (int byteIdx = 0; byteIdx < 6; byteIdx++) {
        uint8_t byteVal = 0;
        for (int bitIdx = 0; bitIdx < 8; bitIdx++) {
            byteVal = (byteVal << 1) | bits[byteIdx * 8 + bitIdx];
        }
        if (byteVal < 16) hexStr += "0";
        hexStr += String(byteVal, HEX);
    }
    hexStr.toUpperCase();
    return hexStr;
}

void ActivLinkEngine::transmitFrame(const String& frameHex, uint16_t burstCount) {
    detachInterrupt(digitalPinToInterrupt(_pin));
    _radio.setTxMode();
    pinMode(_pin, OUTPUT);

    // Convert frameHex to 48 bits
    uint8_t bits[48] = {0};
    for (int i = 0; i < 12 && i < frameHex.length(); i++) {
        char c = frameHex.charAt(i);
        uint8_t val = (c >= '0' && c <= '9') ? (c - '0') :
                      (c >= 'A' && c <= 'F') ? (c - 'A' + 10) :
                      (c >= 'a' && c <= 'f') ? (c - 'a' + 10) : 0;
        bits[i * 4 + 0] = (val >> 3) & 1;
        bits[i * 4 + 1] = (val >> 2) & 1;
        bits[i * 4 + 2] = (val >> 1) & 1;
        bits[i * 4 + 3] = val & 1;
    }

    for (uint16_t b = 0; b < burstCount; b++) {
        sendPreamble();
        for (int i = 0; i < 48; i++) {
            sendBit(bits[i]);
        }
        sendPostamble();
    }

    // Inter-frame gap
    digitalWrite(_pin, LOW);
    delayMicroseconds(2000);

    _radio.setRxMode();
    pinMode(_pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(_pin), gdo0InterruptHandler, CHANGE);
}

ActivLinkPacket ActivLinkEngine::decodeFrameHex(const String& frameHex) {
    ActivLinkPacket pkt;
    pkt.frameHex = frameHex;
    pkt.timestamp = millis();

    uint8_t bits[48] = {0};
    for (int i = 0; i < 12 && i < frameHex.length(); i++) {
        char c = frameHex.charAt(i);
        uint8_t val = (c >= '0' && c <= '9') ? (c - '0') :
                      (c >= 'A' && c <= 'F') ? (c - 'A' + 10) :
                      (c >= 'a' && c <= 'f') ? (c - 'a' + 10) : 0;
        bits[i * 4 + 0] = (val >> 3) & 1;
        bits[i * 4 + 1] = (val >> 2) & 1;
        bits[i * 4 + 2] = (val >> 1) & 1;
        bits[i * 4 + 3] = val & 1;
    }

    uint32_t keyId = 0;
    for (int i = 0; i < 32; i++) {
        keyId = (keyId << 1) | bits[i];
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%08X", keyId);
    pkt.keyIdHex = String(buf);

    pkt.deviceType = (bits[34] << 1) | bits[35];
    if (pkt.deviceType == 2) {
        pkt.deviceTypeStr = "Doorbell / Push Button";
    } else if (pkt.deviceType == 1) {
        pkt.deviceTypeStr = "PIR Sensor";
    } else {
        pkt.deviceTypeStr = "Unknown (" + String(pkt.deviceType) + ")";
    }

    pkt.alert = (bits[38] << 1) | bits[39];
    pkt.secretKnock = bits[43];
    pkt.relay = bits[44];
    pkt.lowbat = bits[46];
    pkt.parity = bits[47];

    int onesCount = 0;
    for (int i = 0; i < 47; i++) {
        if (bits[i]) onesCount++;
    }
    pkt.parityValid = (pkt.parity == (onesCount % 2));

    return pkt;
}

bool ActivLinkEngine::checkRxPacket(ActivLinkPacket& outPacket) {
    if (!frameReady) return false;

    // Decode pulse durations into bit string
    String bits = "";
    for (int i = 0; i < pulseCount - 1; i += 2) {
        uint16_t highDur = pulseBuffer[i];
        if (highDur > 220) {
            bits += "1";
        } else {
            bits += "0";
        }
    }

    pulseCount = 0;
    frameReady = false;

    if (bits.length() >= 48) {
        String frameBits = bits.substring(0, 48);
        String hexStr = "";
        for (int b = 0; b < 48; b += 4) {
            uint8_t val = 0;
            for (int k = 0; k < 4; k++) {
                val = (val << 1) | (frameBits.charAt(b + k) == '1' ? 1 : 0);
            }
            hexStr += String(val, HEX);
        }
        hexStr.toUpperCase();
        outPacket = decodeFrameHex(hexStr);
        return true;
    }

    return false;
}
