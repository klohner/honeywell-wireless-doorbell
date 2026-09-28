"""Command Line Interface for Honeywell ActivLink encoder & decoder."""

import argparse
import json
import sys
from typing import List

from .encoder import create_frame, encode_frame_to_pwm
from .decoder import decode_frame, decode_pwm_stream


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Honeywell ActivLink Protocol CLI Encoder & Decoder"
    )
    subparsers = parser.add_subparsers(dest="command", help="Command to run")

    # Encode subcommand
    encode_parser = subparsers.add_parser("encode", help="Encode frame flags into hex / PWM bitstream")
    encode_parser.add_argument("--key-id", required=True, help="Transmitter Key ID (hex string or integer, e.g., 8BFA3)")
    encode_parser.add_argument("--alert", type=int, default=0, choices=[0, 1, 2, 3], help="Alert mode (0=normal, 1/2=halo, 3=alarm)")
    encode_parser.add_argument("--secret-knock", type=int, default=0, choices=[0, 1], help="Secret knock flag (1=enabled)")
    encode_parser.add_argument("--lowbat", type=int, default=0, choices=[0, 1], help="Low battery flag (1=low)")
    encode_parser.add_argument("--relay", type=int, default=0, choices=[0, 1], help="Relay flag (1=retransmitted)")
    encode_parser.add_argument("--device-type", type=int, default=2, choices=[1, 2], help="Device type (2=doorbell, 1=PIR)")
    encode_parser.add_argument("--json", action="store_true", help="Output result as JSON")

    # Decode subcommand
    decode_parser = subparsers.add_parser("decode", help="Decode 48-bit hex frame or PWM bitstream")
    decode_group = decode_parser.add_mutually_exclusive_group(required=True)
    decode_group.add_argument("--hex", help="12-character frame hex string to decode")
    decode_group.add_argument("--pwm", help="PWM symbol bitstream ('000100110...')")
    decode_parser.add_argument("--json", action="store_true", help="Output result as JSON")

    return parser


def main(args: List[str] = None) -> None:
    parser = build_parser()
    parsed_args = parser.parse_args(args)

    if parsed_args.command == "encode":
        frame_hex = create_frame(
            key_id=parsed_args.key_id,
            secret_knock=parsed_args.secret_knock,
            alert=parsed_args.alert,
            lowbat=parsed_args.lowbat,
            relay=parsed_args.relay,
            device_type=parsed_args.device_type,
        )
        pwm_stream = encode_frame_to_pwm(frame_hex)
        decoded = decode_frame(frame_hex)

        result = {
            "frame_hex": frame_hex.upper(),
            "pwm_stream": pwm_stream,
            "decoded": decoded,
        }

        if parsed_args.json:
            print(json.dumps(result, indent=2))
        else:
            print(f"Constructed Frame Hex: {frame_hex.upper()}")
            print(f"Key ID:               {decoded['key_id']}")
            print(f"Device Type:          {decoded['device_type_str']}")
            print(f"Alert Mode:           {decoded['alert']}")
            print(f"Secret Knock:         {decoded['secret_knock']}")
            print(f"Relay Bit:            {decoded['relay']}")
            print(f"Low Battery:          {decoded['lowbat']}")
            print(f"Parity Bit:           {decoded['parity']} (Valid: {decoded['parity_valid']})")
            print(f"PWM Symbol Stream:    {pwm_stream}")

    elif parsed_args.command == "decode":
        if parsed_args.hex:
            result = decode_frame(parsed_args.hex)
        elif parsed_args.pwm:
            result = decode_pwm_stream(parsed_args.pwm)
            if not result:
                print("Error: Could not decode PWM symbol bitstream.", file=sys.stderr)
                sys.exit(1)

        if parsed_args.json:
            print(json.dumps(result, indent=2))
        else:
            print(f"Decoded Frame Hex:    {result['frame_hex']}")
            print(f"Key ID:               {result['key_id']}")
            print(f"Device Type:          {result['device_type_str']}")
            print(f"Alert Mode:           {result['alert']}")
            print(f"Secret Knock:         {result['secret_knock']}")
            print(f"Relay Bit:            {result['relay']}")
            print(f"Low Battery:          {result['lowbat']}")
            print(f"Parity Bit:           {result['parity']} (Valid: {result['parity_valid']})")

    else:
        parser.print_help()


if __name__ == "__main__":
    main()
