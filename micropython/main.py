"""MicroPython Honeywell ActivLink main entry point."""

import network
import time
import uasyncio as asyncio
from cc1101 import CC1101
from activlink import ActivLinkEngine
from webserver import MicroWebServer

# Setup WiFi Access Point
ap = network.WLAN(network.AP_IF)
ap.active(True)
ap.config(essid="Honeywell-ActivLink-uPy", password="activlink123")

print("WiFi Access Point active.")
print("IP Address:", ap.ifconfig()[0])

# Initialize Radio and Engine
radio = CC1101(freq_mhz=916.8)
if not radio.begin():
    print("ERROR: Failed to initialize CC1101 module!")
else:
    print("CC1101 initialized successfully.")

engine = ActivLinkEngine(radio)
engine.begin()

server = MicroWebServer(engine, radio, port=80)


async def main_loop():
    asyncio.create_task(server.start())
    while True:
        rx_pkt = engine.check_rx_packet()
        if rx_pkt:
            print("Received ActivLink Transmission:", rx_pkt["frame_hex"])
        await asyncio.sleep_ms(20)


try:
    asyncio.run(main_loop())
except KeyboardInterrupt:
    print("Stopped.")
