#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# Example demonstrates use of abstract vtkDataSetToDataSetFilter
# (i.e., vtkElevationFilter - an abstract filter)

sphere = vtkSphereSource()
sphere.SetPhiResolution(12)
sphere.SetThetaResolution(12)

colorIt = vtkElevationFilter()
colorIt.SetInput(sphere.GetOutput())
colorIt.SetLowPoint(0,0,-1)
colorIt.SetHighPoint(0,0,1)

mapper = vtkPolyDataMapper()
mapper.SetInput(colorIt.GetPolyDataOutput())

actor = vtkActor()
actor.SetMapper(mapper)

ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.AddActor(actor)
ren.SetBackground(1,1,1)
renWin.SetSize(400,400)
ren.GetActiveCamera().Zoom(1.4)

iren.Initialize()

iren.Start()
