#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# This script is for testing the Nd Gaussian Smooth filter.



# Image pipeline

reader = vtkImageReader()
#reader.DebugOn()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)

smooth = vtkImageGaussianSmooth()
smooth.SetInput(reader.GetOutput())
smooth.SetDimensionality(2)
smooth.SetStandardDeviations(2,10)

viewer = vtkImageViewer()
#viewer.DebugOn()
viewer.SetInput(smooth.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)


# make interface
WindowLevelInterface(viewer)
