#!/usr/local/bin/python

from vtkpython import *
from WindowLevelInterface import *


# Image pipeline

reader = vtkImageReader()
#reader.DebugOn()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix("../../../vtkdata/fullHead/headsq")
reader.SetDataMask(0x7fff)

gradient = vtkImageGradientMagnitude()
gradient.SetDimensionality(3)
gradient.SetInput(reader.GetOutput())

viewer = vtkImageViewer()
#viewer.DebugOn()
viewer.SetInput(gradient.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(1000)
viewer.SetColorLevel(500)


# make interface
WindowLevelInterface(viewer)
