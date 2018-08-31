# honeywell-wireless-doorbell

----
## What is this?
I started with a Honeywell wireless doorbell, a RTL-SDR, and a 
desire to figure things out.  I'm coming into this with no 
background in SDR or signal processing, so I'm sure there's 
inaccuracies below.

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
frame begins with LOW-LOW-LOW and ends with HIGH-HIGH-HIGH.  
The complete signal itself seems to begin with HIGH-HIGH-HIGH 
and end with LOW-LOW-LOW.

# Understanding The Frame Data

Each data frame is 48 bits, or 6 bytes long. With some 
experimentation (using a YARD Stick One to create custom 
signals), here's what I've been able to determine about the 
data in the frame.

	# Frame bits used in Honeywell RCWL300A, RCWL330A, Series 3, 5, 9 and all Decor Series Wireless Chimes
	# 0000 0000 0000 0000 1111 1111 1111 1111 2222 2222 2222 2222
	# 0123 4567 89ab cdef 0123 4567 89ab cdef 0123 4567 89ab cdef
	# .... .... .... .... .... .... .... .... .... .... .... ...X CHKSUM (LSB of count of set bits in previous 23 bits)
	# .... .... .... .... .... .... .... .... .... .... .... ..X. LOWBAT (1 if battery is low, receiver gives low battery alert)
	# .... .... .... .... .... .... .... .... .... .... .... .X.. IGNORED (0 = default, but 1 is accepted and I don't oberserve any difference)
	# .... .... .... .... .... .... .... .... .... .... .... X... RELAY (1 if signal is a retransmission of a received transmission, only some models)
	# .... .... .... .... .... .... .... .... .... .... ...X .... SECRET KNOCK (if doorbell is pressed 3x rapidly)
	# .... .... .... .... .... .... .... .... .... ..XX .... .... ALERT (00 = normal, 01 or 10 = right-left light pattern, 11 = full volume alarm)
	# XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX XX.. XXX. .... KEY (any change and receiver doesn't recognize signal)
	# XXXX XXXX XXXX XXXX XXXX .... .... .... .... .... .... .... KEY ID (different for each transmitter)
	# .... .... .... .... .... .... ..XX .... .... .... .... .... DEVICE TYPE (10 = doorbell, 01 = PIR Motion sensor)
	# .... .... .... .... .... 0000 00.. 0000 0000 00.. 000. .... KEY UNKNOWN 0 (always 0 in devices I've tested)
