#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# This script subtracts the 2D laplacian from an image to enhance the edges.

# Image pipeline

reader = vtkImageReader()
#reader.DebugOn()
reader.GetOutput().ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)

cast = vtkImageCast()
cast.SetInput(reader.GetOutput())
cast.SetOutputScalarTypeToFloat()

lap = vtkImageLaplacian()
lap.SetInput(cast.GetOutput())
lap.SetDimensionality(2)

subtract = vtkImageMathematics()
subtract.SetOperationToSubtract()
subtract.SetInput1(cast.GetOutput())
subtract.SetInput2(lap.GetOutput())
subtract.ReleaseDataFlagOff()
#subtract.BypassOn()

viewer = vtkImageViewer()
#viewer.DebugOn()
viewer.SetInput(subtract.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)


# make interface
WindowLevelInterface(viewer)
