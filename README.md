# honeywell-wireless-doorbell

## What is this?

An attempt to capture and decode the signals used by the North American
Honeywell RCWL300A, RCWL330A, Series 3, 5, 9 and Decor Series Wireless Chimes.
These units operate at 916.8 MHz. The protocol may be identical to European
variats of these model devices, though those models may operate at or near
868 MHz.

These devices are dissimilar to many other "dumb" wireless doorbells
since they support various other sensors (such as motion sensors), and the
signal protocol supports options for alarm triggers, relaying of signals, and 
parity checks.

I'm using [rtl_433](https://github.com/merbanan/rtl_433) to receive and demodulate 
the signal using its default functionality -- it does not currently support a device
protocol for this series of Honeywell devices. Further effort into creating a
device protocol driver for rtl_433 is an eventual goal.

I'm also able to transmit valid signals to these Honeywell receivers using the 
[YARD Stick One](https://greatscottgadgets.com/yardstickone/). Further efforts may
include efforts into supporting this and other devices for transmitting and
receiving valid signals.

### Awesome! I have an RTL-SDR dongle and one of those doorbells and I want to help!

You're the best! I'm looking for as many valid signals as you can generate on
your doorbell system to further decode the signal. If you have `rtl_433` installed
and working on your system, please see if you can receive the signal and send me
the output you receive here:

**[Submit My Doorbell Code To This Project](https://goo.gl/forms/SuxA3qgVRivXmNMf1)**

Any kind of signal you can provide helps. I'm especially interested in signals from the 
motion detectors or other sensors used with this system.

### Availability

Although I've only personally tested on the North American models of this unit, it seems 
that these doorbells are also available in European model variants. Those versions 
operate near 868 MHz amd are advertised as using "ActivLink" technology. I've read 
indications that those models also use the same transmission signal, though on that 
different frequency.

## The Wireless Signal

The Honeywell wireless transmitter I have direct access to is the 
RPWL400W, though I was able to look at three of them. I also have
tested with a Series 9 and Series 3 receiver.

When the wireless doorbell button is pressed, it sends out a 
signal centered at 916.8 MHz. It seems to be using 2FSK 
modulation with a 50 kHz deviation. The modulation rate seems 
to be 6250 baud, so each HIGH or LOW symbol is 160 microseconds 
(μs).

Because it is using digital symbols over 2FSK modulation, it essentially
looks like two separate, simultaneous, out-of-phase OOK transmissions 
100 kHz away from each other, at 916.75 MHz and and 916.85 MHz.

If you choose to visually look at only one of those frequencies as an OOK transmission, 
it may make sense to choose the higher 916.85 MHz frequency, since its phase will 
represent a "LOW" symbol as a low level on the waveform, and a "HIGH" symbol will 
look like a high level.

Data bits are encoded over three symbols. A "0" bit is defined 
as HIGH-LOW-LOW, and a "1" bit is defined as "HIGH-HIGH-LOW".

Each frame of data consists of a LOW-LOW-LOW preamble, 48 bits of
data, and a postamble of HIGH-HIGH-HIGH.

The signal seems to be 50 consecutive repetitions of the frame, then
symbols LOW-LOW-LOW-HIGH-HIGH-HIGH, and finally 2 continuous milliseconds (2000 μs) 
of LOW.

So, the duration of the entire 2FSK signal is 

	(50 reps * ((1 + 48 + 1 bits) * (3 symbols * 160 μs))) + (6 symbols * 160 μs) + 2000 μs
	=
	(2500 * 480 μs) + 960 μs + 2000 μs
	=
	1200000 μs + 2960 μs
	=
	1.202960 seconds

## The Data Frame

Each data frame is 48 bits, or 6 bytes long. With some 
experimentation (using a YARD Stick One to create custom 
signals), here's what I've been able to ascertain about the 
data in the frame.

	# Frame bits used in Honeywell RCWL300A, RCWL330A, Series 3, 5, 9 and all Decor Series Wireless Chimes
	# 0000 0000 0000 0000 1111 1111 1111 1111 2222 2222 2222 2222
	# 0123 4567 89ab cdef 0123 4567 89ab cdef 0123 4567 89ab cdef
	# XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX XX.. XXX. .... KEY DATA (any change and receiver doesn't seem to recognize signal)
	# XXXX XXXX XXXX XXXX XXXX .... .... .... .... .... .... .... KEY ID (different for each transmitter)
	# .... .... .... .... .... 0000 00.. 0000 0000 00.. 000. .... KEY UNKNOWN 0 (always 0 in devices I've tested)
	# .... .... .... .... .... .... ..XX .... .... .... .... .... DEVICE TYPE (10 = doorbell, 01 = PIR Motion sensor)
	# .... .... .... .... .... .... .... .... .... ..XX ...X XXX. FLAG DATA (may be modified for possible effects on receiver)
	# .... .... .... .... .... .... .... .... .... ..XX .... .... ALERT (00 = normal, 01 or 10 = right-left halo light pattern, 11 = full volume alarm)
	# .... .... .... .... .... .... .... .... .... .... ...X .... SECRET KNOCK (0 = default, 1 if doorbell is pressed 3x rapidly)
	# .... .... .... .... .... .... .... .... .... .... .... X... RELAY (1 if signal is a retransmission of a received transmission, only some models)
	# .... .... .... .... .... .... .... .... .... .... .... .X.. FLAG UNKNOWN (0 = default, but 1 is accepted and I don't oberserve any difference)
	# .... .... .... .... .... .... .... .... .... .... .... ..X. LOWBAT (1 if battery is low, receiver gives low battery alert)
	# .... .... .... .... .... .... .... .... .... .... .... ...X PARITY (LSB of count of set bits in previous 23 bits)

### Data Frame RELAY Notes:

On some receiver models, such as the "Honeywell RDWL917AX2000/E 
Series 9 Portable Wireless Doorbell", the base receiver will immediately retransmit a valid
received signal if the RELAY bit is NOT set. The data in the retransmitted signal will
be modified with the RELAY bit set. This seems to be an effort to extend a signal to more
distant receivers.


## Detecting signals using `rtl_433`

This seems to work pretty well to pick up data frames in the signal as 
symbol pulse bits represented in hex. Note that the returned length of these frames
is 149 bits (symbols) because it is decoding the frame's "HIGH-HIGH-HIGH" postamble
into three "1" bits (symbols).

	rtl_433 -f 916800000 -q -X Honeywell:FSK_PCM:160:160:400,bits=149,match={4}0xe

And this seems to work pretty well to pick up the signal frame data as hex. Note that the 
returned length of these frames is 49 bits because it is decoding the frame's "HIGH-HIGH-HIGH" 
postamble as a "1" bit.

	rtl_433 -f 916800000 -q -X Honeywell:FSK_PWM_RAW:240:480:400,bits=49,invert,match={4}0x8
		
Note the "invert" option is specified here to provide a decoding consistent with this document.

This seems to work pretty well to pick up the whole signal with frame data in rows as hex:

	rtl_433 -f 916800000 -q -X Honeywell:FSK_PWM_RAW:240:400:560,bits=49,invert

Note that these don't seem to pick up the all the data frames. In my tests, it only seems to
pick up 24 of the 50 data frames in the signal before triggering 
`pulse_FSK_detect(): Maximum number of pulses reached!` and ignoring the rest of the signal.

It seems to be possible to also pick up the signal as OOK in rtl_433 by setting the frequency
about 90 kHz away from the center of the FSK frequency. This seems to force rtl_433 to only
notice one side of the 2FSK signal. This command seems to work well to pick up the data frames.
It also has the benefit that when the maximum number of pulses is reached, it doesn't trigger
an error and starts decoding data again immediately.

	rtl_433 -f 916890000 -q -X Honeywell:OOK_PWM:160:320:560:400,bits=49,invert

And finally, tuning low and allowing `rtl_433` to guess the demodulation on its own, it will use
`pulse_demod_pwm_precise()` and output a nice representation of the hex and bit values of the
48-bit data frames. However, it does seems to truncate after 24 rows of data frames.

	rtl_433 -f 916710000 -q -R 0 -A


## Transmitting a signal using a Yard Stick One

Convert the signal you want to send as a hex value representing the symbols in the 
signal where a 1 bit is a HIGH symbol and a 0 bit is a LOW symbol.

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

## References

- [rtl_433](https://github.com/merbanan/rtl_433) is an incredible and 
	essential piece of software. I still have much of it to learn.

- The [YARD Stick One](https://greatscottgadgets.com/yardstickone/) is also
	an incredible thing. Transmitting arbitrary digital signals to test out
	signal changes was essential. This made it easy.
	
- [rfcat](https://github.com/atlas0fd00m/rfcat) is also essential to effectively
	tapping into the abilities of the YARD Stick One.
