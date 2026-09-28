#include "cc1101.h"

CC1101Transceiver::CC1101Transceiver(uint8_t csnPin, uint8_t gdo0Pin)
    : _csnPin(csnPin), _gdo0Pin(gdo0Pin), _currentFreq(916.8) {}

void CC1101Transceiver::select() {
    digitalWrite(_csnPin, LOW);
}

void CC1101Transceiver::deselect() {
    digitalWrite(_csnPin, HIGH);
}

void CC1101Transceiver::strobe(uint8_t cmd) {
    select();
    SPI.transfer(cmd);
    deselect();
}

void CC1101Transceiver::writeReg(uint8_t reg, uint8_t value) {
    select();
    SPI.transfer(reg);
    SPI.transfer(value);
    deselect();
}

uint8_t CC1101Transceiver::readReg(uint8_t reg) {
    select();
    SPI.transfer(reg | 0x80);
    uint8_t val = SPI.transfer(0x00);
    deselect();
    return val;
}

bool CC1101Transceiver::begin(float freqMHz) {
    pinMode(_csnPin, OUTPUT);
    deselect();

    SPI.begin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_CSN);

    strobe(CC1101_SRES);
    delay(10);

    // Verify SPI communication
    uint8_t version = readReg(CC1101_VERSION);
    if (version == 0x00 || version == 0xFF) {
        return false; // Communication failure
    }

    setFrequency(freqMHz);

    // Asynchronous / transparent mode configuration for GDO0
    writeReg(CC1101_IOCFG0, 0x0D);   // GDO0 output: Async Serial Data Output / Input
    writeReg(CC1101_IOCFG2, 0x29);   // GDO2 output: CHP_RDY_N
    writeReg(CC1101_FIFOTHR, 0x07);  // TX/RX FIFO thresholds
    writeReg(CC1101_PKTCTRL0, 0x32); // Asynchronous serial mode, unformatted
    writeReg(CC1101_FSCTRL1, 0x06);  // IF frequency
    writeReg(CC1101_MDMCFG4, 0xF8);  // RX filter bandwidth & exponent
    writeReg(CC1101_MDMCFG3, 0x83);  // 6250 baud rate
    writeReg(CC1101_MDMCFG2, 0x00);  // 2FSK, no sync word
    writeReg(CC1101_DEVIATN, 0x47);  // ±50 kHz frequency deviation
    writeReg(CC1101_MCSM0, 0x18);    // Auto calibrate on IDLE->RX/TX transition
    writeReg(CC1101_FOCCFG, 0x16);   // Frequency offset compensation
    writeReg(CC1101_BSCFG, 0x6C);    // Bit synchronization configuration
    writeReg(CC1101_AGCCTRL2, 0x43); // AGC settings
    writeReg(CC1101_FREND0, 0x10);   // Front end TX configuration

    setRxMode();
    return true;
}

void CC1101Transceiver::setFrequency(float freqMHz) {
    _currentFreq = freqMHz;
    uint32_t freqReg = (uint32_t)((freqMHz * 1000000.0) / (26000000.0 / 65536.0));

    uint8_t freq2 = (freqReg >> 16) & 0xFF;
    uint8_t freq1 = (freqReg >> 8) & 0xFF;
    uint8_t freq0 = freqReg & 0xFF;

    strobe(CC1101_SIDLE);
    writeReg(CC1101_FREQ2, freq2);
    writeReg(CC1101_FREQ1, freq1);
    writeReg(CC1101_FREQ0, freq0);
    strobe(CC1101_SCAL);
    delay(2);
}

void CC1101Transceiver::setTxMode() {
    strobe(CC1101_SIDLE);
    strobe(CC1101_SFTX);
    strobe(CC1101_STX);
}

void CC1101Transceiver::setRxMode() {
    strobe(CC1101_SIDLE);
    strobe(CC1101_SFRX);
    strobe(CC1101_SRX);
}

void CC1101Transceiver::setIdleMode() {
    strobe(CC1101_SIDLE);
}
