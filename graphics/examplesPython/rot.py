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
renWin.SetSize(400,400)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

testsource = vtkAxes()
originsource = vtkAxes()
test = vtkActor()
origin = vtkActor()
originmapper = vtkPolyDataMapper()
testmapper = vtkPolyDataMapper()

originsource.SetScaleFactor(4000.0)
originmapper.SetInput(originsource.GetOutput())
origin.SetMapper(originmapper)
origin.GetProperty().SetAmbient(1.0)
origin.GetProperty().SetDiffuse(0.0)
ren.AddActor(origin)

testsource.SetScaleFactor(2000.0)
testmapper.SetInput(testsource.GetOutput())
test.SetMapper(testmapper)
test.GetProperty().SetAmbient(1.0)
test.GetProperty().SetDiffuse(0.0)
ren.AddActor(test)

test.SetPosition(0.0,1500.0,0.0)
renWin.Render()

# do test rotations and renderings:
test.RotateX(15.0)
renWin.Render()

test.RotateZ(30.0)
renWin.Render()

test.RotateY(45.0)
renWin.Render()


iren.Start()
