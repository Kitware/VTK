#!/usr/bin/env python

# This example demonstrates how to use 2D Delaunay triangulation.
# We create a fancy image of a 2D Delaunay triangulation. Points are
# randomly generated.

import vtk
from vtk.util.colors import *

# Generate some random points
math = vtk.vtkMath()
points = vtk.vtkPoints()
for i in range(0, 50):
    points.InsertPoint(i, math.Random(0, 1), math.Random(0, 1), 0.0)

# Create a polydata with the points we just created.
profile = vtk.vtkPolyData()
profile.SetPoints(points)

# Perform a 2D Delaunay triangulation on them.
delny = vtk.vtkDelaunay2D()
delny.SetInput(profile)
delny.SetTolerance(0.001)
mapMesh = vtk.vtkPolyDataMapper()
mapMesh.SetInput(delny.GetOutput())
meshActor = vtk.vtkActor()
meshActor.SetMapper(mapMesh)
meshActor.GetProperty().SetColor(.1, .2, .4)

# We will now create a nice looking mesh by wrapping the edges in tubes,
# and putting fat spheres at the points.
extract = vtk.vtkExtractEdges()
extract.SetInput(delny.GetOutput())
tubes = vtk.vtkTubeFilter()
tubes.SetInput(extract.GetOutput())
tubes.SetRadius(0.01)
tubes.SetNumberOfSides(6)
mapEdges = vtk.vtkPolyDataMapper()
mapEdges.SetInput(tubes.GetOutput())
edgeActor = vtk.vtkActor()
edgeActor.SetMapper(mapEdges)
edgeActor.GetProperty().SetColor(peacock)
edgeActor.GetProperty().SetSpecularColor(1, 1, 1)
edgeActor.GetProperty().SetSpecular(0.3)
edgeActor.GetProperty().SetSpecularPower(20)
edgeActor.GetProperty().SetAmbient(0.2)
edgeActor.GetProperty().SetDiffuse(0.8)

ball = vtk.vtkSphereSource()
ball.SetRadius(0.025)
ball.SetThetaResolution(12)
ball.SetPhiResolution(12)
balls = vtk.vtkGlyph3D()
balls.SetInput(delny.GetOutput())
balls.SetSource(ball.GetOutput())
mapBalls = vtk.vtkPolyDataMapper()
mapBalls.SetInput(balls.GetOutput())
ballActor = vtk.vtkActor()
ballActor.SetMapper(mapBalls)
ballActor.GetProperty().SetColor(hot_pink)
ballActor.GetProperty().SetSpecularColor(1, 1, 1)
ballActor.GetProperty().SetSpecular(0.3)
ballActor.GetProperty().SetSpecularPower(20)
ballActor.GetProperty().SetAmbient(0.2)
ballActor.GetProperty().SetDiffuse(0.8)

# Create the rendering window, renderer, and interactive renderer
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(ballActor)
ren.AddActor(edgeActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(150, 150)

ren.GetActiveCamera().Zoom(1.5)

# Interact with the data.
iren.Initialize()
renWin.Render()
iren.Start()
