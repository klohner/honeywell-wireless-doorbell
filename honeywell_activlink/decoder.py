"""Decoder for Honeywell ActivLink frames and PWM bitstreams."""

from typing import Dict, Any, Optional
from .encoder import hex_to_bits, bits_to_hex, calculate_parity


def decode_frame(frame_hex: str) -> Dict[str, Any]:
    """Decode a 48-bit Honeywell ActivLink frame hex into bit fields.

    Args:
        frame_hex: 12-character hex string representing 48 bits.

    Returns:
        Dict containing decoded fields: key_id, device_type, alert,
        secret_knock, relay, lowbat, parity, parity_valid.
    """
    bits = hex_to_bits(frame_hex, 48)

    key_id_bits = bits[0:32]
    key_id_hex = f"{int(key_id_bits, 2):08x}"

    device_type_bits = bits[34:36]
    device_type = int(device_type_bits, 2)

    alert_bits = bits[38:40]
    alert = int(alert_bits, 2)

    secret_knock = int(bits[43])
    relay = int(bits[44])
    lowbat = int(bits[46])

    parity_bit = int(bits[47])
    computed_parity = calculate_parity(bits[:47])
    parity_valid = parity_bit == computed_parity

    device_type_str = "Doorbell / Push Button" if device_type == 2 else ("PIR Sensor" if device_type == 1 else f"Unknown ({device_type})")

    return {
        "frame_hex": frame_hex.upper(),
        "key_id": key_id_hex.upper(),
        "device_type": device_type,
        "device_type_str": device_type_str,
        "alert": alert,
        "secret_knock": secret_knock,
        "relay": relay,
        "lowbat": lowbat,
        "parity": parity_bit,
        "parity_valid": parity_valid,
        "raw_bits": bits,
    }


def decode_pwm_stream(pwm_str: str) -> Optional[Dict[str, Any]]:
    """Decode a raw PWM symbol bitstream ('100110...') into frame data.

    In ActivLink PWM:
    - '100' -> '0'
    - '110' -> '1'

    Returns decoded frame dict or None if stream format is invalid.
    """
    # Strip preamble '000' and postamble '111' if present
    cleaned = pwm_str.strip()
    if cleaned.startswith("000"):
        cleaned = cleaned[3:]
    if cleaned.endswith("111"):
        cleaned = cleaned[:-3]

    if len(cleaned) % 3 != 0 or len(cleaned) < 144:  # 48 bits * 3 = 144 symbols
        return None

    decoded_bits = []
    for i in range(0, len(cleaned), 3):
        triplet = cleaned[i : i + 3]
        if triplet == "100":
            decoded_bits.append("0")
        elif triplet == "110":
            decoded_bits.append("1")
        else:
            return None  # Invalid PWM symbol

    bit_string = "".join(decoded_bits[:48])
    frame_hex = bits_to_hex(bit_string)
    return decode_frame(frame_hex)
