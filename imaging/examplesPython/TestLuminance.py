#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# This script calculates the luminanace of an image



# Image pipeline

image = vtkBMPReader()
image.SetFileName(VTK_DATA + "/beach.bmp")

luminance = vtkImageLuminance()
luminance.SetInput(image.GetOutput())

viewer = vtkImageViewer()
viewer.SetInput(luminance.GetOutput())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
#viewer.DebugOn()


#make interface
WindowLevelInterface(viewer)
