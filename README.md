# Honeywell "ActivLink" Wireless Doorbell & Security RF Protocol

[![Python Version](https://img.shields.io/badge/python-3.8%2B-blue.svg)](https://www.python.org/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![PlatformIO](https://img.shields.io/badge/ESP32-PlatformIO-orange.svg)](https://platformio.org/)
[![MicroPython](https://img.shields.io/badge/MicroPython-1.20%2B-green.svg)](https://micropython.org/)

This project contains research, Python tools, ESP32 PlatformIO firmware, and a standalone MicroPython application for capturing, decoding, and transmitting signals used by the **Honeywell ActivLink** wireless family (doorbells, PIR motion detectors, door/window sensors, and security chime receivers).

ActivLink operates at:
- **916.8 MHz** in North America and Australia.
- **868.3 MHz** in European countries (also compatible with Friedland Libra+ and Response systems).

---

## Project Features & Architecture

- 🛰️ **Protocol Specification**: Complete analysis of the 2FSK 6250 baud PWM pulse framing and bit fields. See [docs/protocol.md](docs/protocol.md).
- 🐍 **Python Library & CLI (`honeywell_activlink`)**: Pure Python 3 package for constructing frames, calculating parity, encoding PWM pulse bitstreams, and decoding raw captures.
- 📻 **Interactive RF Cat Tools (`honeywell_doorbell.py`)**: Script for YARD Stick One / RfCat interactive RF transmission.
- ⚡ **ESP32 + CC1101 C++ Application**: PlatformIO application with CC1101 driver, precision pulse timing engine, WiFi connectivity, live Web UI dashboard, and REST API. See [docs/esp32.md](docs/esp32.md).
- 🐍⚡ **MicroPython ESP32 Application**: Native MicroPython port (`micropython/`) featuring CC1101 SPI driver, pulse timing, and async web server. See [docs/micropython.md](docs/micropython.md).

---

## Directory Structure

```text
├── docs/
│   ├── protocol.md         # Detailed 2FSK RF protocol, frame layout & rtl_433 guide
│   ├── esp32.md            # ESP32 + CC1101 wiring, PlatformIO setup & Web UI guide
│   └── micropython.md      # MicroPython installation & web gateway guide
├── honeywell_activlink/    # Python 3 library & CLI tool
│   ├── __init__.py
│   ├── encoder.py          # Frame bit generation & PWM encoder
│   ├── decoder.py          # Bitstream & pulse decoder
│   └── cli.py              # Command-line tool interface
├── esp32/                  # ESP32 + CC1101 C++ PlatformIO firmware
│   ├── platformio.ini
│   └── src/                # C++ source files (CC1101 driver, Web Server, SSE)
├── micropython/            # ESP32 MicroPython gateway files
│   ├── cc1101.py           # MicroPython CC1101 SPI driver
│   ├── activlink.py        # MicroPython ActivLink engine
│   ├── webserver.py        # MicroPython uasyncio Web Server & REST API
│   └── main.py             # MicroPython entry point
├── tests/                  # Pytest unit tests for Python library
├── honeywell_doorbell.py   # RfCat interactive transmitter script
└── README.md               # Main project documentation
```

---

## Quickstart Guide

### 1. Python Library & CLI Installation

```bash
# Clone the repository
git clone https://github.com/user/honeywell_activlink.git
cd honeywell_activlink

# Run CLI encoder
python3 -m honeywell_activlink.cli encode --key-id 8BFA3 --alert 0

# Run CLI decoder
python3 -m honeywell_activlink.cli decode --hex 00008bfa3000

# Run pytest unit tests
pytest
```

### 2. ESP32 + CC1101 Applications

- **PlatformIO (C++) Firmware**: See [docs/esp32.md](docs/esp32.md)
- **MicroPython Gateway**: See [docs/micropython.md](docs/micropython.md)

---

## Hardware Compatibility

### Tested Transmitters & Receivers

- **North American Models**: RDWL311A, RDWL313A, RDWL515A, RDWL917AX, RCWL251A, RPWL300A, RPWL400W, RPWL401B, RPWL4045A.
- **European Models**: DW915SG, DC917SL, DC915SG, DC515S, DC315N, HS3MAG1N, HS3PIR1S, Friedland Libra+ Series (D911S, D912S, D914b, etc.), Friedland Response Alarm Series.
- **Australian Models**: DC917NGA, DC515NA, DC313NA, DCP311GA.

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
