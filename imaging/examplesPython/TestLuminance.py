#!/usr/local/bin/python

from vtkpython import *
from WindowLevelInterface import *

# This script calculates the luminanace of an image



# Image pipeline

image = vtkBMPReader()
image.SetFileName("../../../vtkdata/beach.bmp")

luminance = vtkImageLuminance()
luminance.SetInput(image.GetOutput())

viewer = vtkImageViewer()
viewer.SetInput(luminance.GetOutput())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
#viewer.DebugOn()


#make interface
WindowLevelInterface(viewer)
