#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *


# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

sr = vtkSTLReader()
sr.SetFileName(VTK_DATA + "/42400-IDGH.stl")

stlMapper = vtkPolyDataMapper()
stlMapper.SetInput(sr.GetOutput())
stlActor = vtkLODActor()
stlActor.SetMapper(stlMapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(stlActor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(500,500)

# render the image
#
cam1=ren.GetActiveCamera()
cam1.Zoom(1.4)
iren.Initialize()

iren.Start()
