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
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)

imageFloat = vtkImageCast()
imageFloat.SetInput(reader.GetOutput())
imageFloat.SetOutputScalarTypeToFloat()

flipNO = vtkImageFlip()
flipNO.SetInput(imageFloat.GetOutput())
flipNO.BypassOn()

flipX = vtkImageFlip()
flipX.SetInput(imageFloat.GetOutput())
flipX.SetFilteredAxes(VTK_IMAGE_X_AXIS)

flipY = vtkImageFlip()
flipY.SetInput(imageFloat.GetOutput())
flipY.SetFilteredAxes(VTK_IMAGE_Y_AXIS)

append = vtkImageAppend()
append.AddInput(flipNO.GetOutput())
append.AddInput(flipX.GetOutput())
append.AddInput(flipY.GetOutput())
append.SetAppendAxis(VTK_IMAGE_X_AXIS)

#flip.BypassOn()
#flip.PreserveImageExtentOn()

viewer = vtkImageViewer()
viewer.SetInput(append.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)

#make interface
WindowLevelInterface(viewer)
