#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# This script shows the result of an ideal lowpass filter in frequency space.


# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.DebugOn()

fft = vtkImageFFT()
fft.SetDimensionality(2)
fft.SetInput(reader.GetOutput())
#fft.DebugOn()

lowPass = vtkImageButterworthLowPass()
lowPass.SetInput(fft.GetOutput())
lowPass.SetOrder(2)
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
