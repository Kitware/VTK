#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# This script is for testing the 3d NonMaximumSuppressionFilter.
# The filter works exclusively on the output of the gradient filter.
# The effect is to pick the peaks of the gradient creating thin surfaces.




# Image pipeline
reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.DebugOn()

gradient = vtkImageGradient()
gradient.SetDimensionality(2)
gradient.SetInput(reader.GetOutput())

magnitude = vtkImageMagnitude()
magnitude.SetInput(gradient.GetOutput())

suppress = vtkImageNonMaximumSuppression()
suppress.SetVectorInput(gradient.GetOutput())
suppress.SetMagnitudeInput(magnitude.GetOutput())
suppress.SetDimensionality(2)

viewer = vtkImageViewer()
viewer.SetInput(suppress.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(1000)
viewer.SetColorLevel(500)


# make interface
WindowLevelInterface(viewer)
