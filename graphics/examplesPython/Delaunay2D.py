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

# create some points
#
math = vtkMath()
points = vtkPoints()
for i in range(0,1000):
	points.InsertPoint(i,math.Random(0,1),math.Random(0,1),0.0)
 

profile = vtkPolyData()
profile.SetPoints(points)

# triangulate them
#
delny = vtkDelaunay2D()
delny.SetInput(profile)
delny.BoundingTriangulationOn()
delny.SetTolerance(0.001)
delny.SetAlpha(0.0)
delny.Update()
    
shrink = vtkShrinkPolyData()
shrink.SetInput(delny.GetOutput())

map = vtkPolyDataMapper()
map.SetInput(shrink.GetOutput())

triangulation = vtkActor()
triangulation.SetMapper(map)
triangulation.GetProperty().SetColor(1,0,0)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(triangulation)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)
renWin.Render()

cam1=ren.GetActiveCamera()
cam1.Zoom(1.5)

renWin.Render()

#renWin.SetFileName("Delaunay2D.ppm")
#renWin.SaveImageAsPPM()

iren.Start()
