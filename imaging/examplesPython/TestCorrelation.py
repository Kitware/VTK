#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# Show the constant kernel.  Smooth an impulse function.


s1 = vtkImageCanvasSource2D()
s1.SetScalarType(VTK_FLOAT)
s1.SetExtent(0,255,0,255,0,0)
s1.SetDrawColor(0)
s1.FillBox(0,255,0,255)
s1.SetDrawColor(2.0)
s1.FillTriangle(10,100,190,150,40,250)

s2 = vtkImageCanvasSource2D()
s2.SetScalarType(VTK_FLOAT)
s2.SetExtent(0,31,0,31,0,0)
s2.SetDrawColor(0.0)
s2.FillBox(0,31,0,31)
s2.SetDrawColor(2.0)
s2.FillTriangle(10,1,25,10,1,5)


convolve = vtkImageCorrelation()
convolve.SetDimensionality(2)
convolve.SetInput1(s1.GetOutput())
convolve.SetInput2(s2.GetOutput())

viewer = vtkImageViewer()
viewer.SetInput(convolve.GetOutput())
viewer.SetColorWindow(256)
viewer.SetColorLevel(127.5)


# make interface
WindowLevelInterface(viewer)
