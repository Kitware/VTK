#!/usr/local/bin/python

from vtkpython import *
from WindowLevelInterface import *

# Divergence measures rate of change of gradient.


# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix("../../../vtkdata/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.DebugOn()

gradient = vtkImageGradient()
gradient.SetDimensionality(3)
gradient.SetInput(reader.GetOutput())

derivative = vtkImageDivergence()
derivative.SetDimensionality(3)
derivative.SetInput(gradient.GetOutput())

viewer = vtkImageViewer()
viewer.SetInput(derivative.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(1000)
viewer.SetColorLevel(0)


# make interface
WindowLevelInterface(viewer)
