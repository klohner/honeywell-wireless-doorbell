"""Encoder for Honeywell ActivLink frames and bitstreams."""

import math
from typing import Dict, Any, Union


def hex_to_bits(hex_str: str, bit_len: int = 48) -> str:
    """Convert hex string to zero-padded binary string of bit_len bits."""
    clean_hex = hex_str.strip().lstrip("0x")
    val = int(clean_hex, 16) if clean_hex else 0
    return f"{val:0{bit_len}b}"


def bits_to_hex(bit_str: str) -> str:
    """Convert binary string to hexadecimal string."""
    bit_len = len(bit_str)
    hex_len = (bit_len + 3) // 4
    val = int(bit_str, 2) if bit_str else 0
    return f"{val:0{hex_len}x}"


def calculate_parity(bit_str_47: str) -> int:
    """Calculate even parity bit for the first 47 bits of a frame."""
    ones_count = bit_str_47.count("1")
    return ones_count % 2


def create_frame(
    key_id: Union[str, int],
    secret_knock: int = 0,
    alert: int = 0,
    lowbat: int = 0,
    relay: int = 0,
    device_type: int = 2,  # 2 = 0b10 (doorbell)
) -> str:
    """Construct a 48-bit Honeywell ActivLink frame hex string.

    Args:
        key_id: Hex string or integer device ID (32-bit key).
        secret_knock: 1 if secret knock triggered, else 0.
        alert: Alert mode (0=normal, 1 or 2=halo flash, 3=alarm).
        lowbat: 1 if low battery, else 0.
        relay: 1 if retransmitted relay signal, else 0.
        device_type: Device type (2 = 0b10 doorbell, 1 = 0b01 PIR).

    Returns:
        12-character hex string representing the 48-bit frame.
    """
    if isinstance(key_id, int):
        key_id_val = key_id & 0xFFFFFFFF
    else:
        clean_hex = str(key_id).strip().lstrip("0x")
        key_id_val = int(clean_hex, 16) if clean_hex else 0

    # Format key_id into top 32 bits (bits 0..31)
    key_bits = f"{key_id_val:032b}"[-32:]
    id_bits = bytearray(key_bits.encode("ascii") + b"0" * 16)

    # Device type at bits 34-35
    dev_type_bin = f"{device_type & 0x03:02b}"
    id_bits[34:36] = dev_type_bin.encode("ascii")

    # Key Unknown 1 at bits 36-37
    id_bits[36:38] = b"10"

    # Alert mode at bits 38-39
    alert_bin = f"{alert & 0x03:02b}"
    id_bits[38:40] = alert_bin.encode("ascii")

    # Secret knock at bit 43
    id_bits[43] = ord("1" if secret_knock else "0")

    # Relay at bit 44
    id_bits[44] = ord("1" if relay else "0")

    # Low battery at bit 46
    id_bits[46] = ord("1" if lowbat else "0")

    # Calculate checksum / parity bit for the first 47 bits
    first_47_str = id_bits[:47].decode("ascii")
    parity_bit = calculate_parity(first_47_str)
    id_bits[47] = ord(str(parity_bit))

    frame_bits = id_bits.decode("ascii")
    return bits_to_hex(frame_bits)


def pwm_0() -> str:
    """PWM encoding for bit 0: HIGH-LOW-LOW ('100')."""
    return "100"


def pwm_1() -> str:
    """PWM encoding for bit 1: HIGH-HIGH-LOW ('110')."""
    return "110"


def pwm_packet_start() -> str:
    """PWM preamble: LOW-LOW-LOW ('000')."""
    return "000"


def pwm_packet_end() -> str:
    """PWM postamble: HIGH-HIGH-HIGH ('111')."""
    return "111"


def bits_to_pwm(bit_str: str) -> str:
    """Convert bit string ('0101...') into PWM symbol bitstream ('100110...')."""
    pwm_out = []
    for bit in bit_str:
        if bit == "1":
            pwm_out.append(pwm_1())
        else:
            pwm_out.append(pwm_0())
    return "".join(pwm_out)


def encode_frame_to_pwm(frame_hex: str) -> str:
    """Convert 48-bit frame hex into complete PWM frame bitstream with preamble and postamble."""
    bit_str = hex_to_bits(frame_hex, 48)
    pwm_bits = f"{pwm_packet_start()}{bits_to_pwm(bit_str)}{pwm_packet_end()}"
    return pwm_bits


def bits_to_bytes(bit_str: str, pad_bit: str = "0", align: str = ">") -> bytes:
    """Convert binary string into bytes array."""
    bit_len = len(bit_str)
    byte_len = math.ceil(bit_len / 8)
    total_bits = byte_len * 8

    if align == ">":
        bit_str = bit_str.rjust(total_bits, pad_bit)
    else:
        bit_str = bit_str.ljust(total_bits, pad_bit)

    byte_arr = bytearray()
    for i in range(0, total_bits, 8):
        byte_chunk = bit_str[i : i + 8]
        byte_arr.append(int(byte_chunk, 2))
    return bytes(byte_arr)


def encode_frame_to_bytes(frame_hex: str, burst: int = 50) -> bytes:
    """Encode frame hex into byte payload for transmission (with burst repetition)."""
    pwm_frame = encode_frame_to_pwm(frame_hex)
    full_pwm_stream = pwm_frame * burst
    return bits_to_bytes(full_pwm_stream)
