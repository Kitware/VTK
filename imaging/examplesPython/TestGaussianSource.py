#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# A script to test the GaussianSource


# Image pipeline

gauss = vtkImageGaussianSource()
gauss.SetWholeExtent(0,225,0,225,0,20)
gauss.SetCenter(100,100,10)
gauss.SetStandardDeviation(100.0)
gauss.SetMaximum(255.0)
gauss.ReleaseDataFlagOff()

viewer = vtkImageViewer()
viewer.SetInput(gauss.GetOutput())
viewer.SetZSlice(10)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
#viewer.DebugOn()

# make interface
WindowLevelInterface(viewer)
