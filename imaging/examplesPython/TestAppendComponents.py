#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# append multiple displaced spheres into an RGB image.

# Image pipeline

sphere1 = vtkImageEllipsoidSource()
sphere1.SetCenter(95,100,0)
sphere1.SetRadius(70,70,70)

sphere2 = vtkImageEllipsoidSource()
sphere2.SetCenter(161,100,0)
sphere2.SetRadius(70,70,70)

sphere3 = vtkImageEllipsoidSource()
sphere3.SetCenter(128,160,0)
sphere3.SetRadius(70,70,70)

append = vtkImageAppendComponents()
append.AddInput(sphere3.GetOutput())
append.AddInput(sphere1.GetOutput())
append.AddInput(sphere2.GetOutput())

viewer = vtkImageViewer()
viewer.SetInput(append.GetOutput())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)

# make interface
WindowLevelInterface(viewer)
