"""Unit tests for honeywell_activlink package."""

import pytest
from honeywell_activlink import (
    create_frame,
    encode_frame_to_pwm,
    encode_frame_to_bytes,
    decode_frame,
    decode_pwm_stream,
)
from honeywell_activlink.encoder import calculate_parity, bits_to_bytes, hex_to_bits, bits_to_hex
from honeywell_activlink.cli import build_parser, main


def test_parity_calculation():
    assert calculate_parity("0000") == 0
    assert calculate_parity("1000") == 1
    assert calculate_parity("1100") == 0
    assert calculate_parity("1110") == 1


def test_hex_and_bits_conversion():
    bits = hex_to_bits("8BFA3", 48)
    assert len(bits) == 48
    assert bits_to_hex(bits) == "00000008bfa3"


def test_create_and_decode_frame():
    frame_hex = create_frame(key_id="8BFA3", alert=1, secret_knock=1, lowbat=1, relay=0)
    assert len(frame_hex) == 12

    decoded = decode_frame(frame_hex)
    assert decoded["alert"] == 1
    assert decoded["secret_knock"] == 1
    assert decoded["lowbat"] == 1
    assert decoded["relay"] == 0
    assert decoded["parity_valid"] is True


def test_pwm_encoding_and_decoding():
    frame_hex = create_frame(key_id="12345678", alert=0)
    pwm_stream = encode_frame_to_pwm(frame_hex)

    # PWM stream must start with preamble 000 and end with postamble 111
    assert pwm_stream.startswith("000")
    assert pwm_stream.endswith("111")
    assert len(pwm_stream) == 3 + (48 * 3) + 3  # 150 symbols

    decoded_from_pwm = decode_pwm_stream(pwm_stream)
    assert decoded_from_pwm is not None
    assert decoded_from_pwm["frame_hex"] == frame_hex.upper()


def test_bytes_encoding():
    frame_hex = create_frame(key_id="8BFA3")
    payload_bytes = encode_frame_to_bytes(frame_hex, burst=1)
    assert isinstance(payload_bytes, bytes)
    assert len(payload_bytes) > 0


def test_cli_parser(capsys):
    parser = build_parser()
    args = parser.parse_args(["encode", "--key-id", "8BFA3", "--alert", "0", "--json"])
    assert args.command == "encode"
    assert args.key_id == "8BFA3"

    main(["encode", "--key-id", "8BFA3", "--json"])
    captured = capsys.readouterr()
    assert "frame_hex" in captured.out

    main(["decode", "--hex", "00000008BFA3", "--json"])
    captured_decode = capsys.readouterr()
    assert "device_type" in captured_decode.out
