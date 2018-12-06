# Honeywell "ActivLink" Wireless Chimes

## What is this?

An attempt to capture and decode the signals used by the North American
Honeywell RCWL300A, RCWL330A, Series 3, 5, 9 and Decor Series Wireless Chimes.
These units operate at 916.8 MHz. The protocol may be identical to European
variants of these model devices, though those models may operate at or near 868
MHz.

These devices are dissimilar to many other "dumb" wireless doorbells since they
support various other sensors (such as motion sensors), and the signal protocol
supports options for alarm triggers, relaying of signals, and parity checks.

I'm using [rtl_433](https://github.com/merbanan/rtl_433) to receive and
demodulate the signal using its default functionality -- it does not currently
support a device protocol for this series of Honeywell devices. Further effort
into creating a device protocol driver for rtl_433 is an eventual goal.

I'm also able to transmit valid signals to these Honeywell receivers using the
[YARD Stick One](https://greatscottgadgets.com/yardstickone/), which uses the
[TI CC1111](http://www.ti.com/product/CC1110-CC1111) chipset.

I've also had success transmitting a valid signal using the 
[HopeRF RFM69HCW](http://www.hoperf.com/rf_transceiver/modules/RFM69HCW.html) 
module, specifically as found as part of the 
[Adafruit Feather 32u4 RFM69HCW Packet Radio](https://www.adafruit.com/product/3076) 
device.

Planned further efforts include working with modules that use the 
[TI CC1101](http://www.ti.com/product/CC1101) chip, and maybe other modules that
are able to support this signal.

### Awesome! I have an RTL-SDR dongle and one of those doorbells and I want to help!

You're the best! I'm looking for as many valid signals as you can generate on
your doorbell system to further decode the signal. If you have `rtl_433`
installed and working on your system, please see if you can receive the signal
and send me the output you receive here:

**[Submit My Doorbell Code To This Project](https://goo.gl/forms/SuxA3qgVRivXmNMf1)**

Any kind of signal you can provide helps. I'm especially interested in signals
from the motion detectors or other sensors used with this system.

### Availability

Although I've only personally tested on the North American models of this unit,
it seems that these doorbells are also available in European model variants.
Those versions operate near 868 MHz amd are advertised as using "ActivLink"
technology. I've read indications that those models also use the same
transmission signal, though on that different frequency.

[There's some](https://livewell.honeywellhome.com/en/support/alarm-support/) [indication](https://livewell.honeywellhome.com/en/support/alarm-support/) that this protocol may be based on the Friedland / Response 868MHz alarm system.  [This FAQ](https://livewell.honeywellhome.com/en/support/doorbell-support/) 
indicates that the 868MHz variants of the Honeywell ActivLink system is compatible 
with the Friedland Libra+ Wirefree Doorbell system.  [An IQ sample from this system](https://www.sigidwiki.com/wiki/Friedland_Libra%2B_48249SL_wireless_doorbell) is available at the [Signal Identification Guide wiki](https://www.sigidwiki.com/wiki/Signal_Identification_Guide) and should be analyzed further.

### Hardware inside the Honeywell doorbell transmitter

User [tos7](https://www.rtl-sdr.com/forum/memberlist.php?mode=viewprofile&u=1539)
has [detailed on an rtl-sdr.com forum](https://www.rtl-sdr.com/forum/viewtopic.php?t=1138) 
that the button contains a PIC Micro controller and a Transmitter chip.

- [PIC16F505](https://www.microchip.com/wwwproducts/en/PIC16F505)

- [FSK Transmitter - 868/915 MHz TH72031](https://www.melexis.com/en/product/TH72031/FSK-Transmitter)

## The Wireless Signal

The Honeywell wireless transmitter I have direct access to is the RPWL400W,
though I was able to look at three of them. I also have tested with a Series 9
and Series 3 receiver.

When the wireless doorbell button is pressed, it sends out a signal centered at
916.8 MHz. It seems to be using 2FSK modulation with a 50 kHz deviation. The
modulation rate seems to be 6250 baud, so each HIGH or LOW symbol is 160
microseconds (μs).

Because it is using digital symbols over 2FSK modulation, it essentially looks
like two separate, simultaneous, out-of-phase OOK transmissions 100 kHz away
from each other, at 916.85 MHz and and 916.75 MHz.  In FSK parlance, these higher
and lower frequencies are respectively referred to as the "mark" and "space" 
frequencies.

If you choose to visually look at only one of those frequencies as an ASK/OOK
transmission (such as with SDR# or gqrx), it may make sense to choose the higher
"mark" 916.85 MHz frequency, since its phase would represent a "HIGH" symbol as a 
high signal level, and a "LOW" symbol would be the absense of a signal.

Data bits are encoded over three symbols. A "0" bit is defined as HIGH-LOW-LOW,
and a "1" bit is defined as "HIGH-HIGH-LOW".

Each frame of data consists of a LOW-LOW-LOW signal preamble, 48 bits (144 
symbols) of data, and a postamble of HIGH-HIGH-HIGH.

The signal seems to be 50 consecutive repetitions of the frame, then symbols
LOW-LOW-LOW-HIGH-HIGH-HIGH, and finally 2 continuous milliseconds (2000 μs) of
LOW.

So, the duration of the entire 2FSK signal is 

	(50 reps * ((1 + 48 + 1 bits) * (3 symbols * 160 μs))) + (6 symbols * 160 μs) + 2000 μs
	=
	(2500 * 480 μs) + 960 μs + 2000 μs
	=
	1200000 μs + 2960 μs
	=
	1.202960 seconds

## The Data Frame

Each data frame is 48 bits, or 6 bytes long. With some experimentation (using a
YARD Stick One to create custom signals), here's what I've been able to
ascertain about the data in the frame.

	# Frame bits used in Honeywell RCWL300A, RCWL330A, Series 3, 5, 9 and all Decor Series Wireless Chimes
	# 0000 0000 1111 1111 2222 2222 3333 3333 4444 4444 5555 5555
	# 7654 3210 7654 3210 7654 3210 7654 3210 7654 3210 7654 3210
	# XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX XX.. XXX. .... KEY DATA (any change and receiver doesn't seem to recognize signal)
	# XXXX XXXX XXXX XXXX XXXX .... .... .... .... .... .... .... KEY ID (different for each transmitter)
	# .... .... .... .... .... 0000 00.. 0000 0000 00.. 000. .... KEY UNKNOWN 0 (always 0 in devices I've tested)
	# .... .... .... .... .... .... ..XX .... .... .... .... .... DEVICE TYPE (10 = doorbell, 01 = PIR Motion sensor)
	# .... .... .... .... .... .... .... .... .... ..XX ...X XXX. FLAG DATA (may be modified for possible effects on receiver)
	# .... .... .... .... .... .... .... .... .... ..XX .... .... ALERT (00 = normal, 01 or 10 = right-left halo light pattern, 11 = full volume alarm)
	# .... .... .... .... .... .... .... .... .... .... ...X .... SECRET KNOCK (0 = default, 1 if doorbell is pressed 3x rapidly)
	# .... .... .... .... .... .... .... .... .... .... .... X... RELAY (1 if signal is a retransmission of a received transmission, only some models)
	# .... .... .... .... .... .... .... .... .... .... .... .X.. FLAG UNKNOWN (0 = default, but 1 is accepted and I don't observe any effects)
	# .... .... .... .... .... .... .... .... .... .... .... ..X. LOWBAT (1 if battery is low, receiver gives low battery alert)
	# .... .... .... .... .... .... .... .... .... .... .... ...X PARITY (LSB of count of set bits in previous 47 bits)

### Data Frame Device ID Notes:
For simplicity, the full Device ID for the device might just be the first 
4 bytes.  Device Type should be included as part of the Device ID since if 
it is changed, a receiver would no longer recognize the signal as being from 
the same device.  Bits after byte 4 that seem to be part of the Key ID are
always 0 on all devices I've seen.

### Data Frame KEY UNKNOWN 0 Notes:
These bits are 0 in all devices I've seen.  Some preliminary tests with
generating artificial signals seems to indicate that if these bits are not
0, the receiver does not seem to recognize the signal as a previously
recognized device.  So, these bits are either part of the device ID, or they
simply must be 0 for the receiver to accept the signal as valid.  Further
testing here is required.

### Data Frame DEVICE TYPE Notes:
For all devices I've tested, it seems that only the two bits specified for this
field change depending on the type of device used to generate the signal.
It seems logical that perhaps the the width of this field is more than two
bits.  It might even be all of the 4th byte.  I have no data to confirm this, 
though.  For all devices I've seen, though, only these two bits of the 4th 
byte might be anything other than 0.

### Data Frame ALERT Notes:
On some receivers, such as the Honeywell RDWL917AX, an alert type of 01 or
10 forces the receiver to display a blinking right and left pattern, instead
of the normal full perimeter LED blinking.

### Data Frame RELAY Notes:
On some receiver models, such as the "Honeywell RDWL917AX", the base receiver 
will immediately retransmit a valid received signal if the RELAY bit is NOT 
set. The data in the retransmitted signal will be modified with the RELAY bit 
set. This seems to be an effort to extend a signal to more distant receivers.

## Detecting signals using `rtl_433`

This seems to work pretty well to pick up data frames in the signal as symbol
pulse bits represented in hex. Note that the returned length of these frames is
149 bits (symbols) because it is decoding the frame's "HIGH-HIGH-HIGH" postamble
into three "1" bits (symbols).

	rtl_433 -f 916800000 -q -X Honeywell:FSK_PCM:160:160:400,bits=149,match={4}0xe

And this seems to work pretty well to pick up the signal frame data as hex. Note
that the returned length of these frames is 49 bits because it is decoding the
frame's "HIGH-HIGH-HIGH" postamble as a "1" bit.

	rtl_433 -f 916800000 -q -X Honeywell:FSK_PWM_RAW:240:480:400,bits=49,invert,match={4}0x8
		
Note the "invert" option is specified here to provide a decoding consistent with
this document.

This seems to work pretty well to pick up the whole signal with frame data in
rows as hex:

	rtl_433 -f 916800000 -q -X Honeywell:FSK_PWM_RAW:240:400:560,bits=49,invert

Note that these don't seem to pick up the all the data frames. In my tests, it
only seems to pick up 24 of the 50 data frames in the signal before triggering
`pulse_FSK_detect(): Maximum number of pulses reached!` and ignoring the rest of
the signal.

It seems to be possible to also pick up the signal as OOK in rtl_433 by setting
the frequency about 90 kHz away from the center of the FSK frequency. This seems
to force rtl_433 to only notice one side of the 2FSK signal. This command seems
to work well to pick up the data frames. It also has the benefit that when the
maximum number of pulses is reached, it doesn't trigger an error and starts
decoding data again immediately.

	rtl_433 -f 916890000 -q -X Honeywell:OOK_PWM:160:320:560:400,bits=49,invert

And finally, tuning low and allowing `rtl_433` to guess the demodulation on its
own, it will use `pulse_demod_pwm_precise()` and output a nice representation of
the hex and bit values of the 48-bit data frames. However, it does seems to
truncate after 24 rows of data frames.

	rtl_433 -f 916710000 -q -R 0 -A


## Transmitting a signal using a YARD Stick One

Convert the signal you want to send as a hex value representing the symbols in
the signal where a 1 bit is a HIGH symbol and a 0 bit is a LOW symbol.

```python
import sys
from rflib import *

def init(device):
	device.setFreq(916800000)
	device.setMdmModulation(MOD_2FSK)
	device.setMdmDeviatn(50000)
	device.setMdmSyncMode(0)
	device.setMdmDRate(6250)
	device.setMaxPower()

r = RfCat()
init(r)
r.RFxmit(data=pwm_signal_bytes)
```

### Devices using this signal
Here's an incomplete list of devices and kits known or suspected to use this signal.

#### North American models
- RCA902N1004/N Wireless Motion Detector
- RDWL311A 3 Series Portable Wireless Doorbell & Push Button
- RDWL313A 3 Series Portable Wireless Doorbell with Strobe Light & Push Button
- RDWL313P 3 Series Plug-In Wireless Doorbell with Strobe Light & Push Button
- RDWL515A2000/E 5 Series Portable Wireless Doorbell with Halo Light & Push Button
- RDWL515P 5 Series Plug-In Wireless Doorbell with Halo Light & Push Button
- RDWL515A2000/E Portable Wireless Doorbell with Halo Light and Push Button 2 Pack
- RDWL917AX2000/E Series 9 Portable Wireless Doorbell / Door Chime & Push Button
- RCWL251A1005 Décor Door Chime & Push Button
- RPWL300A Decor Wireless Push Button
- RPWL302A1005/A Decor Wireless Surface Mount Push Button for Door Chime
- RPWL4045A Wired to Wireless Doorbell Adapter Converter for Series 3, 5, 9 Honeywell Door Bells
- RPWL401B Wireless Doorbell Push Button for Series 3, 5, 9 Honeywell Door Bells (Black)
- RPWL401B2000/A Wireless Surface Mount Push Button
- RPWL400W Wireless Doorbell Push Button for Series 3, 5, 9 Honeywell Door Bells (White)
- RPWL400W2000/A Series 3, 5, 9 Wireless Doorbell Push Button with Halo Light
- RPWL4045A2000 Wired to Wireless Doorbell Adapter for Series 3, 5, 9
- RCWL330A1000/N P4-Premium Portable Wireless Doorbell / Door Chime and Push Button
- RCWL35 Series, includes RCWL35N, RCWL3501A, RCWL3502A, RCWL3503A, RCWL3504A, RCWL3505A, RCWL3506A
- RCWL3501A1004/N Decor Wireless Door Chime
- RCWL3502A1002/N Decor Wireless Door Chime
- RCWL3503A1000/N Decor Wireless Door Chime
- RCWL3504A1008/N Decor Wireless Door Chime
- RCWL3505A1005/N Decor Customizable Wood Wireless Doorbell / Door Chime and Push Button
- RCWL3506A1003/N Decor Wireless Door Chime

#### Australian models (916.8 MHz)
- DC917NGA Wireless portable MP3 doorbell with range extender, customisable melodies and push button – Grey
- DC515NA Wireless portable doorbell with halo light, sleep mode and push button – White
- DC515NGA Wireless portable doorbell with halo light, sleep mode and push button – Grey
- DC515NP2A Wireless plug-in doorbell with sleep mode, nightlight and push button – White
- DC515NGP2A Wireless plug-in doorbell with sleep mode, nightlight and push button – Grey
- DC313NA Wireless portable doorbell with volume control and push button – White
- DC313NGA Wireless portable doorbell with volume control and push button – Grey
- DCP311GA Wireless push button with LED confidence light – Portrait, Grey
- DCP511A Wireless push button with nameplate and LED confidence light – Offset Landscape, White
- DCP511GA Wireless push button with nameplate and LED confidence light – Offset Landscape, Grey

#### European models

- DW915SG Wired and wireless doorbell with range extender, sleep mode and halo light – Grey
- DW915S Wired and wireless doorbell with range extender, sleep mode and halo light – White
- DC917SL Wireless portable doorbell with range extender, customisable melodies and push button – White
- DC917SG Wireless portable doorbell with range extender, customisable melodies and push button – Grey
- DC917NG Wireless portable doorbell with range extender, customisable melodies and push button – Grey
- DC915SG Wireless portable doorbell with range extender, sleep mode and push button – Grey
- DC915SEA Wireless portable doorbell with range extender, wireless motion sensor and push button – White
- DC915SCV Wireless portable doorbell with range extender, sleep mode and wired to wireless converter – White
- DC915S Wireless portable doorbell with range extender, sleep mode and push button – White
- DC915NG Wireless portable doorbell with range extender, sleep mode and push button – Grey
- DC915N Wireless portable doorbell with range extender, sleep mode and push button – White
- DC515S Wireless portable doorbell with halo light, sleep mode and push button – White
- DC515NGBS Wireless plug-in doorbell with sleep mode, nightlight and push button – Grey
- DC515NG Wireless portable doorbell with halo light and push button – Grey
- DC515NBS Wireless plug-in doorbell with sleep mode, nightlight and push button – White
- DC515N Wireless portable doorbell with halo light, sleep mode and push button – White
- DC315NG Wireless portable doorbell with halo light and push button – Grey
- DC315NBS Wireless plug-in doorbell with halo light, USB charging and push button – White
- DC315N Wireless portable doorbell with halo light and push button – White
- DC313SFB Wireless portable doorbell with volume control and two push buttons – White
- DC313NHGBS Wireless portable and plug-in doorbell with volume control and push button – White
- DC313NG Wireless portable doorbell with volume control and push button – Grey
- DC313NFB Wireless portable doorbell with volume control and two push buttons – White
- DC313NBS Wireless plug-in doorbell with volume control and push button – White
- DC313N Wireless portable doorbell with volume control and push button – White
- DC312SP2USB Wireless plug-in doorbell with push button – White
- DC311NBS Wireless plug-in doorbell with push button – White
- DC311N Wireless portable doorbell with push button – White
- DCP311 Wireless push button with LED confidence light – Portrait, White
- DCP311G Wireless push button with LED confidence light – Portrait, Grey
- DCP511 Wireless push button with nameplate and LED confidence light – Offset Landscape, White
- DCP711 Wireless push button with LED confidence light – Round, White
- DCP711G Wireless push button with LED confidence light – Round, Grey
- DCP917S Doorbell wired to wireless converter kit – White
- HS3MAG1N Wireless door and window sensor – White
- HS3MAG1S Wireless door and window sensor – White
- HS3MAG2S Wireless door and window sensor twin pack – White
- HS3SS1S Wireless solar siren
- HS3PIR2S Wireless motion sensor (PIR) twin pack
- HS3PIR1S Wireless motion sensor (PIR)
- HS3FOB1S Wireless remote control key fob
- HS3BS1S Wireless battery siren
- L430S Wireless Motion Sensor (IP54) – White

## References

- User [tos7](https://www.rtl-sdr.com/forum/memberlist.php?mode=viewprofile&u=1539) performed
some earlier valuable analysis of this hardware and signal as 
[detailed on an rtl-sdr.com forum post](https://www.rtl-sdr.com/forum/viewtopic.php?t=1138).

- [rtl_433](https://github.com/merbanan/rtl_433) is an incredible and essential
piece of software. I still have much of it to learn.

- The [YARD Stick One](https://greatscottgadgets.com/yardstickone/) is also an
incredible thing. Transmitting arbitrary digital signals to test out signal
changes is essential to this project. This made it easy.
	
- [rfcat](https://github.com/atlas0fd00m/rfcat) is also essential to effectively
tap into the abilities of the YARD Stick One.
