#!/usr/local/bin/python

from vtkpython import *
from WindowLevelInterface import *

# This script is for testing the 2d Gradient filter.
# It only displays the first component (0) which contains
# the magnitude of the gradient.



# Image pipeline

reader = vtkImageReader()
#reader.DebugOn()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix("../../../vtkdata/fullHead/headsq")
reader.SetDataMask(0x7fff)

gradient = vtkImageGradient()
gradient.SetInput(reader.GetOutput())
gradient.SetDimensionality(3)

viewer = vtkImageViewer()
#viewer.DebugOn()
viewer.SetInput(gradient.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(400)
viewer.SetColorLevel(0)


#make interface
WindowLevelInterface(viewer)
