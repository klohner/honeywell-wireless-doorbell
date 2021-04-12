#!/usr/bin/env python
# usage from rfcat interactive shell:
#   %run honeywell_doorbell.py
#   hw_tx()

import sys
from rflib import *

default_key_id_hex = '8BFA3' # Set this to be the ID of your doorbell.

def rf_LO():
	return "0"

def rf_HI():
	return "1"

def pwm_0():
	return "{1}{0}{0}".format(rf_LO(), rf_HI())

def pwm_1():
	return "{1}{1}{0}".format(rf_LO(), rf_HI())

def pwm_packet_start():
	return "{0}{0}{0}".format(rf_LO(), rf_HI())

def pwm_packet_end():
	return "{1}{1}{1}".format(rf_LO(), rf_HI())

def bits_to_bytes(bit_str, pad_bit="0", align=">"):
	bit_len = len(bit_str)
	byte_len = bit_len // 8 + (bit_len % 8 > 0)
	bit_str = "{:{}{}{}}".format(bit_str, pad_bit, align, byte_len*8)
	byte_arr=bytearray()
	while(byte_len):
		byte_arr.insert(0,int(bit_str[-8:],2))
		bit_str = bit_str[:-8]
		byte_len -= 1
	return bytes(byte_arr)

def hex_to_bits(hex_str):
	hex_len = len(hex_str)
	bit_len = hex_len * 4
	return "{:0>{}b}".format(int(hex_str,16), bit_len)

def bits_to_hex(bit_str, pad_bit="0", align="<"):
	bit_len = len(bit_str)
	hex_len = bit_len // 4 + (bit_len % 4 > 0)
	bit_str = "{:{}{}{}}".format(bit_str, pad_bit, align, hex_len*4)
	return "{:0>{}x}".format(int(bit_str,2), pad_bit, hex_len)

def bits_to_pwm(bit_str):
	pwm_str = ""
	for bit in bit_str:
		pwm_bits = "*"
		if(bit == "0"):
			pwm_bits = pwm_0() #  A zero is encoded as a longer high pulse (high-low-low)
		if(bit == "1"):
			pwm_bits = pwm_1() # and a one is encoded as a shorter high pulse (high-high-low).
		pwm_str = "%s%s" % (pwm_str, pwm_bits)
	return pwm_str

def hex_key_to_pwm_bit_packet(hex_key_str):
	pwm_bits = "%s%s%s" % (pwm_packet_start(), bits_to_pwm(hex_to_bits(hex_key_str)), pwm_packet_end())
	return pwm_bits

def make_honeywell_id(key_id, secret_knock, alert, lowbat, relay):
	id_bit_len = 48
	id_bit_arr = bytearray("{:0<{}}".format(hex_to_bits(key_id),id_bit_len))
	if secret_knock is not None:
		id_bit_arr[0x2b:0x2c]="{:0>1b}".format(secret_knock)[-1:]
	if alert is not None:
		id_bit_arr[0x26:0x28]="{:0>2b}".format(alert)[-2:]
	if lowbat is not None:
		id_bit_arr[0x2e:0x2f]="{:0>1b}".format(lowbat)[-1:]
	if relay is not None:
		id_bit_arr[0x2c:0x2d]="{:0>1b}".format(relay)[-1:]
	# Key Unknown 1
	id_bit_arr[0x1a:0x1b]="1"
	# Calculate checksum
	id_bit_arr[0x2f:0x30]="{:0>1b}".format(id_bit_arr[0x00:0x2f].count('1')%2)[-1:]
	return bits_to_hex(bytes(id_bit_arr))

def hw_config(device=d):
	device.setFreq(916800000)
	device.setMdmModulation(MOD_2FSK)
	device.setMdmDeviatn(50000)
	device.setMdmSyncMode(0)
	device.setMdmDRate(6250)
	device.setMaxPower()

def hw_tx(key_id_hex=default_key_id_hex, secret_knock=0, alert=0, lowbat=0, relay=0, burst=50):
	hw_config(d)
	honeywell_id = make_honeywell_id(key_id_hex, secret_knock, alert, lowbat, relay)	
	print "Honeywell TX key: %s x %i" % (honeywell_id, burst)
	pwm_burst_bytes = bits_to_bytes(hex_key_to_pwm_bit_packet(honeywell_id)*burst)
	d.RFxmit(data=pwm_burst_bytes)
	print "Done."
