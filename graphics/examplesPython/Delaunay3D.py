#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# create some random points in the unit cube centered at (.5,.5,.5)
#
math = vtkMath()
points = vtkPoints()
for i in range(0,25):
	points.InsertPoint(i,math.Random(0,1),math.Random(0,1),math.Random(0,1))
 

profile = vtkPolyData()
profile.SetPoints(points)

# triangulate them
#
delny = vtkDelaunay3D()
delny.SetInput(profile)
delny.BoundingTriangulationOn()
delny.SetTolerance(0.01)
delny.SetAlpha(0.2)
delny.BoundingTriangulationOff()
    
shrink = vtkShrinkFilter()
shrink.SetInput(delny.GetOutput())
shrink.SetShrinkFactor(0.9)

map = vtkDataSetMapper()
map.SetInput(shrink.GetOutput())

triangulation = vtkActor()
triangulation.SetMapper(map)
triangulation.GetProperty().SetColor(1,0,0)

# Create graphics stuff
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(triangulation)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)
renWin.Render()

cam1=ren.GetActiveCamera()
cam1.Zoom(1.5)

renWin.Render()
#renWin.SetFileName("Delaunay3D.ppm")
#renWin.SaveImageAsPPM()

iren.Start()
