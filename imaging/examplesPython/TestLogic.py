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

xor = vtkImageLogic()
xor.SetInput1(sphere1.GetOutput())
xor.SetInput2(sphere2.GetOutput())
xor.SetOutputTrueValue(150)
xor.SetOperationToXor()

viewer = vtkImageViewer()
viewer.SetInput(xor.GetOutput())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)

# make interface
WindowLevelInterface(viewer)
