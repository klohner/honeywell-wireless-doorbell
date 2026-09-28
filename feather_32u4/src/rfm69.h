#ifndef RFM69_H
#define RFM69_H

#include <Arduino.h>
#include <SPI.h>

// Adafruit Feather 32u4 RFM69HCW Pin Definitions
#define RFM69_CS   8
#define RFM69_RST  4
#define RFM69_DIO0 7

// RFM69 Registers
#define REG_FIFO            0x00
#define REG_OPMODE          0x01
#define REG_DATAMODUL       0x02
#define REG_BITRATEMSB      0x03
#define REG_BITRATELSB      0x04
#define REG_FDEVMSB         0x05
#define REG_FDEVLSB         0x06
#define REG_FRFMSB          0x07
#define REG_FRFMID          0x08
#define REG_FRFLSB          0x09
#define REG_RXBW            0x19
#define REG_AFCBW           0x1A
#define REG_DIOMAPPING1     0x25
#define REG_IRQFLAGS2       0x28
#define REG_RSSIVAL         0x24
#define REG_SYNCCONFIG      0x2E
#define REG_PACKETCONFIG1   0x37
#define REG_PAYLOADLEN      0x38
#define REG_FIFOTHRESH      0x3C
#define REG_TESTPA1         0x5A
#define REG_TESTPA2         0x5C
#define REG_VERSION         0x42

// Modes
#define MODE_SLEEP          0x00
#define MODE_STDBY          0x04
#define MODE_TX             0x0C
#define MODE_RX             0x10

class RFM69Transceiver {
public:
    RFM69Transceiver(uint8_t csPin = RFM69_CS, uint8_t rstPin = RFM69_RST, uint8_t dio0Pin = RFM69_DIO0);
    bool begin(float freqMHz = 916.8);
    void setFrequency(float freqMHz);
    void setMode(uint8_t mode);
    void writeReg(uint8_t reg, uint8_t val);
    uint8_t readReg(uint8_t reg);

private:
    uint8_t _csPin;
    uint8_t _rstPin;
    uint8_t _dio0Pin;
    float _freqMHz;
    void select();
    void deselect();
};

#endif
