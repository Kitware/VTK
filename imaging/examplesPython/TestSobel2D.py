#!/usr/local/bin/python

from vtkpython import *
from WindowLevelInterface import *

# This script is for testing the 3D Sobel filter.
# Displays the 3 components using color.


# Image pipeline

reader = vtkImageReader()
#reader.DebugOn()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix("../../../vtkdata/fullHead/headsq")
reader.SetDataMask(0x7fff)

sobel = vtkImageSobel2D()
sobel.SetInput(reader.GetOutput())
sobel.ReleaseDataFlagOff()

viewer = vtkImageViewer()
#viewer.DebugOn()
viewer.SetInput(sobel.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(400)
viewer.SetColorLevel(0)


# make interface
WindowLevelInterface(viewer)
