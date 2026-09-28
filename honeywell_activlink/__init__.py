"""Honeywell ActivLink protocol library for Python."""

from .encoder import create_frame, encode_frame_to_pwm, encode_frame_to_bytes
from .decoder import decode_frame, decode_pwm_stream

__version__ = "1.0.0"
__all__ = [
    "create_frame",
    "encode_frame_to_pwm",
    "encode_frame_to_bytes",
    "decode_frame",
    "decode_pwm_stream",
]
