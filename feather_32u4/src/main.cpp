#include <Arduino.h>
#include "rfm69.h"

#define SYMBOL_US 160

RFM69Transceiver radio(RFM69_CS, RFM69_RST, RFM69_DIO0);

String currentKeyIdHex = "8BFA3";
float currentFreqMHz = 916.8;

static volatile unsigned long lastEdgeUs = 0;
static volatile uint16_t pulses[300];
static volatile uint16_t pulseCount = 0;
static volatile bool frameReady = false;

void dio0InterruptHandler() {
    unsigned long now = micros();
    unsigned long dur = now - lastEdgeUs;
    lastEdgeUs = now;

    if (dur > 3000) {
        pulseCount = 0;
    } else if (pulseCount < 300 && !frameReady) {
        pulses[pulseCount++] = (uint16_t)dur;
        if (pulseCount >= 288) {
            frameReady = true;
        }
    }
}

String buildFrameHex(uint32_t keyId, uint8_t secretKnock, uint8_t alert, uint8_t lowbat, uint8_t relay, uint8_t deviceType) {
    uint8_t bits[48] = {0};

    for (int i = 0; i < 32; i++) {
        bits[i] = (keyId >> (31 - i)) & 1;
    }

    bits[34] = (deviceType >> 1) & 1;
    bits[35] = deviceType & 1;
    bits[36] = 1;
    bits[37] = 0;
    bits[38] = (alert >> 1) & 1;
    bits[39] = alert & 1;
    bits[43] = secretKnock & 1;
    bits[44] = relay & 1;
    bits[46] = lowbat & 1;

    int ones = 0;
    for (int i = 0; i < 47; i++) {
        if (bits[i]) ones++;
    }
    bits[47] = ones % 2;

    String hexStr = "";
    for (int b = 0; b < 6; b++) {
        uint8_t val = 0;
        for (int k = 0; k < 8; k++) {
            val = (val << 1) | bits[b * 8 + k];
        }
        if (val < 16) hexStr += "0";
        hexStr += String(val, HEX);
    }
    hexStr.toUpperCase();
    return hexStr;
}

void sendBit(bool bit) {
    if (bit) {
        digitalWrite(RFM69_DIO0, HIGH);
        delayMicroseconds(SYMBOL_US * 2);
        digitalWrite(RFM69_DIO0, LOW);
        delayMicroseconds(SYMBOL_US);
    } else {
        digitalWrite(RFM69_DIO0, HIGH);
        delayMicroseconds(SYMBOL_US);
        digitalWrite(RFM69_DIO0, LOW);
        delayMicroseconds(SYMBOL_US * 2);
    }
}

void transmitBurst(const String& frameHex, uint16_t burstCount = 50) {
    detachInterrupt(digitalPinToInterrupt(RFM69_DIO0));
    radio.setMode(MODE_TX);
    pinMode(RFM69_DIO0, OUTPUT);

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
        // Preamble
        digitalWrite(RFM69_DIO0, LOW);
        delayMicroseconds(SYMBOL_US * 3);

        for (int i = 0; i < 48; i++) {
            sendBit(bits[i]);
        }

        // Postamble
        digitalWrite(RFM69_DIO0, HIGH);
        delayMicroseconds(SYMBOL_US * 3);
        digitalWrite(RFM69_DIO0, LOW);
    }

    delayMicroseconds(2000);
    radio.setMode(MODE_RX);
    pinMode(RFM69_DIO0, INPUT);
    attachInterrupt(digitalPinToInterrupt(RFM69_DIO0), dio0InterruptHandler, CHANGE);
}

void printHelp() {
    Serial.println(F("\n--- Honeywell ActivLink Commands (Feather 32u4 RFM69HCW) ---"));
    Serial.println(F("  SET_KEY <hex_id>            Set target receiver Key ID (e.g., SET_KEY 8BFA3)"));
    Serial.println(F("  TX [alert] [knock] [lowbat] Send ActivLink burst (e.g., TX 0 0 0)"));
    Serial.println(F("  SET_FREQ <freq_mhz>         Set RF frequency (e.g., SET_FREQ 916.8 or 868.3)"));
    Serial.println(F("  STATUS                      Print current status and configuration"));
    Serial.println(F("  HELP                        Display this menu\n"));
}

void processCommand(String input) {
    input.trim();
    if (input.length() == 0) return;

    int spaceIdx = input.indexOf(' ');
    String cmd = (spaceIdx == -1) ? input : input.substring(0, spaceIdx);
    cmd.toUpperCase();

    if (cmd == "HELP") {
        printHelp();
    } else if (cmd == "STATUS") {
        Serial.print(F("Current Key ID: 0x"));
        Serial.println(currentKeyIdHex);
        Serial.print(F("Current Frequency: "));
        Serial.print(currentFreqMHz);
        Serial.println(F(" MHz"));
    } else if (cmd == "SET_KEY") {
        if (spaceIdx != -1) {
            currentKeyIdHex = input.substring(spaceIdx + 1);
            currentKeyIdHex.trim();
            currentKeyIdHex.toUpperCase();
            Serial.print(F("Key ID updated to: 0x"));
            Serial.println(currentKeyIdHex);
        } else {
            Serial.println(F("Error: Missing hex_id argument. Example: SET_KEY 8BFA3"));
        }
    } else if (cmd == "SET_FREQ") {
        if (spaceIdx != -1) {
            float f = input.substring(spaceIdx + 1).toFloat();
            if (f > 800.0 && f < 1000.0) {
                currentFreqMHz = f;
                radio.setFrequency(currentFreqMHz);
                Serial.print(F("Frequency updated to: "));
                Serial.print(currentFreqMHz);
                Serial.println(F(" MHz"));
            } else {
                Serial.println(F("Error: Invalid frequency range. Use 916.8 or 868.3"));
            }
        }
    } else if (cmd == "TX") {
        uint8_t alert = 0, knock = 0, lowbat = 0;
        if (spaceIdx != -1) {
            String args = input.substring(spaceIdx + 1);
            sscanf(args.c_str(), "%hhu %hhu %hhu", &alert, &knock, &lowbat);
        }
        uint32_t keyId = strtoul(currentKeyIdHex.c_str(), NULL, 16);
        String frameHex = buildFrameHex(keyId, knock, alert, lowbat, 0, 2);

        Serial.print(F("Transmitting ActivLink Frame: "));
        Serial.print(frameHex);
        Serial.print(F(" (Key: 0x"));
        Serial.print(currentKeyIdHex);
        Serial.println(F(")..."));

        transmitBurst(frameHex, 50);
        Serial.println(F("Transmission complete. Listening for incoming signals..."));
    } else {
        Serial.print(F("Unknown command: "));
        Serial.println(cmd);
        printHelp();
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {}

    Serial.println(F("\n============================================"));
    Serial.println(F(" Adafruit Feather 32u4 RFM69HCW ActivLink "));
    Serial.println(F("============================================"));

    if (!radio.begin(currentFreqMHz)) {
        Serial.println(F("ERROR: RFM69HCW radio initialization failed over SPI!"));
    } else {
        Serial.print(F("RFM69HCW initialized successfully at "));
        Serial.print(currentFreqMHz);
        Serial.println(F(" MHz."));
    }

    pinMode(RFM69_DIO0, INPUT);
    attachInterrupt(digitalPinToInterrupt(RFM69_DIO0), dio0InterruptHandler, CHANGE);

    printHelp();
    Serial.print(F("ActivLink> "));
}

void checkIncomingSignal() {
    if (!frameReady) return;

    String bits = "";
    for (int i = 0; i < pulseCount - 1; i += 2) {
        bits += (pulses[i] > 220) ? "1" : "0";
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

        Serial.println();
        Serial.print(F("[RX] Decoded ActivLink Frame: 0x"));
        Serial.println(hexStr);
        Serial.print(F("ActivLink> "));
    }
}

void loop() {
    checkIncomingSignal();

    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        processCommand(input);
        Serial.print(F("ActivLink> "));
    }
}
