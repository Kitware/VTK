#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# A script to test the SinusoidSource


# Image pipeline

cos = vtkImageSinusoidSource()
cos.SetWholeExtent(0,225,0,225,0,20)
cos.SetAmplitude(250)
cos.SetDirection(1,1,1)
cos.SetPeriod(20)
cos.ReleaseDataFlagOff()

viewer = vtkImageViewer()
viewer.SetInput(cos.GetOutput())
viewer.SetZSlice(10)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
#viewer.DebugOn()

# make interface
WindowLevelInterface(viewer)
