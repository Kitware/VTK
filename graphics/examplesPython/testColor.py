#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'
def printColor(r,g,b):
	print 'r,g,b= ',r,g,b

printColor(1,0,0)
red = 1,0,0
r,g,b = 1,0,0
printColor(r,g,b)
