#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# This script shows the magnitude of an image in frequency domain.




# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,0,92)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)

gradient = vtkImageGradient()
gradient.SetInput(reader.GetOutput())
gradient.SetDimensionality(3)
#gradient.DebugOn()

magnitude = vtkImageMagnitude()
magnitude.SetInput(gradient.GetOutput())

viewer = vtkImageViewer()
viewer.SetInput(magnitude.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(1000)
viewer.SetColorLevel(200)
#viewer.DebugOn()


#make interface
WindowLevelInterface(viewer)
