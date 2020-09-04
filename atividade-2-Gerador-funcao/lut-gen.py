#!/bin/python

import math as m
expr_start = "{\n"
expr_end = "\n }"

sin_lut = expr_start
triang_lut = expr_start
sawtooth_lut = expr_start

lut_len = 100
sig_ampl = 255;

def break_str(str):
	""" break string into 80 width lines"""
	if (len(str) % 80 < 5):
		str += "\n"

	return str;

fmt = " {v},"
for i in range(0,lut_len):
	f = float(i)/(lut_len);
	#print(f);

	if(i == lut_len - 1):
		fmt = " {v}"

	#print("*"*int(f*sig_ampl))
	
	sin_lut += fmt.format(v=int((0.5+0.5*m.sin(2*m.pi*f))*sig_ampl))
	triang_lut += fmt.format(v = int(sig_ampl*(f*2) if f < 0.5 else sig_ampl*((1-f)*2)))
	sawtooth_lut += fmt.format(v = int(f*sig_ampl))

	sin_lut = break_str(sin_lut);
	triang_lut = break_str(triang_lut);
	sawtooth_lut = break_str(sawtooth_lut);

sin_lut += expr_end
triang_lut += expr_end
sawtooth_lut += expr_end

#print out results
print("static const uint8_t sine_lut[LUT_LEN] PROGMEM = \n {v};".format(l=lut_len,v=sin_lut))
print("static const uint8_t trgl_lut[LUT_LEN] PROGMEM = \n {v};".format(l=lut_len,v=triang_lut))
print("static const uint8_t swtt_lut[LUT_LEN] PROGMEM = \n {v};".format(l=lut_len,v=sawtooth_lut))
