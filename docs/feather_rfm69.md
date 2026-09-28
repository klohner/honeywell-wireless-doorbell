# Adafruit Feather 32u4 RFM69HCW Serial Transceiver

This directory contains C / C++ firmware for the **Adafruit Feather 32u4 RFM69HCW Packet Radio** (900 MHz or 433 MHz version tuned to 916.8 MHz / 868.3 MHz).

---

## Features

- 💻 **Serial Terminal CLI Interface**: Send commands over USB Serial at 115200 baud.
- 📻 **2FSK Honeywell ActivLink Transceiver**: Transmits and receives ActivLink frames using the onboard RFM69HCW module.
- 🔔 **Interactive Controls**: Define target receiver Key ID, trigger alert patterns, secret knock, low battery indicators, and toggle frequency bands (916.8 MHz NA/AU vs 868.3 MHz EU).

---

## Hardware Pinout (Feather 32u4 RFM69HCW)

The Adafruit Feather 32u4 RFM69HCW has the radio internally wired to SPI and specific IO pins:

| RFM69 Pin | ATmega32u4 Pin | Function |
| :--- | :--- | :--- |
| **CS** | Pin 8 | SPI Chip Select |
| **RST** | Pin 4 | Radio Reset |
| **DIO0** | Pin 7 (IRQ 4) | Interrupt / Data Out |
| **MOSI** | SPI MOSI | SPI Master Out |
| **MISO** | SPI MISO | SPI Master In |
| **SCK** | SPI SCK | SPI Clock |

---

## Interactive Serial Command Interface

Connect via Serial Monitor / PuTTY at **115200 baud**:

| Command | Arguments | Description | Example |
| :--- | :--- | :--- | :--- |
| `SET_KEY` | `<hex_id>` | Set default target Key ID (up to 8 hex chars) | `SET_KEY 8BFA3` |
| `TX` | `[alert] [knock] [lowbat]` | Transmit ActivLink burst with current parameters | `TX 0 0 0` |
| `SET_FREQ` | `<freq_mhz>` | Set frequency (916.8 or 868.3) | `SET_FREQ 916.8` |
| `STATUS` | None | Print current radio registers and configured parameters | `STATUS` |
| `HELP` | None | Display help menu | `HELP` |

---

## Building & Flashing

### Using PlatformIO CLI

```bash
cd feather_32u4
pio run --target upload
pio device monitor
```
