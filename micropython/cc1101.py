"""MicroPython Texas Instruments CC1101 SPI driver for 2FSK Honeywell ActivLink."""

import time
from machine import Pin, SPI

# CC1101 Registers
CC1101_IOCFG0 = 0x02
CC1101_IOCFG2 = 0x00
CC1101_FIFOTHR = 0x03
CC1101_PKTCTRL0 = 0x08
CC1101_FSCTRL1 = 0x0B
CC1101_FREQ2 = 0x0D
CC1101_FREQ1 = 0x0E
CC1101_FREQ0 = 0x0F
CC1101_MDMCFG4 = 0x10
CC1101_MDMCFG3 = 0x11
CC1101_MDMCFG2 = 0x12
CC1101_DEVIATN = 0x15
CC1101_MCSM0 = 0x18
CC1101_FOCCFG = 0x19
CC1101_BSCFG = 0x1A
CC1101_AGCCTRL2 = 0x1B
CC1101_FREND0 = 0x22

# Strobes
CC1101_SRES = 0x30
CC1101_SFSTXON = 0x31
CC1101_SCAL = 0x33
CC1101_SRX = 0x34
CC1101_STX = 0x35
CC1101_SIDLE = 0x36
CC1101_SFRX = 0x3A
CC1101_SFTX = 0x3B
CC1101_VERSION = 0x31


class CC1101:
    def __init__(self, spi_bus=1, sck=18, mosi=23, miso=19, csn=5, gdo0=2, freq_mhz=916.8):
        self.csn = Pin(csn, Pin.OUT, value=1)
        self.gdo0 = Pin(gdo0, Pin.OUT)
        self.spi = SPI(spi_bus, baudrate=5000000, polarity=0, phase=0, sck=Pin(sck), mosi=Pin(mosi), miso=Pin(miso))
        self.freq_mhz = freq_mhz

    def select(self):
        self.csn.value(0)

    def deselect(self):
        self.csn.value(1)

    def strobe(self, cmd):
        self.select()
        self.spi.write(bytes([cmd]))
        self.deselect()

    def write_reg(self, reg, val):
        self.select()
        self.spi.write(bytes([reg, val]))
        self.deselect()

    def read_reg(self, reg):
        self.select()
        self.spi.write(bytes([reg | 0x80]))
        val = self.spi.read(1)[0]
        self.deselect()
        return val

    def begin(self):
        self.strobe(CC1101_SRES)
        time.sleep_ms(10)

        version = self.read_reg(CC1101_VERSION)
        if version == 0x00 or version == 0xFF:
            return False

        self.set_frequency(self.freq_mhz)

        # Asynchronous 2FSK configuration for ActivLink 6250 baud
        self.write_reg(CC1101_IOCFG0, 0x0D)   # Async serial output/input on GDO0
        self.write_reg(CC1101_IOCFG2, 0x29)   # CHP_RDY_N on GDO2
        self.write_reg(CC1101_FIFOTHR, 0x07)
        self.write_reg(CC1101_PKTCTRL0, 0x32) # Async serial mode
        self.write_reg(CC1101_FSCTRL1, 0x06)
        self.write_reg(CC1101_MDMCFG4, 0xF8)
        self.write_reg(CC1101_MDMCFG3, 0x83)  # 6250 baud rate
        self.write_reg(CC1101_MDMCFG2, 0x00)  # 2FSK
        self.write_reg(CC1101_DEVIATN, 0x47)  # ±50 kHz deviation
        self.write_reg(CC1101_MCSM0, 0x18)    # Auto-calibrate
        self.write_reg(CC1101_FOCCFG, 0x16)
        self.write_reg(CC1101_BSCFG, 0x6C)
        self.write_reg(CC1101_AGCCTRL2, 0x43)
        self.write_reg(CC1101_FREND0, 0x10)

        self.set_rx_mode()
        return True

    def set_frequency(self, freq_mhz):
        self.freq_mhz = freq_mhz
        freq_reg = int((freq_mhz * 1000000.0) / (26000000.0 / 65536.0))
        freq2 = (freq_reg >> 16) & 0xFF
        freq1 = (freq_reg >> 8) & 0xFF
        freq0 = freq_reg & 0xFF

        self.strobe(CC1101_SIDLE)
        self.write_reg(CC1101_FREQ2, freq2)
        self.write_reg(CC1101_FREQ1, freq1)
        self.write_reg(CC1101_FREQ0, freq0)
        self.strobe(CC1101_SCAL)
        time.sleep_ms(2)

    def set_tx_mode(self):
        self.strobe(CC1101_SIDLE)
        self.strobe(CC1101_SFTX)
        self.strobe(CC1101_STX)

    def set_rx_mode(self):
        self.strobe(CC1101_SIDLE)
        self.strobe(CC1101_SFRX)
        self.strobe(CC1101_SRX)
