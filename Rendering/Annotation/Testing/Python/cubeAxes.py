#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# read in an interesting object and outline it
#
fohe = vtk.vtkBYUReader()
fohe.SetGeometryFileName(VTK_DATA_ROOT + "/Data/teapot.g")

normals = vtk.vtkPolyDataNormals()
normals.SetInputConnection(fohe.GetOutputPort())

foheMapper = vtk.vtkPolyDataMapper()
foheMapper.SetInputConnection(normals.GetOutputPort())

foheActor = vtk.vtkLODActor()
foheActor.SetMapper(foheMapper)

outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(normals.GetOutputPort())

mapOutline = vtk.vtkPolyDataMapper()
mapOutline.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(mapOutline)
outlineActor.GetProperty().SetColor(0, 0, 0)

# Create the RenderWindow, Renderer, and setup viewports
camera = vtk.vtkCamera()
camera.SetClippingRange(1.60187, 20.0842)
camera.SetFocalPoint(0.21406, 1.5, 0)
camera.SetPosition(11.63, 6.32, 5.77)
camera.SetViewUp(0.180325, 0.549245, -0.815974)

light = vtk.vtkLight()
light.SetFocalPoint(0.21406, 1.5, 0)
light.SetPosition(8.3761, 4.94858, 4.12505)

ren1 = vtk.vtkRenderer()
ren1.SetViewport(0, 0, 0.5, 1.0)
ren1.SetActiveCamera(camera)
ren1.AddLight(light)

ren2 = vtk.vtkRenderer()
ren2.SetViewport(0.5, 0, 1.0, 1.0)
ren2.SetActiveCamera(camera)
ren2.AddLight(light)

renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.SetWindowName("VTK - Cube Axes")

renWin.SetSize(790, 400)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddViewProp(foheActor)
ren1.AddViewProp(outlineActor)

ren2.AddViewProp(foheActor)
ren2.AddViewProp(outlineActor)

ren1.SetBackground(0.1, 0.2, 0.4)
ren2.SetBackground(0.1, 0.2, 0.4)

tprop = vtk.vtkTextProperty()
tprop.SetColor(1, 1, 1)
tprop.ShadowOn()

axes = vtk.vtkCubeAxesActor2D()
axes.SetInputConnection(normals.GetOutputPort())
axes.SetCamera(ren1.GetActiveCamera())
axes.SetLabelFormat("%6.1f")
axes.SetFlyModeToOuterEdges()
axes.SetFontFactor(0.8)
axes.SetAxisTitleTextProperty(tprop)
axes.SetAxisLabelTextProperty(tprop)

ren1.AddViewProp(axes)

axes2 = vtk.vtkCubeAxesActor2D()
axes2.SetViewProp(foheActor)
axes2.SetCamera(ren2.GetActiveCamera())
axes2.SetLabelFormat(axes.GetLabelFormat())
axes2.SetFlyModeToClosestTriad()
axes2.SetFontFactor(axes.GetFontFactor())
axes2.ScalingOff()
axes2.SetAxisTitleTextProperty(tprop)
axes2.SetAxisLabelTextProperty(tprop)

ren2.AddViewProp(axes2)

renWin.Render()

# render the image
#
iren.Initialize()

def TkCheckAbort (object_binding, event_name):
    foo = renWin.GetEventPending()
    if (foo != 0):
        renWin.SetAbortRender(1)
        pass

renWin.AddObserver("AbortCheckEvent", TkCheckAbort)

#iren.Start()
