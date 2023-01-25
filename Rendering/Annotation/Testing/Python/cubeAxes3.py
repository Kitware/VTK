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

# This example illustrates how one may explicitly specify the range of each
# axes that's used to define the prop, while displaying data with a different
# set of bounds (unlike cubeAxes2.tcl). This example allows you to separate
# the notion of extent of the axes in physical space (bounds) and the extent
# of the values it represents. In other words, you can have the ticks and
# labels show a different range.
#
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
foheActor.GetProperty().SetDiffuseColor(0.7, 0.3, 0.0)

outline = vtkOutlineFilter()
outline.SetInputConnection(normals.GetOutputPort())

mapOutline = vtkPolyDataMapper()
mapOutline.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(mapOutline)
outlineActor.GetProperty().SetColor(0, 0, 0)

# Create the RenderWindow, Renderer, and setup viewports
camera = vtkCamera()
camera.SetClippingRange(1.0, 100.0)
camera.SetFocalPoint(0.9, 1.0, 0.0)
camera.SetPosition(11.63, 6.0, 10.77)

light = vtkLight()
light.SetFocalPoint(0.21406, 1.5, 0)
light.SetPosition(8.3761, 4.94858, 4.12505)

ren2 = vtkRenderer()
ren2.SetActiveCamera(camera)
ren2.AddLight(light)

renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren2)
renWin.SetWindowName("VTK - Cube Axes custom range")

renWin.SetSize(600, 600)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren2.AddViewProp(foheActor)
ren2.AddViewProp(outlineActor)
ren2.SetBackground(0.1, 0.2, 0.4)

normals.Update()

bounds = normals.GetOutput().GetBounds()
axes2 = vtkCubeAxesActor()
axes2.SetBounds(
  bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5])
axes2.SetXAxisRange(20, 300)
axes2.SetYAxisRange(-0.01, 0.01)
axes2.SetCamera(ren2.GetActiveCamera())
axes2.SetXLabelFormat("%6.1f")
axes2.SetYLabelFormat("%6.1f")
axes2.SetZLabelFormat("%6.1f")
axes2.SetFlyModeToClosestTriad()
axes2.SetScreenSize(20.0)

ren2.AddViewProp(axes2)

renWin.Render()
ren2.ResetCamera()
renWin.Render()

# render the image
#
iren.Initialize()

def TkCheckAbort(obj=None, event=""):
    if renWin.GetEventPending():
        renWin.SetAbortRender(1)

renWin.AddObserver("AbortCheckEvent", TkCheckAbort)

#iren.Start()
