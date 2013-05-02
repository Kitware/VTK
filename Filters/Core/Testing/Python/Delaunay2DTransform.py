#!/usr/bin/env python
import math
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create some points on a sphere such that the data is not in the form
# of z = f(x,y)
#
math1 = vtk.vtkMath()
points = vtk.vtkPoints()
vectors = vtk.vtkFloatArray()
vectors.SetNumberOfComponents(3)
i = 0
while i < 100:
    theta = math1.Random(0.31415, 2.8)
    phi = math1.Random(0.31415, 2.8)
    points.InsertPoint(i, math.cos(theta) * math.sin(phi),
      math.sin(theta) * math.sin(phi), math.cos(phi))
    vectors.InsertTuple3(i, math.cos(theta) * math.sin(phi),
      math.sin(theta) * math.sin(phi), math.cos(phi))
    i = i + 1

profile = vtk.vtkPolyData()
profile.SetPoints(points)
profile.GetPointData().SetVectors(vectors)

# build a transform that rotates this data into z = f(x,y)
#
transform = vtk.vtkTransform()
transform.RotateX(90)

# triangulate the data using the specified transform
#
del1 = vtk.vtkDelaunay2D()
del1.SetInputData(profile)
del1.SetTransform(transform)
del1.BoundingTriangulationOff()
del1.SetTolerance(0.001)
del1.SetAlpha(0.0)

shrink = vtk.vtkShrinkPolyData()
shrink.SetInputConnection(del1.GetOutputPort())

map = vtk.vtkPolyDataMapper()
map.SetInputConnection(shrink.GetOutputPort())

triangulation = vtk.vtkActor()
triangulation.SetMapper(map)
triangulation.GetProperty().SetColor(1, 0, 0)
triangulation.GetProperty().BackfaceCullingOn()

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(triangulation)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(300, 300)
renWin.Render()

cam1 = ren1.GetActiveCamera()
cam1.Zoom(1.5)
cam1.Azimuth(90)
cam1.Elevation(30)
cam1.Azimuth(-60)

# render the image
#
renWin.Render()

#iren.Start()
