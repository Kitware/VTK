#!/usr/local/bin/python

from vtkpython import *
from WindowLevelInterface import *

# Doubles the size of the image in the X and Y dimensions.


# Image pipeline

reader = vtkImageReader()
#reader DebugOn
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix("../../../vtkdata/fullHead/headsq")
reader.SetDataMask(0x7fff)

magnify = vtkImageMagnify()
magnify.SetInput(reader.GetOutput())
magnify.SetMagnificationFactors(2,2,1)
magnify.InterpolateOn()

viewer = vtkImageViewer()
#viewer.DebugOn()
viewer.SetInput(magnify.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(3000)
viewer.SetColorLevel(1500)


#make interface
WindowLevelInterface(viewer)
