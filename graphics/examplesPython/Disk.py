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

disk = vtkDiskSource()
disk.SetInnerRadius(1.0)
disk.SetOuterRadius(2.0)
disk.SetRadialResolution(1)
disk.SetCircumferentialResolution(20)

diskMapper = vtkPolyDataMapper()
diskMapper.SetInput(disk.GetOutput())
diskActor = vtkActor()
diskActor.SetMapper(diskMapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(diskActor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(450,450)

iren.Initialize()
renWin.Render()

iren.Start()
