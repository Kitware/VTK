#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# This Script test the euclidean to polar by coverting 2D vectors 
# from a gradient into polar, which is converted into HSV, and then to RGB.


# Image pipeline

gauss = vtkImageGaussianSource()
gauss.SetWholeExtent(0,255,0,255,0,44)
gauss.SetCenter(127,127,22)
gauss.SetStandardDeviation(50.0)
gauss.SetMaximum(10000.0)

gradient = vtkImageGradient()
gradient.SetInput(gauss.GetOutput())
gradient.SetDimensionality(2)

polar = vtkImageEuclideanToPolar()
polar.SetInput(gradient.GetOutput())
polar.SetThetaMaximum(255)

viewer = vtkImageViewer()
#viewer.DebugOn()
viewer.SetInput(polar.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)


#make interface
WindowLevelInterface(viewer)
