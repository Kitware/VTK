#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

## Test the rotational extrusion filter and tube generator. Sweep a spiral
## curve to generate a tube.


# create bottle profile
#
# draw the arrows
points = vtkPoints()
points.InsertNextPoint(1,0,0)
points.InsertNextPoint(0.707,0.707,1)
points.InsertNextPoint(0,1,2)
points.InsertNextPoint(-0.707,0.707,3)
points.InsertNextPoint(-1,0,4)
points.InsertNextPoint(-0.707,-0.707,5)
points.InsertNextPoint(0,-1,6)
points.InsertNextPoint(0.707,-0.707,7)
lines = vtkCellArray()
lines.InsertNextCell(8)
lines.InsertCellPoint(0)
lines.InsertCellPoint(1)
lines.InsertCellPoint(2)
lines.InsertCellPoint(3)
lines.InsertCellPoint(4)
lines.InsertCellPoint(5)
lines.InsertCellPoint(6)
lines.InsertCellPoint(7)
profile = vtkPolyData()
profile.SetPoints(points)
profile.SetLines(lines)

#create the tunnel
extrude = vtkRotationalExtrusionFilter()
extrude.SetInput(profile)
extrude.SetAngle(360)
extrude.SetResolution(80)
clean = vtkCleanPolyData()
clean.SetInput(extrude.GetOutput())
clean.SetTolerance(0.001)
normals = vtkPolyDataNormals()
normals.SetInput(clean.GetOutput())
normals.SetFeatureAngle(90)
map = vtkPolyDataMapper()
map.SetInput(normals.GetOutput())
sweep = vtkActor()
sweep.SetMapper(map)
sweep.GetProperty().SetColor(0.3800,0.7000,0.1600)

#create the seam
tuber = vtkTubeFilter()
tuber.SetInput(profile)
tuber.SetNumberOfSides(6)
tuber.SetRadius(0.1)
tubeMapper = vtkPolyDataMapper()
tubeMapper.SetInput(tuber.GetOutput())
seam = vtkActor()
seam.SetMapper(tubeMapper)
seam.GetProperty().SetColor(1.0000,0.3882,0.2784)

# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(sweep)
ren.AddActor(seam)
ren.SetBackground(1,1,1)
ren.TwoSidedLightingOn()

acam = vtkCamera()
acam.SetClippingRange(1.38669,69.3345)
acam.SetFocalPoint(-0.0368406,0.191581,3.37003)
acam.SetPosition(13.6548,2.10315,2.28369)
acam.SetViewAngle(30)
acam.SetViewUp(0.157669,-0.801427,0.576936)

ren.SetActiveCamera(acam)

renWin.SetSize(400,400)
renWin.Render()

renWin.SetFileName("sweptCurve.ppm")

iren.Start()
