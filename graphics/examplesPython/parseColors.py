#!/usr/bin/env python
import sys, string, regex, re
from scanfile import scanner

#
# parseColors.py - Pythonize colors.tcl
# 

name = sys.argv[1]
print name
#print '------------------'
input = open(name + '.tcl', 'r')
output = open(name + '.py', 'w')


def processLine(line):
	if line[0] == '#':
		output.write(line)
	else:
		line = re.sub('\"','',line)
		n = len(line)
#		if line[n-1:n] == ;
#			line = line[:-1]

		keys = string.split(line)
		keysLength = len(keys)
#		print 'length(keys) = ',keysLength
		if keysLength == 0:
			output.write("\n")
		elif keysLength == 5:
			output.write(keys[1] + " = (" + keys[2]+","+keys[3]+","+keys[4]+")\n")

for line in input.readlines():
	processLine(line)
