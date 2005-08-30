#!/usr/bin/env python

# This example demonstrates how to use implicit modelling.

import vtk
from vtk.util.misc import vtkGetDataRoot
from vtk.util.colors import red, peacock
VTK_DATA_ROOT = vtkGetDataRoot()

# Create lines which serve as the "seed" geometry. The lines spell the
# word "hello".
reader = vtk.vtkPolyDataReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/hello.vtk")
lineMapper = vtk.vtkPolyDataMapper()
lineMapper.SetInputConnection(reader.GetOutputPort())
lineActor = vtk.vtkActor()
lineActor.SetMapper(lineMapper)
lineActor.GetProperty().SetColor(red)

# Create implicit model with vtkImplicitModeller. This computes a
# scalar field which is the distance from the generating geometry. The
# contour filter then extracts the geoemtry at the distance value 0.25
# from the generating geometry.
imp = vtk.vtkImplicitModeller()
imp.SetInputConnection(reader.GetOutputPort())
imp.SetSampleDimensions(110, 40, 20)
imp.SetMaximumDistance(0.25)
imp.SetModelBounds(-1.0, 10.0, -1.0, 3.0, -1.0, 1.0)
contour = vtk.vtkContourFilter()
contour.SetInputConnection(imp.GetOutputPort())
contour.SetValue(0, 0.25)
impMapper = vtk.vtkPolyDataMapper()
impMapper.SetInputConnection(contour.GetOutputPort())
impMapper.ScalarVisibilityOff()
impActor = vtk.vtkActor()
impActor.SetMapper(impMapper)
impActor.GetProperty().SetColor(peacock)
impActor.GetProperty().SetOpacity(0.5)

# Create the usual graphics stuff.
# Create the RenderWindow, Renderer and both Actors
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(lineActor)
ren.AddActor(impActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(600, 250)

camera = vtk.vtkCamera()
camera.SetClippingRange(1.81325, 90.6627)
camera.SetFocalPoint(4.5, 1, 0)
camera.SetPosition(4.5, 1.0, 6.73257)
camera.SetViewUp(0, 1, 0)
camera.Zoom(0.8)
ren.SetActiveCamera(camera)

iren.Initialize()
renWin.Render()
iren.Start()
