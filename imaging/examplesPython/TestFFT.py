#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# This scripts shows the real component of an image in frequencey space.


VTK_FLOAT = 1
VTK_INT = 2
VTK_SHORT = 3
VTK_UNSIGNED_SHORT = 4
VTK_UNSIGNED_CHAR = 5

VTK_IMAGE_X_AXIS = 0
VTK_IMAGE_Y_AXIS = 1
VTK_IMAGE_Z_AXIS = 2
VTK_IMAGE_TIME_AXIS = 3
VTK_IMAGE_COMPONENT_AXIS = 4


# Image pipeline

reader = vtkImageReader()
reader.GetOutput().ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.DebugOn()

fft = vtkImageFFT()
fft.SetDimensionality(2)
fft.SetInput(reader.GetOutput())
fft.ReleaseDataFlagOff()
#fft.DebugOn()

viewer = vtkImageViewer()
viewer.SetInput(fft.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(10000)
viewer.SetColorLevel(4000)


WindowLevelInterface(viewer)
