#!/usr/local/bin/python

from vtkpython import *
from WindowLevelInterface import *

# This script shows the result of an ideal lowpass filter in frequency space.


# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix("../../../vtkdata/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.DebugOn()

fft = vtkImageFFT()
fft.SetFilteredAxes(VTK_IMAGE_X_AXIS,VTK_IMAGE_Y_AXIS)
fft.SetInput(reader.GetOutput())
#fft.DebugOn()

lowPass = vtkImageIdealLowPass()
lowPass.SetInput(fft.GetOutput())
lowPass.SetXCutOff(0.2)
lowPass.SetYCutOff(0.1)
lowPass.ReleaseDataFlagOff()
#lowPass.DebugOn()

viewer = vtkImageViewer()
viewer.SetInput(lowPass.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(10000)
viewer.SetColorLevel(5000)
#viewer.DebugOn()


# make interface
WindowLevelInterface(viewer)
