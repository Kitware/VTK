#!/usr/bin/env python

# Create a constrained Delaunay triangulation following fault lines. The
# fault lines serve as constraint edges in the Delaunay triangulation.

import vtk
from vtk.util.misc import vtkGetDataRoot
from vtk.util.colors import *
VTK_DATA_ROOT = vtkGetDataRoot()

# Generate some points by reading a VTK data file. The data file also
# has edges that represent constraint lines. This is originally from a
# geologic horizon.
reader = vtk.vtkPolyDataReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/faults.vtk")

# Perform a 2D triangulation with constraint edges.
delny = vtk.vtkDelaunay2D()
delny.SetInput(reader.GetOutput())
delny.SetSource(reader.GetOutput())
delny.SetTolerance(0.00001)
normals = vtk.vtkPolyDataNormals()
normals.SetInput(delny.GetOutput())
mapMesh = vtk.vtkPolyDataMapper()
mapMesh.SetInput(normals.GetOutput())
meshActor = vtk.vtkActor()
meshActor.SetMapper(mapMesh)
meshActor.GetProperty().SetColor(beige)

# Now pretty up the mesh with tubed edges and balls at the vertices.
tuber = vtk.vtkTubeFilter()
tuber.SetInput(reader.GetOutput())
tuber.SetRadius(25)
mapLines = vtk.vtkPolyDataMapper()
mapLines.SetInput(tuber.GetOutput())
linesActor = vtk.vtkActor()
linesActor.SetMapper(mapLines)
linesActor.GetProperty().SetColor(1, 0, 0)
linesActor.GetProperty().SetColor(tomato)

# Create graphics objects
# Create the rendering window, renderer, and interactive renderer
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(linesActor)
ren.AddActor(meshActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(350, 250)

cam1 = vtk.vtkCamera()
cam1.SetClippingRange(2580, 129041)
cam1.SetFocalPoint(461550, 6.58e+006, 2132)
cam1.SetPosition(463960, 6.559e+06, 16982)
cam1.SetViewUp(-0.321899, 0.522244, 0.78971)
light = vtk.vtkLight()
light.SetPosition(0, 0, 1)
light.SetFocalPoint(0, 0, 0)
ren.SetActiveCamera(cam1)
ren.AddLight(light)

ren.GetActiveCamera().Zoom(1.5)
iren.LightFollowCameraOff()

iren.Initialize()
renWin.Render()
iren.Start()
