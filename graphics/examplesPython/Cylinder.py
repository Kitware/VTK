#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# this is a Python version of the Mace example

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

cone = vtkCylinderSource()
coneMapper = vtkPolyDataMapper()
coneMapper.SetInput(cone.GetOutput())
coneActor = vtkActor()
coneActor.SetMapper(coneMapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(coneActor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(450,450)

# Get handles to some useful objects
#
iren.Initialize()
renWin.Render()

coneProp=coneActor.GetProperty()

iren.Start()
