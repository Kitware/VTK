#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# Create a fancy image of a 2D Delaunay triangulation. Points are randomly 
# generated.

from colors import *
# create some points
#
math = vtkMath()
points = vtkPoints()
for i in range(0,50):
	points.InsertPoint(i, math.Random(0,1),math.Random(0,1),0.0)
 

profile = vtkPolyData()
profile.SetPoints(points)

# triangulate them
#
delny = vtkDelaunay2D()
delny.SetInput(profile)
delny.SetTolerance(0.001)
mapMesh = vtkPolyDataMapper()
mapMesh.SetInput(delny.GetOutput())
meshActor = vtkActor()
meshActor.SetMapper(mapMesh)
meshActor.GetProperty().SetColor(.1,.2,.4)

extract = vtkExtractEdges()
extract.SetInput(delny.GetOutput())
tubes = vtkTubeFilter()
tubes.SetInput(extract.GetOutput())
tubes.SetRadius(0.01)
tubes.SetNumberOfSides(6)
mapEdges = vtkPolyDataMapper()
mapEdges.SetInput(tubes.GetOutput())
edgeActor = vtkActor()
edgeActor.SetMapper(mapEdges)
edgeActor.GetProperty().SetColor(peacock[0],peacock[1],peacock[2])
edgeActor.GetProperty().SetSpecularColor(1,1,1)
edgeActor.GetProperty().SetSpecular(0.3)
edgeActor.GetProperty().SetSpecularPower(20)
edgeActor.GetProperty().SetAmbient(0.2)
edgeActor.GetProperty().SetDiffuse(0.8)

ball = vtkSphereSource()
ball.SetRadius(0.025)
ball.SetThetaResolution(12)
ball.SetPhiResolution(12)
balls = vtkGlyph3D()
balls.SetInput(delny.GetOutput())
balls.SetSource(ball.GetOutput())
mapBalls = vtkPolyDataMapper()
mapBalls.SetInput(balls.GetOutput())
ballActor = vtkActor()
ballActor.SetMapper(mapBalls)
ballActor.GetProperty().SetColor(hot_pink[0],hot_pink[1],hot_pink[2])
ballActor.GetProperty().SetSpecularColor(1,1,1)
ballActor.GetProperty().SetSpecular(0.3)
ballActor.GetProperty().SetSpecularPower(20)
ballActor.GetProperty().SetAmbient(0.2)
ballActor.GetProperty().SetDiffuse(0.8)

# Create graphics objects
# Create the rendering window, renderer, and interactive renderer
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(ballActor)
ren.AddActor(edgeActor)
ren.SetBackground(1,1,1)
renWin.SetSize(150,150)

ren.GetActiveCamera().Zoom(1.5)
iren.Initialize()

renWin.SetFileName("DelMesh.ppm")

iren.Start()
