#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# Test the median filter.


# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.DebugOn()

median = vtkImageMedian3D()
median.SetInput(reader.GetOutput())
median.SetKernelSize(7,7,1)
median.ReleaseDataFlagOff()


viewer = vtkImageViewer()
#viewer.DebugOn()
viewer.SetInput(median.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)

# make interface
WindowLevelInterface(viewer)
