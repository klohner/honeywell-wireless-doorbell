#include "rfm69.h"

RFM69Transceiver::RFM69Transceiver(uint8_t csPin, uint8_t rstPin, uint8_t dio0Pin)
    : _csPin(csPin), _rstPin(rstPin), _dio0Pin(dio0Pin), _freqMHz(916.8) {}

void RFM69Transceiver::select() {
    digitalWrite(_csPin, LOW);
}

void RFM69Transceiver::deselect() {
    digitalWrite(_csPin, HIGH);
}

void RFM69Transceiver::writeReg(uint8_t reg, uint8_t val) {
    select();
    SPI.transfer(reg | 0x80);
    SPI.transfer(val);
    deselect();
}

uint8_t RFM69Transceiver::readReg(uint8_t reg) {
    select();
    SPI.transfer(reg & 0x7F);
    uint8_t val = SPI.transfer(0x00);
    deselect();
    return val;
}

bool RFM69Transceiver::begin(float freqMHz) {
    pinMode(_csPin, OUTPUT);
    deselect();
    pinMode(_rstPin, OUTPUT);

    // Hard reset RFM69
    digitalWrite(_rstPin, HIGH);
    delay(10);
    digitalWrite(_rstPin, LOW);
    delay(10);

    SPI.begin();

    // Verify SPI communication
    uint8_t version = readReg(REG_VERSION);
    if (version == 0x00 || version == 0xFF) {
        return false;
    }

    setMode(MODE_STDBY);

    // Continuous 2FSK modulation for Honeywell ActivLink
    writeReg(REG_DATAMODUL, 0x60);     # Continuous mode without bit sync, 2FSK
    writeReg(REG_BITRATEMSB, 0x14);    # 6250 baud rate
    writeReg(REG_BITRATELSB, 0x00);
    writeReg(REG_FDEVMSB, 0x03);       # ±50 kHz frequency deviation
    writeReg(REG_FDEVLSB, 0x33);
    writeReg(REG_RXBW, 0x42);          # RX filter bandwidth ~100 kHz
    writeReg(REG_SYNCCONFIG, 0x00);    # Sync off (continuous raw PWM stream)
    writeReg(REG_PACKETCONFIG1, 0x00); # Fixed length, no CRC/addressing

    setFrequency(freqMHz);
    setMode(MODE_RX);
    return true;
}

void RFM69Transceiver::setFrequency(float freqMHz) {
    _freqMHz = freqMHz;
    uint32_t frf = (uint32_t)((freqMHz * 1000000.0) / 61.03515625);
    writeReg(REG_FRFMSB, (frf >> 16) & 0xFF);
    writeReg(REG_FRFMID, (frf >> 8) & 0xFF);
    writeReg(REG_FRFLSB, frf & 0xFF);
}

void RFM69Transceiver::setMode(uint8_t mode) {
    writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | mode);
    while ((readReg(REG_IRQFLAGS2) & 0x80) == 0) {} // Wait for ModeReady
}
