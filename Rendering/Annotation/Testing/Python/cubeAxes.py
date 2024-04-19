#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import vtkPolyDataNormals
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkIOGeometry import vtkBYUReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
    vtkLight,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkTextProperty,
)
from vtkmodules.vtkRenderingAnnotation import vtkCubeAxesActor2D
from vtkmodules.vtkRenderingLOD import vtkLODActor
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# read in an interesting object and outline it
#
fohe = vtkBYUReader()
fohe.SetGeometryFileName(VTK_DATA_ROOT + "/Data/teapot.g")

normals = vtkPolyDataNormals()
normals.SetInputConnection(fohe.GetOutputPort())

foheMapper = vtkPolyDataMapper()
foheMapper.SetInputConnection(normals.GetOutputPort())

foheActor = vtkLODActor()
foheActor.SetMapper(foheMapper)

outline = vtkOutlineFilter()
outline.SetInputConnection(normals.GetOutputPort())

mapOutline = vtkPolyDataMapper()
mapOutline.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(mapOutline)
outlineActor.GetProperty().SetColor(0, 0, 0)

# Create the RenderWindow, Renderer, and setup viewports
camera = vtkCamera()
camera.SetClippingRange(1.60187, 20.0842)
camera.SetFocalPoint(0.21406, 1.5, 0)
camera.SetPosition(11.63, 6.32, 5.77)
camera.SetViewUp(0.180325, 0.549245, -0.815974)

light = vtkLight()
light.SetFocalPoint(0.21406, 1.5, 0)
light.SetPosition(8.3761, 4.94858, 4.12505)

ren1 = vtkRenderer()
ren1.SetViewport(0, 0, 0.5, 1.0)
ren1.SetActiveCamera(camera)
ren1.AddLight(light)

ren2 = vtkRenderer()
ren2.SetViewport(0.5, 0, 1.0, 1.0)
ren2.SetActiveCamera(camera)
ren2.AddLight(light)

renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.SetWindowName("VTK - Cube Axes")

renWin.SetSize(790, 400)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddViewProp(foheActor)
ren1.AddViewProp(outlineActor)

ren2.AddViewProp(foheActor)
ren2.AddViewProp(outlineActor)

ren1.SetBackground(0.1, 0.2, 0.4)
ren2.SetBackground(0.1, 0.2, 0.4)

tprop = vtkTextProperty()
tprop.SetColor(1, 1, 1)
tprop.ShadowOn()

axes = vtkCubeAxesActor2D()
axes.SetInputConnection(normals.GetOutputPort())
axes.SetCamera(ren1.GetActiveCamera())
axes.SetLabelFormat("%6.1f")
axes.SetFlyModeToOuterEdges()
axes.SetFontFactor(0.8)
axes.SetAxisTitleTextProperty(tprop)
axes.SetAxisLabelTextProperty(tprop)

ren1.AddViewProp(axes)

axes2 = vtkCubeAxesActor2D()
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

def TkCheckAbort(obj=None, event=""):
    if renWin.GetEventPending():
        renWin.SetAbortRender(1)

renWin.AddObserver("AbortCheckEvent", TkCheckAbort)

#iren.Start()
