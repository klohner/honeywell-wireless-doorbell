"""MicroPython Honeywell ActivLink protocol engine."""

import time
from machine import Pin

SYMBOL_US = 160


class ActivLinkEngine:
    def __init__(self, radio, tx_rx_pin=2):
        self.radio = radio
        self.pin_num = tx_rx_pin
        self.pin = Pin(tx_rx_pin, Pin.OUT)
        self.pulse_buffer = []
        self.last_us = time.ticks_us()
        self.frame_ready = False

    def _irq_handler(self, pin):
        now = time.ticks_us()
        dur = time.ticks_diff(now, self.last_us)
        self.last_us = now

        if dur > 3000:
            self.pulse_buffer.clear()
        elif len(self.pulse_buffer) < 300 and not self.frame_ready:
            self.pulse_buffer.append(dur)
            if len(self.pulse_buffer) >= 288:
                self.frame_ready = True

    def begin(self):
        self.pin = Pin(self.pin_num, Pin.IN)
        self.pin.irq(trigger=Pin.IRQ_RISING | Pin.IRQ_FALLING, handler=self._irq_handler)

    def send_bit(self, bit):
        if bit:
            # Bit 1: HIGH-HIGH-LOW (320 us HIGH, 160 us LOW)
            self.pin.value(1)
            time.sleep_us(SYMBOL_US * 2)
            self.pin.value(0)
            time.sleep_us(SYMBOL_US)
        else:
            # Bit 0: HIGH-LOW-LOW (160 us HIGH, 320 us LOW)
            self.pin.value(1)
            time.sleep_us(SYMBOL_US)
            self.pin.value(0)
            time.sleep_us(SYMBOL_US * 2)

    def send_preamble(self):
        # Preamble: LOW-LOW-LOW (480 us LOW)
        self.pin.value(0)
        time.sleep_us(SYMBOL_US * 3)

    def send_postamble(self):
        # Postamble: HIGH-HIGH-HIGH (480 us HIGH)
        self.pin.value(1)
        time.sleep_us(SYMBOL_US * 3)
        self.pin.value(0)

    def build_frame_hex(self, key_id, secret_knock=0, alert=0, lowbat=0, relay=0, device_type=2):
        if isinstance(key_id, str):
            clean = key_id.strip().lstrip("0x")
            key_val = int(clean, 16) if clean else 0
        else:
            key_val = int(key_id) & 0xFFFFFFFF

        bits = [0] * 48

        # Bits 0..31: Key ID
        for i in range(32):
            bits[i] = (key_val >> (31 - i)) & 1

        # Bits 34..35: Device Type
        bits[34] = (device_type >> 1) & 1
        bits[35] = device_type & 1

        # Bits 36..37: Padding (0b10)
        bits[36] = 1
        bits[37] = 0

        # Bits 38..39: Alert mode
        bits[38] = (alert >> 1) & 1
        bits[39] = alert & 1

        # Bit 43: Secret knock
        bits[43] = secret_knock & 1

        # Bit 44: Relay
        bits[44] = relay & 1

        # Bit 46: Low battery
        bits[46] = lowbat & 1

        # Calculate parity over first 47 bits
        ones = sum(bits[:47])
        bits[47] = ones % 2

        # Convert bits array to hex string
        hex_chars = []
        for b_idx in range(6):
            byte_val = 0
            for bit_idx in range(8):
                byte_val = (byte_val << 1) | bits[b_idx * 8 + bit_idx]
            hex_chars.append("{:02X}".format(byte_val))

        return "".join(hex_chars)

    def transmit_frame(self, frame_hex, burst_count=50):
        self.pin.irq(handler=None)
        self.radio.set_tx_mode()
        self.pin = Pin(self.pin_num, Pin.OUT)

        # Convert frame_hex to 48 bits
        bits = []
        val = int(frame_hex, 16)
        for i in range(48):
            bits.append((val >> (47 - i)) & 1)

        for _ in range(burst_count):
            self.send_preamble()
            for b in bits:
                self.send_bit(b)
            self.send_postamble()

        self.pin.value(0)
        time.sleep_us(2000)

        self.radio.set_rx_mode()
        self.pin = Pin(self.pin_num, Pin.IN)
        self.pin.irq(trigger=Pin.IRQ_RISING | Pin.IRQ_FALLING, handler=self._irq_handler)

    def decode_frame_hex(self, frame_hex):
        val = int(frame_hex, 16)
        bits = [(val >> (47 - i)) & 1 for i in range(48)]

        key_id_val = 0
        for i in range(32):
            key_id_val = (key_id_val << 1) | bits[i]

        dev_type = (bits[34] << 1) | bits[35]
        alert = (bits[38] << 1) | bits[39]
        secret_knock = bits[43]
        relay = bits[44]
        lowbat = bits[46]
        parity = bits[47]

        parity_valid = (parity == (sum(bits[:47]) % 2))
        dev_type_str = "Doorbell / Push Button" if dev_type == 2 else ("PIR Sensor" if dev_type == 1 else "Unknown")

        return {
            "frame_hex": frame_hex.upper(),
            "key_id": "{:08X}".format(key_id_val),
            "device_type": dev_type,
            "device_type_str": dev_type_str,
            "alert": alert,
            "secret_knock": secret_knock,
            "relay": relay,
            "lowbat": lowbat,
            "parity": parity,
            "parity_valid": parity_valid
        }

    def check_rx_packet(self):
        if not self.frame_ready:
            return None

        bits = []
        for i in range(0, len(self.pulse_buffer) - 1, 2):
            high_dur = self.pulse_buffer[i]
            bits.append("1" if high_dur > 220 else "0")

        self.pulse_buffer.clear()
        self.frame_ready = False

        if len(bits) >= 48:
            bit_str = "".join(bits[:48])
            val = int(bit_str, 2)
            hex_str = "{:012X}".format(val)
            return self.decode_frame_hex(hex_str)

        return None
