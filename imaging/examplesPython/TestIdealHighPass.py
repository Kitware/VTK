#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# This script shows the result of an ideal highpass filter in frequency space.

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
highPass.SetXCutOff(0.2)
highPass.SetYCutOff(0.1)
highPass.ReleaseDataFlagOff()
#highPass.DebugOn()

viewer = vtkImageViewer()
viewer.SetInput(highPass.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(10000)
viewer.SetColorLevel(5000)
#viewer.DebugOn()


# make interface
WindowLevelInterface(viewer)
