#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'
import sys, string

for name in sys.argv[1:]:
	input = open(name + '.tcl', 'r')
	print input.read(),
	print '::::::'
