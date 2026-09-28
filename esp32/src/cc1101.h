#ifndef CC1101_H
#define CC1101_H

#include <Arduino.h>
#include <SPI.h>

// SPI Configuration Defaults
#define CC1101_SCK   18
#define CC1101_MISO  19
#define CC1101_MOSI  23
#define CC1101_CSN   5
#define CC1101_GDO0  2
#define CC1101_GDO2  4

// CC1101 Register Addresses
#define CC1101_IOCFG2       0x00
#define CC1101_IOCFG0       0x02
#define CC1101_FIFOTHR      0x03
#define CC1101_PKTLEN       0x06
#define CC1101_PKTCTRL1     0x07
#define CC1101_PKTCTRL0     0x08
#define CC1101_ADDR         0x09
#define CC1101_CHANNR       0x0A
#define CC1101_FSCTRL1      0x0B
#define CC1101_FSCTRL0      0x0C
#define CC1101_FREQ2        0x0D
#define CC1101_FREQ1        0x0E
#define CC1101_FREQ0        0x0F
#define CC1101_MDMCFG4      0x10
#define CC1101_MDMCFG3      0x11
#define CC1101_MDMCFG2      0x12
#define CC1101_MDMCFG1      0x13
#define CC1101_MDMCFG0      0x14
#define CC1101_DEVIATN      0x15
#define CC1101_MCSM1        0x17
#define CC1101_MCSM0        0x18
#define CC1101_FOCCFG       0x19
#define CC1101_BSCFG        0x1A
#define CC1101_AGCCTRL2     0x1B
#define CC1101_AGCCTRL1     0x1C
#define CC1101_AGCCTRL0     0x1D
#define CC1101_FREND1       0x21
#define CC1101_FREND0       0x22
#define CC1101_FSCAL3       0x23
#define CC1101_FSCAL2       0x24
#define CC1101_FSCAL1       0x25
#define CC1101_FSCAL0       0x26
#define CC1101_TEST2        0x2C
#define CC1101_TEST1        0x2D
#define CC1101_TEST0        0x2E

// CC1101 Command Strobes
#define CC1101_SRES         0x30
#define CC1101_SFSTXON      0x31
#define CC1101_SXOFF        0x32
#define CC1101_SCAL         0x33
#define CC1101_SRX          0x34
#define CC1101_STX          0x35
#define CC1101_SIDLE        0x36
#define CC1101_SFRX         0x3A
#define CC1101_SFTX         0x3B
#define CC1101_SNOP         0x3D

#define CC1101_PARTNUM      0x30
#define CC1101_VERSION      0x31

class CC1101Transceiver {
public:
    CC1101Transceiver(uint8_t csnPin = CC1101_CSN, uint8_t gdo0Pin = CC1101_GDO0);
    bool begin(float freqMHz = 916.8);
    void setFrequency(float freqMHz);
    void setTxMode();
    void setRxMode();
    void setIdleMode();
    void writeReg(uint8_t reg, uint8_t value);
    uint8_t readReg(uint8_t reg);
    void strobe(uint8_t cmd);

private:
    uint8_t _csnPin;
    uint8_t _gdo0Pin;
    float _currentFreq;
    void select();
    void deselect();
};

#endif
