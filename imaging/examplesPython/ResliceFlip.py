#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *
import os

# This example demonstrates how to permute an image with vtkImageReslice.
# The advantage of using vtkImageReslice for this (rather than
# vtkImagePermute) is that you can specify negative or even oblique axes.

reader = vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetDataSpacing(1,1,2)
reader.SetFilePrefix(os.path.join(VTK_DATA, "fullHead/headsq"))
reader.SetDataMask(0x7fff)
# uncomment this to read all the data at once (speeds things up)
reader.Update()

reslice = vtkImageReslice()
reslice.SetInput(reader.GetOutput())
reslice.SetResliceAxesDirectionCosines(-1,0,0, 0,+1,0, 0,0,+1)

viewer = vtkImageViewer()
viewer.SetInput(reslice.GetOutput())
viewer.GetImageWindow().DoubleBufferOn()
viewer.SetZSlice(128)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
#viewer.Render()

#make interface
WindowLevelInterface(viewer)

