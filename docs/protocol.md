# Honeywell ActivLink Protocol Specification

## Overview

Honeywell ActivLink is a proprietary radio frequency protocol used by Honeywell wireless doorbells, PIR motion sensors, door/window sensors, and home security chime receivers. It operates primarily in two frequency bands:
- **North America & Australia**: 916.8 MHz (sometimes referred to as 916.5–916.8 MHz)
- **Europe**: 868.3 MHz (also compatible with Friedland Libra+ and Response systems)

Unlike simple 433 MHz doorbells that use ASK/OOK modulation, ActivLink utilizes **2FSK modulation** with advanced pulse-width modulation (PWM) encoding, parity checks, flag attributes, and relay retransmission support.

---

## Radio Physical Layer Specification

| Parameter | Value | Notes |
| :--- | :--- | :--- |
| **Frequency (NA / AU)** | `916.800 MHz` | Center frequency |
| **Frequency (EU)** | `868.300 MHz` | Center frequency |
| **Modulation** | 2FSK | 2-frequency Shift Keying |
| **Frequency Deviation** | `±50 kHz` | Total carrier spread ~100 kHz |
| **Baud Rate** | `6250 baud` | Symbol duration $T_{\text{sym}} = 160\ \mu\text{s}$ |
| **Mark Frequency** | $f_c + 50\ \text{kHz}$ | Logic HIGH pulse |
| **Space Frequency** | $f_c - 50\ \text{kHz}$ | Logic LOW pulse |

---

## Pulse Width Modulation (PWM) Symbol Encoding

Each bit of data is encoded into three physical radio symbols (each $160\ \mu\text{s}$ in length):

- **Bit `0`**: `HIGH-LOW-LOW` (Pulse duration: $160\ \mu\text{s}$ HIGH, $320\ \mu\text{s}$ LOW, total $480\ \mu\text{s}$)
- **Bit `1`**: `HIGH-HIGH-LOW` (Pulse duration: $320\ \mu\text{s}$ HIGH, $160\ \mu\text{s}$ LOW, total $480\ \mu\text{s}$)

### Frame Structure

Each frame transmitted over the air consists of:
1. **Preamble**: `LOW-LOW-LOW` ($480\ \mu\text{s}$)
2. **Payload**: 48 data bits ($48 \times 3 \times 160\ \mu\text{s} = 23,040\ \mu\text{s}$)
3. **Postamble**: `HIGH-HIGH-HIGH` ($480\ \mu\text{s}$)

Total frame duration: $24.0\ \text{ms}$ ($24,000\ \mu\text{s}$).

### Transmission Burst Pattern

A standard transmission sequence consists of:
- **Burst count**: 50 consecutive frame repetitions.
- **Inter-frame gap / tail**: `LOW-LOW-LOW-HIGH-HIGH-HIGH` ($960\ \mu\text{s}$) followed by $2000\ \mu\text{s}$ continuous `LOW`.
- **Total burst duration**: $\approx 1.202960\ \text{seconds}$.

---

## 48-bit Data Frame Layout

The 48-bit (6-byte) data payload is structured as follows:

```text
Bit Offset  0         8        16        24        32        40     47
            |---------|---------|---------|---------|---------|------|
Field:      [-------------- KEY ID / DEVICE ADDRESS -------------]
            [............................][TYPE][..][AL][SK][RL][?][LB][P]
```

### Bit Field Breakdown

| Bit Range | Length | Field Name | Description |
| :--- | :--- | :--- | :--- |
| `0..31` | 32 bits | **Key ID (Bytes 0-3)** | Unique transmitter address assigned to device |
| `32..33` | 2 bits | **Key Unknown 0** | Always set to `00` in standard devices |
| `34..35` | 2 bits | **Device Type** | Device class indicator: `10` = Doorbell / Push Button, `01` = PIR Motion Sensor |
| `36..37` | 2 bits | **Key Unknown 1** | Constant padding bits (`00` or `10`) |
| `38..39` | 2 bits | **Alert Mode** | `00` = Normal ring, `01` / `10` = Left-right halo LED flash, `11` = High volume alarm |
| `40..42` | 3 bits | **Reserved / Unknown** | Unused / default `000` |
| `43` | 1 bit | **Secret Knock** | `1` if button pressed 3 times rapidly, `0` otherwise |
| `44` | 1 bit | **Relay / Extender** | `1` if retransmitted by a range extender chime, `0` if direct from transmitter |
| `45` | 1 bit | **Flag Unknown** | Reserved flag bit (default `0`) |
| `46` | 1 bit | **Low Battery** | `1` if battery voltage is low, triggering chime low-bat warning |
| `47` | 1 bit | **Parity Bit** | Even parity over bits `0..46`: `count_ones(bits[0..46]) % 2` |

---

## Receiving & Decoding with `rtl_433`

### North American / Australian Frequency (916.8 MHz)

```bash
rtl_433 -f 916800000 -q -R 0 -X "n=Honeywell_ActivLink,m=FSK_PWM,s=160,l=320,r=560,y=480,invert,bits=48"
```

### European Frequency (868.3 MHz)

```bash
rtl_433 -f 868300000 -q -R 0 -X "n=Honeywell_ActivLink,m=FSK_PWM,s=160,l=320,r=560,y=480,invert,bits=48"
```

### OOK Demodulation Offset Workaround

To prevent FSK pulse limit overflows in `rtl_433`, capture one side of the 2FSK signal using an OOK offset tuning frequency (+90 kHz):

```bash
rtl_433 -f 916890000 -q -R 0 -X "n=Honeywell_ActivLink,m=OOK_PWM,s=160,l=320,g=400,r=560,y=480,bits=48,invert"
```
