#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# Tst the OpenClose3D filter.


# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.DebugOn()

thresh = vtkImageThreshold()
thresh.SetInput(reader.GetOutput())
thresh.SetOutputScalarTypeToUnsignedChar()
thresh.ThresholdByUpper(2000.0)
thresh.SetInValue(255)
thresh.SetOutValue(0)
thresh.ReleaseDataFlagOff()

my_close = vtkImageOpenClose3D()
my_close.SetInput(thresh.GetOutput())
my_close.SetOpenValue(0)
my_close.SetCloseValue(255)
my_close.SetKernelSize(5,5,3)
my_close.ReleaseDataFlagOff()
# for coverage (we could compare results to see if they are correct).
my_close.DebugOn()
my_close.DebugOff()
my_close.GetOutput()
my_close.GetCloseValue()
my_close.GetOpenValue()

viewer = vtkImageViewer()
viewer.SetInput(my_close.GetOutput())
viewer.SetZSlice(2)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)


# make interface
WindowLevelInterface(viewer)
