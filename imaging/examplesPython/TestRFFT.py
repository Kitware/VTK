#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# This scripts the reverse FFT. Pipeline is Reader->FFT->RFFT->Viewer.
# Output should be the same as Reader.


# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader DebugOn

fft = vtkImageFFT()
fft.SetDimensionality(2)
fft.SetInput(reader.GetOutput())
#fft.DebugOn()

rfft = vtkImageRFFT()
rfft.SetDimensionality(2)
rfft.SetInput(fft.GetOutput())
rfft.ReleaseDataFlagOff()
#rfft.DebugOn()


viewer = vtkImageViewer()
viewer.SetInput(rfft.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
#viewer.DebugOn()


# make interface
WindowLevelInterface(viewer)
