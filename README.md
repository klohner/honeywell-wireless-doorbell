# honeywell-wireless-doorbell

## What is this?
An attempt to capture and decode the signals used by the
Honeywell RCWL300A, RCWL330A, Series 3, 5, 9 and Decor Series Wireless Chimes.

I'm also attempting to transmit valid signals using the Yard Stick One.

## The Wireless Signal
The Honeywell wireless transmitter I have direct access to is the 
RPWL400W, though I was able to look at three of them.

When the wireless doorbell button is pressed, it sends out a 
signal centered at 916.8 MHz. It seems to be using 2FSK 
modulation with a 50 kHz deviation. The modulation rate seems 
to be 6250 baud, so each HIGH or LOW symbol is 160 microseconds 
(μs).

Data bits are encoded over three symbols.  A "0" bit is defined 
as HIGH-LOW-LOW, and a "1" bit is defined as "HIGH-HIGH-LOW".

The signal seems to be 50 repetitions of a 48-bit frame, each 
frame begins with preamble of LOW-LOW-LOW and ends with postamble
of HIGH-HIGH-HIGH.  

The complete signal then seems to end with LOW-LOW-LOW, HIGH-HIGH-HIGH.

## The Data Frame

Each data frame is 48 bits, or 6 bytes long. With some 
experimentation (using a YARD Stick One to create custom 
signals), here's what I've been able to determine about the 
data in the frame.

	# Frame bits used in Honeywell RCWL300A, RCWL330A, Series 3, 5, 9 and all Decor Series Wireless Chimes
	# 0000 0000 0000 0000 1111 1111 1111 1111 2222 2222 2222 2222
	# 0123 4567 89ab cdef 0123 4567 89ab cdef 0123 4567 89ab cdef
	# XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX XX.. XXX. .... KEY DATA (any change and receiver doesn't seem to recognize signal)
	# XXXX XXXX XXXX XXXX XXXX .... .... .... .... .... .... .... KEY ID (different for each transmitter)
	# .... .... .... .... .... 0000 00.. 0000 0000 00.. 000. .... KEY UNKNOWN 0 (always 0 in devices I've tested)
	# .... .... .... .... .... .... ..XX .... .... .... .... .... DEVICE TYPE (10 = doorbell, 01 = PIR Motion sensor)
	# .... .... .... .... .... .... .... .... .... ..XX ...X XXX. FLAG DATA (may be changed for possible effects on receiver)
	# .... .... .... .... .... .... .... .... .... ..XX .... .... ALERT (00 = normal, 01 or 10 = right-left halo light pattern, 11 = full volume alarm)
	# .... .... .... .... .... .... .... .... .... .... ...X .... SECRET KNOCK (if doorbell is pressed 3x rapidly)
	# .... .... .... .... .... .... .... .... .... .... .... X... RELAY (1 if signal is a retransmission of a received transmission, only some models)
	# .... .... .... .... .... .... .... .... .... .... .... .X.. FLAG UNKNOWN (0 = default, but 1 is accepted and I don't oberserve any difference)
	# .... .... .... .... .... .... .... .... .... .... .... ..X. LOWBAT (1 if battery is low, receiver gives low battery alert)
	# .... .... .... .... .... .... .... .... .... .... .... ...X CHKSUM (LSB of count of set bits in previous 23 bits)

### Data Frame RELAY Notes:

On some receiver models, such as the "Honeywell RDWL917AX2000/E 
Series 9 Portable Wireless Doorbell", the base receiver will immediately retransmit a valid
received signal if the RELAY bit is NOT set.  The data in the retransmitted signal will
be modified with the RELAY bit set.  This seems to be an effort to extend a signal to more
distant receivers.

## Detecting signals using rtl_433

This seems to work pretty well to pick up every other data frame in the signal as 
symbol pulse bits represented in hex:

	rtl_433 -f 916800000 -q -X "Honeywell:FSK_PCM:160:160:400,bits=149,match={4}0xe"

And this seems to work pretty well to pick up the signal frame data as hex:

	rtl_433 -f 916800000 -q -X "Honeywell:FSK_PWM_RAW:240:480:400,bits=49,invert,match={4}0x8"

Note that these don't seem to pick up the all the data frames.  In my tests, it only seems to
pick up 24 of the 50 data frames in the signal.

It seems like rtl_433 is looking for the "LOW-LOW-LOW" preamble as the gap between frames, then 
becomes out of sync and not receiving the next frame properly, then only syncing back up to get the
subsequent frame.  I think it's also completely missing either the first or last frame as well.
I need to do more testing to confirm.

## Transmitting the signal using a Yard Stick One

Convert the signal you want to send as a hex value representing the symbols in the signal where
a 1 is a HIGH symbol and a 0 is a LOW symbol.

	device.setFreq(916800000)
	device.setMdmModulation(MOD_2FSK)
	device.setMdmDeviatn(50000)
	device.setMdmSyncMode(0)
	device.setMdmDRate(6250)
	device.setMaxPower()
	device.RFxmit(data=pwm_signal_bytes)