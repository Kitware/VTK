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
)
from vtkmodules.vtkRenderingAnnotation import vtkCubeAxesActor
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
ren1.SetViewport(0, 0, 0.33, 0.5)
ren1.SetActiveCamera(camera)
ren1.AddLight(light)

ren2 = vtkRenderer()
ren2.SetViewport(0.33, 0, 0.66, 0.5)
ren2.SetActiveCamera(camera)
ren2.AddLight(light)

ren3 = vtkRenderer()
ren3.SetViewport(0.66, 0, 1.0, 0.5)
ren3.SetActiveCamera(camera)
ren3.AddLight(light)

ren4 = vtkRenderer()
ren4.SetViewport(0, 0.5, 0.5, 1.0)
ren4.SetActiveCamera(camera)
ren4.AddLight(light)

ren5 = vtkRenderer()
ren5.SetViewport(0.5, 0.5, 1.0, 1.0)
ren5.SetActiveCamera(camera)
ren5.AddLight(light)

renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
renWin.AddRenderer(ren4)
renWin.AddRenderer(ren5)
renWin.SetWindowName("VTK - Cube Axes")

renWin.SetSize(600, 600)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddViewProp(foheActor)
ren1.AddViewProp(outlineActor)

ren2.AddViewProp(foheActor)
ren2.AddViewProp(outlineActor)

ren3.AddViewProp(foheActor)
ren3.AddViewProp(outlineActor)

ren4.AddViewProp(foheActor)
ren4.AddViewProp(outlineActor)

ren5.AddViewProp(foheActor)

ren1.SetBackground(0.1, 0.2, 0.4)
ren2.SetBackground(0.1, 0.2, 0.4)
ren3.SetBackground(0.1, 0.2, 0.4)
ren4.SetBackground(0.1, 0.2, 0.4)
ren5.SetBackground(0.1, 0.2, 0.4)

normals.Update()

bounds = normals.GetOutput().GetBounds()

axes = vtkCubeAxesActor()
axes.SetBounds(
  bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5])
axes.SetCamera(ren1.GetActiveCamera())
axes.SetXLabelFormat("%6.1f")
axes.SetYLabelFormat("%6.1f")
axes.SetZLabelFormat("%6.1f")
axes.SetFlyModeToOuterEdges()

ren1.AddViewProp(axes)

axes2 = vtkCubeAxesActor()
axes2.SetBounds(
  bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5])
axes2.SetCamera(ren2.GetActiveCamera())
axes2.SetXLabelFormat(axes.GetXLabelFormat())
axes2.SetYLabelFormat(axes.GetYLabelFormat())
axes2.SetZLabelFormat(axes.GetZLabelFormat())
axes2.SetFlyModeToClosestTriad()

ren2.AddViewProp(axes2)

axes3 = vtkCubeAxesActor()
axes3.SetBounds(
  bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5])
axes3.SetCamera(ren2.GetActiveCamera())
axes3.SetXLabelFormat(axes.GetXLabelFormat())
axes3.SetYLabelFormat(axes.GetYLabelFormat())
axes3.SetZLabelFormat(axes.GetZLabelFormat())
axes3.SetFlyModeToFurthestTriad()

ren3.AddViewProp(axes3)

bounds2 = axes3.GetBounds()

axes4 = vtkCubeAxesActor()
axes4.SetBounds(
  bounds2[0], bounds2[1], bounds2[2], bounds2[3], bounds2[4], bounds2[5])
axes4.SetCamera(ren2.GetActiveCamera())
axes4.SetXLabelFormat(axes.GetXLabelFormat())
axes4.SetYLabelFormat(axes.GetYLabelFormat())
axes4.SetZLabelFormat(axes.GetZLabelFormat())
axes4.SetFlyModeToStaticTriad()

ren4.AddViewProp(axes4)

axes5 = vtkCubeAxesActor()
axes5.SetBounds(
  bounds2[0], bounds2[1], bounds2[2], bounds2[3], bounds2[4], bounds2[5])
axes5.SetCamera(ren2.GetActiveCamera())
axes5.SetXLabelFormat(axes.GetXLabelFormat())
axes5.SetYLabelFormat(axes.GetYLabelFormat())
axes5.SetZLabelFormat(axes.GetZLabelFormat())
axes5.SetFlyModeToStaticEdges()

ren5.AddViewProp(axes5)

renWin.Render()
# render the image
#
iren.Initialize()

def TkCheckAbort(obj=None, event=""):
    if renWin.GetEventPending():
        renWin.SetAbortRender(1)

renWin.AddObserver("AbortCheckEvent", TkCheckAbort)


threshold = 13

#iren.Start()
