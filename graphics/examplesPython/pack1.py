#!/usr/people/heiland/Python/Python-1.5.1/python
import sys, string

for name in sys.argv[1:]:
	input = open(name + '.tcl', 'r')
	print input.read(),
	print '::::::'
