#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# This script shows the result of an ideal highpass filter in  spatial domain

# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.DebugOn()

fft = vtkImageFFT()
fft.SetFilteredAxes(VTK_IMAGE_X_AXIS,VTK_IMAGE_Y_AXIS)
fft.SetInput(reader.GetOutput())
#fft.DebugOn()

highPass = vtkImageIdealHighPass()
highPass.SetInput(fft.GetOutput())
highPass.SetXCutOff(0.1)
highPass.SetYCutOff(0.1)
highPass.ReleaseDataFlagOff()
#highPass.DebugOn()

rfft = vtkImageRFFT()
rfft.SetFilteredAxes(VTK_IMAGE_X_AXIS,VTK_IMAGE_Y_AXIS)
rfft.SetInput(highPass.GetOutput())
#fft.DebugOn()

real = vtkImageExtractComponents()
real.SetInput(rfft.GetOutput())
real.SetComponents(0)


viewer = vtkImageViewer()
viewer.SetInput(real.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(500)
viewer.SetColorLevel(0)
#viewer.DebugOn()


# make interface
WindowLevelInterface(viewer)
