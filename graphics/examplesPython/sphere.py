#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# Generate implicit model of a sphere
#
from colors import *
# Create renderer stuff
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
sphere = vtkSphere()
sphere.SetRadius(1)
sample = vtkSampleFunction()
sample.SetImplicitFunction(sphere)
iso = vtkContourFilter()
iso.SetInput(sample.GetOutput())
iso.SetValue(0,0.0)
isoMapper = vtkPolyDataMapper()
isoMapper.SetInput(iso.GetOutput())
isoMapper.ScalarVisibilityOff()
isoActor = vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(peacock[0],peacock[1],peacock[2])

outline = vtkOutlineFilter()
outline.SetInput(sample.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(isoActor)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)
iren.Initialize()

# render the image
#




iren.Start()
