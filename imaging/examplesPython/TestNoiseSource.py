#!/usr/local/bin/python

from vtkpython import *
from WindowLevelInterface import *

# A script to test the NoiseSource


# Image pipeline

noise = vtkImageNoiseSource()
noise.SetWholeExtent(0,225,0,225,0,20)
noise.SetMinimum(0.0)
noise.SetMaximum(255.0)

viewer = vtkImageViewer()
viewer.SetInput(noise.GetOutput())
viewer.SetZSlice(10)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
#viewer.DebugOn()

# make interface
WindowLevelInterface(viewer)
