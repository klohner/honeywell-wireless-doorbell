#!/usr/bin/env python3
"""Honeywell Doorbell RF Transmission Script for YARD Stick One / RfCat.

Usage from rfcat interactive shell:
  %run honeywell_doorbell.py
  hw_tx()
"""

import sys
from honeywell_activlink.encoder import (
    create_frame,
    encode_frame_to_bytes,
    pwm_0,
    pwm_1,
    pwm_packet_start,
    pwm_packet_end,
    bits_to_bytes,
    bits_to_pwm,
)
from honeywell_activlink.decoder import decode_frame

try:
    from rflib import MOD_2FSK
except ImportError:
    # rflib is available in rfcat interactive shell environment
    MOD_2FSK = 0x20

default_key_id_hex = '8BFA3'


def make_honeywell_id(key_id, secret_knock=0, alert=0, lowbat=0, relay=0):
    """Construct Honeywell ActivLink frame hex string using honeywell_activlink library."""
    return create_frame(
        key_id=key_id,
        secret_knock=secret_knock,
        alert=alert,
        lowbat=lowbat,
        relay=relay,
    )


def hw_config(device):
    """Configure RfCat device registers for Honeywell ActivLink 2FSK modulation."""
    device.setFreq(916800000)
    device.setMdmModulation(MOD_2FSK)
    device.setMdmDeviatn(50000)
    device.setMdmSyncMode(0)
    device.setMdmDRate(6250)
    device.setMaxPower()


def hw_tx(device=None, key_id_hex=default_key_id_hex, secret_knock=0, alert=0, lowbat=0, relay=0, burst=50):
    """Transmit Honeywell ActivLink doorbell burst via RfCat device."""
    if device is None:
        try:
            device = d  # 'd' is predefined in rfcat shell
        except NameError:
            print("Error: No RfCat device passed or defined as 'd' in current namespace.")
            return

    hw_config(device)
    honeywell_id = make_honeywell_id(key_id_hex, secret_knock, alert, lowbat, relay)
    print(f"Honeywell TX key: {honeywell_id} x {burst}")
    pwm_burst_bytes = encode_frame_to_bytes(honeywell_id, burst=burst)
    device.RFxmit(data=pwm_burst_bytes)
    print("Done.")


if __name__ == "__main__":
    print(f"Honeywell ActivLink doorbell library loaded. Default key: {default_key_id_hex}")
    print("Run hw_tx() inside rfcat shell to transmit.")
