#!/usr/local/bin/python

from vtkpython import *
from WindowLevelInterface import *

# This script shows the magnitude of an image in frequency domain.




# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,0,92)
reader.SetFilePrefix("../../../vtkdata/fullHead/headsq")
reader.SetDataMask(0x7fff)

gradient = vtkImageGradient()
gradient.SetInput(reader.GetOutput())
gradient.SetDimensionality(3)
#gradient.DebugOn()

magnitude = vtkImageMagnitude()
magnitude.SetInput(gradient.GetOutput())

viewer = vtkImageViewer()
viewer.SetInput(magnitude.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(1000)
viewer.SetColorLevel(200)
#viewer.DebugOn()


#make interface
WindowLevelInterface(viewer)
