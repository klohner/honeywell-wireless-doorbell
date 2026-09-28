# ESP32 + CC1101 Honeywell ActivLink Transceiver & Web Interface

This document provides complete instructions for building and running the ESP32 and Texas Instruments CC1101 wireless transceiver application.

---

## Hardware Requirements

1. **ESP32 Development Board** (e.g., ESP32-WROOM-32, NodeMCU-32S, ESP32-S3)
2. **TI CC1101 Transceiver Module** (868 MHz / 915 MHz version with external antenna)
3. **Jumper Wires & Breadboard**

---

## Hardware Wiring & Pinout

Connect the CC1101 module SPI and GPIO pins to the ESP32 as follows:

| CC1101 Pin | ESP32 GPIO Pin | Function Description |
| :--- | :--- | :--- |
| **VCC** | 3.3V | 3.3V Power Supply (Do NOT use 5V) |
| **GND** | GND | Ground Connection |
| **CSN / SS** | GPIO 5 | SPI Chip Select |
| **SCK** | GPIO 18 | SPI Clock |
| **MOSI** | GPIO 23 | SPI Master Out Slave In |
| **MISO** | GPIO 19 | SPI Master In Slave Out |
| **GDO0** | GPIO 2 | Digital IO 0 (TX Data / RX Pulse Interrupt) |
| **GDO2** | GPIO 4 | Digital IO 2 (Optional Carrier Detect) |

> **Note**: Standard 868/915 MHz CC1101 modules require an appropriate SMA or wire antenna ($1/4$ wave length $\approx 8.2\text{cm}$ for 916.8 MHz) for clear transmission and reception.

---

## Firmware Compilation & Flashing

The firmware is structured as a PlatformIO project in the `esp32/` directory.

### Prerequisites

- [VS Code with PlatformIO IDE Extension](https://platformio.org/) OR [PlatformIO Core CLI](https://docs.platformio.org/en/latest/core/index.html)

### Building with PlatformIO CLI

```bash
# Navigate to the esp32 directory
cd esp32

# Build project
pio run

# Flash to connected ESP32 board
pio run --target upload

# Open Serial Monitor (115200 baud)
pio device monitor
```

---

## WiFi Configuration & Web Interface

### Initial Network Setup

Upon booting for the first time, if no WiFi credentials are saved, the ESP32 creates an Access Point:
- **SSID**: `Honeywell-ActivLink-AP`
- **Password**: `activlink123`
- **IP Address**: `192.168.4.1`

Connect to this AP and open `http://192.168.4.1` in your browser to configure your home WiFi credentials, or use the REST API.

### Web Dashboard Features

When connected to WiFi, access the web interface via the assigned local IP (or `http://activlink.local` via MDNS):

1. **Live Signal Monitor**: View incoming Honeywell ActivLink frames in real-time via Server-Sent Events (SSE). Decodes Device ID, Device Type, Alert Mode, Secret Knock, Relay, and Low Battery status.
2. **Signal Transmitter**: Send custom Honeywell signals to trigger chiming, alert flashes, or test receiver responses.
3. **Frequency Selector**: Instantly toggle RF band between North America / Australia (`916.8 MHz`) and Europe (`868.3 MHz`).

---

## REST API Endpoints

The web server exposes the following REST API endpoints:

### `GET /api/status`
Returns system status, WiFi connectivity, current frequency, and last received frame.

**Response**:
```json
{
  "status": "online",
  "frequency_mhz": 916.8,
  "wifi_connected": true,
  "ip": "192.168.1.150",
  "packets_received": 14
}
```

### `POST /api/transmit`
Triggers a Honeywell ActivLink transmission.

**Request Body**:
```json
{
  "key_id": "8BFA3",
  "secret_knock": 0,
  "alert": 0,
  "lowbat": 0,
  "relay": 0,
  "burst": 50
}
```

### `GET /api/events`
Server-Sent Events (SSE) stream broadcasting received radio packets in real-time.
