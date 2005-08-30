#!/usr/bin/env python

# This example shows how to use Delaunay3D with alpha shapes.

import vtk

# The points to be triangulated are generated randomly in the unit
# cube located at the origin. The points are then associated with a
# vtkPolyData.
math = vtk.vtkMath()
points = vtk.vtkPoints()
for i in range(0, 25):
    points.InsertPoint(i, math.Random(0, 1), math.Random(0, 1),
                       math.Random(0, 1))

profile = vtk.vtkPolyData()
profile.SetPoints(points)

# Delaunay3D is used to triangulate the points. The Tolerance is the
# distance that nearly coincident points are merged
# together. (Delaunay does better if points are well spaced.) The
# alpha value is the radius of circumcircles, circumspheres. Any mesh
# entity whose circumcircle is smaller than this value is output.
delny = vtk.vtkDelaunay3D()
delny.SetInput(profile)
delny.SetTolerance(0.01)
delny.SetAlpha(0.2)
delny.BoundingTriangulationOff()

# Shrink the result to help see it better.
shrink = vtk.vtkShrinkFilter()
shrink.SetInputConnection(delny.GetOutputPort())
shrink.SetShrinkFactor(0.9)

map = vtk.vtkDataSetMapper()
map.SetInputConnection(shrink.GetOutputPort())

triangulation = vtk.vtkActor()
triangulation.SetMapper(map)
triangulation.GetProperty().SetColor(1, 0, 0)

# Create graphics stuff
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(triangulation)
ren.SetBackground(1, 1, 1)
renWin.SetSize(250, 250)
renWin.Render()

cam1 = ren.GetActiveCamera()
cam1.Zoom(1.5)

iren.Initialize()
renWin.Render()
iren.Start()
