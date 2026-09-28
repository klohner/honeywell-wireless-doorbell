# MicroPython Honeywell ActivLink Gateway for ESP32 + CC1101

This directory contains a standalone MicroPython port of the Honeywell ActivLink gateway for ESP32 and Texas Instruments CC1101 transceivers.

---

## Hardware Requirements & Pinout

| CC1101 Pin | ESP32 GPIO Pin | MicroPython Hardware SPI |
| :--- | :--- | :--- |
| **VCC** | 3.3V | Power Supply (3.3V max) |
| **GND** | GND | Ground |
| **CSN / SS** | GPIO 5 | Chip Select Pin |
| **SCK** | GPIO 18 | SPI Clock (SPI1/HSPI or SPI2/VSPI) |
| **MOSI** | GPIO 23 | SPI MOSI |
| **MISO** | GPIO 19 | SPI MISO |
| **GDO0** | GPIO 2 | Digital IO (TX pulse output / RX input) |

---

## MicroPython Installation & Flashing

1. Download the latest ESP32 MicroPython firmware image (`.bin`) from [micropython.org/download/esp32](https://micropython.org/download/esp32/).
2. Flash the firmware using `esptool`:
   ```bash
   esptool.py --chip esp32 --port /dev/ttyUSB0 erase_flash
   esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 write_flash -z 0x1000 esp32-20231005-v1.21.0.bin
   ```
3. Upload the MicroPython files to the ESP32 using `ampy`, `rshell`, or `mpremote`:
   ```bash
   cd micropython
   mpremote connect /dev/ttyUSB0 cp cc1101.py activlink.py webserver.py main.py :
   ```

---

## Web Dashboard & API Usage

Upon booting, the ESP32 creates a WiFi Access Point:
- **SSID**: `Honeywell-ActivLink-uPy`
- **Password**: `activlink123`
- **Dashboard URL**: `http://192.168.4.1/`

### REST API Endpoints

- `GET /api/status`: Returns JSON status and current network IP address.
- `POST /api/transmit`: Accepts JSON body `{"key_id": "8BFA3", "alert": 0, "secret_knock": 0, "lowbat": 0, "burst": 50}` to transmit ActivLink signals.
