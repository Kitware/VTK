#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# This scripts shows the real component of an image in frequencey space.


# Image pipeline

reader = vtkImageReader()
reader.GetOutput().ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)

fft = vtkImageFFT()
fft.SetDimensionality(2)
fft.SetInput(reader.GetOutput())
fft.ReleaseDataFlagOff()
#fft.DebugOn()

center = vtkImageFourierCenter()
center.SetInput(fft.GetOutput())
center.SetDimensionality(2)

viewer = vtkImageViewer()
viewer.SetInput(center.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(10000)
viewer.SetColorLevel(4000)


WindowLevelInterface(viewer)
