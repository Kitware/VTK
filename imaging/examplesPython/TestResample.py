#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# Doubles the The number of images (x dimension).



# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)
reader.ReleaseDataFlagOff()
#reader.DebugOn()

magnify = vtkImageResample()
magnify.SetDimensionality(3)
magnify.SetInput(reader.GetOutput())
magnify.SetAxisOutputSpacing(VTK_IMAGE_X_AXIS,0.52)
magnify.SetAxisOutputSpacing(VTK_IMAGE_Y_AXIS,2.2)
magnify.SetAxisOutputSpacing(VTK_IMAGE_Z_AXIS,0.8)
magnify.ReleaseDataFlagOff()


viewer = vtkImageViewer()
viewer.SetInput(magnify.GetOutput())
viewer.SetZSlice(30)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)

# make interface
WindowLevelInterface(viewer)
