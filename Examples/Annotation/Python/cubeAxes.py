#!/usr/bin/env python

# This example demonstrates the use of vtkCubeAxesActor2D to indicate
# the position in space that the camera is currently viewing.  The
# vtkCubeAxesActor2D draws axes on the bounding box of the data set
# and labels the axes with x-y-z coordinates.

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create a vtkBYUReader and read in a data set.
fohe = vtk.vtkBYUReader()
fohe.SetGeometryFileName(VTK_DATA_ROOT + "/Data/teapot.g")

# Create a vtkPolyDataNormals filter to calculate the normals of the
# data set.
normals = vtk.vtkPolyDataNormals()
normals.SetInput(fohe.GetOutput())
# Set up the associated mapper and actor.
foheMapper = vtk.vtkPolyDataMapper()
foheMapper.SetInput(normals.GetOutput())
foheActor = vtk.vtkLODActor()
foheActor.SetMapper(foheMapper)

# Create a vtkOutlineFilter to draw the bounding box of the data set.
# Also create the associated mapper and actor.
outline = vtk.vtkOutlineFilter()
outline.SetInput(normals.GetOutput())
mapOutline = vtk.vtkPolyDataMapper()
mapOutline.SetInput(outline.GetOutput())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(mapOutline)
outlineActor.GetProperty().SetColor(0, 0, 0)

# Create a vtkCamera, and set the camera parameters.
camera = vtk.vtkCamera()
camera.SetClippingRange(1.60187, 20.0842)
camera.SetFocalPoint(0.21406, 1.5, 0)
camera.SetPosition(8.3761, 4.94858, 4.12505)
camera.SetViewUp(0.180325, 0.549245, -0.815974)

# Create a vtkLight, and set the light parameters.
light = vtk.vtkLight()
light.SetFocalPoint(0.21406, 1.5, 0)
light.SetPosition(8.3761, 4.94858, 4.12505)

# Create the Renderers.  Assign them the appropriate viewport
# coordinates, active camera, and light.
ren = vtk.vtkRenderer()
ren.SetViewport(0, 0, 0.5, 1.0)
ren.SetActiveCamera(camera)
ren.AddLight(light)
ren2 = vtk.vtkRenderer()
ren2.SetViewport(0.5, 0, 1.0, 1.0)
ren2.SetActiveCamera(camera)
ren2.AddLight(light)

# Create the RenderWindow and RenderWindowInteractor.
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.AddRenderer(ren2)
renWin.SetWindowName("VTK - Cube Axes")
renWin.SetSize(600, 300)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, and set the background.
ren.AddProp(foheActor)
ren.AddProp(outlineActor)
ren2.AddProp(foheActor)
ren2.AddProp(outlineActor)

ren.SetBackground(0.1, 0.2, 0.4)
ren2.SetBackground(0.1, 0.2, 0.4)

# Create a text property for both cube axes
tprop = vtk.vtkTextProperty()
tprop.SetColor(1, 1, 1)
tprop.ShadowOn()

# Create a vtkCubeAxesActor2D.  Use the outer edges of the bounding box to
# draw the axes.  Add the actor to the renderer.
axes = vtk.vtkCubeAxesActor2D()
axes.SetInput(normals.GetOutput())
axes.SetCamera(ren.GetActiveCamera())
axes.SetLabelFormat("%6.4g")
axes.SetFlyModeToOuterEdges()
axes.SetFontFactor(0.8)
axes.SetAxisTitleTextProperty(tprop)
axes.SetAxisLabelTextProperty(tprop)
ren.AddProp(axes)

# Create a vtkCubeAxesActor2D.  Use the closest vertex to the camera to
# determine where to draw the axes.  Add the actor to the renderer.
axes2 = vtk.vtkCubeAxesActor2D()
axes2.SetProp(foheActor)
axes2.SetCamera(ren2.GetActiveCamera())
axes2.SetLabelFormat("%6.4g")
axes2.SetFlyModeToClosestTriad()
axes2.SetFontFactor(0.8)
axes2.ScalingOff()
axes2.SetAxisTitleTextProperty(tprop)
axes2.SetAxisLabelTextProperty(tprop)
ren2.AddProp(axes2)

# Set up a check for aborting rendering.
def CheckAbort(obj, event):
    # obj will be the object generating the event.  In this case it
    # is renWin.    
    if obj.GetEventPending() != 0:
        obj.SetAbortRender(1)
 
renWin.AddObserver("AbortCheckEvent", CheckAbort)

iren.Initialize()
renWin.Render()
iren.Start()
