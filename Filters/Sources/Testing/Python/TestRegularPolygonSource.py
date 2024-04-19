#!/usr/bin/env python
from vtkmodules.vtkFiltersSources import vtkRegularPolygonSource
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
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create two polygon sources, one a closed polyline, one a polygon
#
polyline = vtkRegularPolygonSource()
polyline.SetCenter(1,1,1)
polyline.SetRadius(1)
polyline.SetNumberOfSides(12)
polyline.SetNormal(1,2,3)
polyline.GeneratePolylineOn()
polyline.GeneratePolygonOff()
polylineMapper = vtkPolyDataMapper()
polylineMapper.SetInputConnection(polyline.GetOutputPort())
polylineActor = vtkActor()
polylineActor.SetMapper(polylineMapper)
polylineActor.GetProperty().SetColor(0,1,0)
polylineActor.GetProperty().SetAmbient(1)
polygon = vtkRegularPolygonSource()
polygon.SetCenter(3,1,1)
polygon.SetRadius(1)
polygon.SetNumberOfSides(12)
polygon.SetNormal(1,2,3)
polygon.GeneratePolylineOff()
polygon.GeneratePolygonOn()
polygonMapper = vtkPolyDataMapper()
polygonMapper.SetInputConnection(polygon.GetOutputPort())
polygonActor = vtkActor()
polygonActor.SetMapper(polygonMapper)
polygonActor.GetProperty().SetColor(1,0,0)
polygonActor.GetProperty().SetAmbient(1)
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create room profile# Add the actors to the renderer, set the background and size
#
ren1.AddActor(polylineActor)
ren1.AddActor(polygonActor)
ren1.SetBackground(0,0,0)
renWin.SetSize(200,200)
renWin.Render()
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
