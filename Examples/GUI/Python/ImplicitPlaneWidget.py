#!/usr/bin/env python

# This example demonstrates how to use the vtkPlaneWidget to probe a
# dataset and then generate contours on the probed data.

import vtk

# Create a mace out of filters.
sphere = vtk.vtkSphereSource()
cone = vtk.vtkConeSource()
glyph = vtk.vtkGlyph3D()
glyph.SetInputConnection(sphere.GetOutputPort())
glyph.SetSource(cone.GetOutput())
glyph.SetVectorModeToUseNormal()
glyph.SetScaleModeToScaleByVector()
glyph.SetScaleFactor(0.25)

# The sphere and spikes are appended into a single polydata.
# This just makes things simpler to manage.
apd = vtk.vtkAppendPolyData()
apd.AddInput(glyph.GetOutput())
apd.AddInput(sphere.GetOutput())

maceMapper = vtk.vtkPolyDataMapper()
maceMapper.SetInputConnection(apd.GetOutputPort())

maceActor = vtk.vtkLODActor()
maceActor.SetMapper(maceMapper)
maceActor.VisibilityOn()

# This portion of the code clips the mace with the vtkPlanes
# implicit function. The clipped region is colored green.
plane = vtk.vtkPlane()
clipper = vtk.vtkClipPolyData()
clipper.SetInputConnection(apd.GetOutputPort())
clipper.SetClipFunction(plane)
clipper.InsideOutOn()

selectMapper = vtk.vtkPolyDataMapper()
selectMapper.SetInputConnection(clipper.GetOutputPort())

selectActor = vtk.vtkLODActor()
selectActor.SetMapper(selectMapper)
selectActor.GetProperty().SetColor(0, 1, 0)
selectActor.VisibilityOff()
selectActor.SetScale(1.01, 1.01, 1.01)

# Create the RenderWindow, Renderer and both Actors
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# The callback function
def myCallback(obj, event):
    global plane, selectActor
    obj.GetPlane(plane)
    selectActor.VisibilityOn()
 
# Associate the line widget with the interactor
planeWidget = vtk.vtkImplicitPlaneWidget()
planeWidget.SetInteractor(iren)
planeWidget.SetPlaceFactor(1.25)
planeWidget.SetInput(glyph.GetOutput())
planeWidget.PlaceWidget()
planeWidget.AddObserver("InteractionEvent", myCallback)

ren.AddActor(maceActor)
ren.AddActor(selectActor)

# Add the actors to the renderer, set the background and size
ren.SetBackground(1, 1, 1)
renWin.SetSize(300, 300)
ren.SetBackground(0.1, 0.2, 0.4)

# Start interaction.
iren.Initialize()
renWin.Render()
iren.Start()
