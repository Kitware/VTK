#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *


# Image pipeline

reader = vtkImageReader()
#reader.DebugOn()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)

gradient = vtkImageGradientMagnitude()
gradient.SetDimensionality(3)
gradient.SetInput(reader.GetOutput())

viewer = vtkImageViewer()
#viewer.DebugOn()
viewer.SetInput(gradient.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(1000)
viewer.SetColorLevel(500)


# make interface
WindowLevelInterface(viewer)
