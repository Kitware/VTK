#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkPlane
from vtkmodules.vtkFiltersCore import vtkFlyingEdgesPlaneCutter
from vtkmodules.vtkFiltersModeling import vtkImageDataOutlineFilter
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
from vtkmodules.vtkInteractionWidgets import (
    vtkImplicitPlaneRepresentation,
    vtkImplicitPlaneWidget2,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from math import cos, sin, pi
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Plane cut an oriented volume

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(1000,1000)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Cut the volume
rta = vtkRTAnalyticSource()
rta.SetWholeExtent(-10, 10, -10, 10, -10, 10)
rta.Update()

plane = vtkPlane()
plane.SetOrigin(1,1,1)
plane.SetNormal(2,1,1.5)

cutter = vtkFlyingEdgesPlaneCutter()
cutter.SetInputConnection(rta.GetOutputPort())
cutter.SetPlane(plane)

cutterMapper = vtkPolyDataMapper()
cutterMapper.SetInputConnection(cutter.GetOutputPort())

cutterActor = vtkActor()
cutterActor.SetMapper(cutterMapper)

# Create an outline around the image
outline = vtkImageDataOutlineFilter()
outline.SetInputConnection(rta.GetOutputPort())
outline.GenerateFacesOn()
outline.Update()

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetOpacity(0.25)
outlineActor.GetProperty().SetColor(0,1,0)

# Widget to manipulate the plane
# The cut plane
def MovePlane(widget, event_string):
    rep.GetPlane(plane)
    cutter.Modified()

rep = vtkImplicitPlaneRepresentation()
rep.SetPlaceFactor(1.0);
rep.PlaceWidget(rta.GetOutput().GetBounds())
rep.DrawPlaneOff()
rep.SetPlane(plane)

planeWidget = vtkImplicitPlaneWidget2()
planeWidget.SetInteractor(iren)
planeWidget.SetRepresentation(rep);
planeWidget.AddObserver("InteractionEvent",MovePlane);
planeWidget.On()

ren1.AddActor(outlineActor)
ren1.AddActor(cutterActor)
ren1.ResetCamera()

renWin.Render()
iren.Start()
