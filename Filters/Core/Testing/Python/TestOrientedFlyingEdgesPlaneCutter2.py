#!/usr/bin/env python
import vtk
from math import cos, sin, pi
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Plane cut an oriented volume

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(1000,1000)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Cut the volume
rta = vtk.vtkRTAnalyticSource()
rta.SetWholeExtent(-10, 10, -10, 10, -10, 10)
rta.Update()

plane = vtk.vtkPlane()
plane.SetOrigin(1,1,1)
plane.SetNormal(2,1,1.5)

cutter = vtk.vtkFlyingEdgesPlaneCutter()
cutter.SetInputConnection(rta.GetOutputPort())
cutter.SetPlane(plane)

cutterMapper = vtk.vtkPolyDataMapper()
cutterMapper.SetInputConnection(cutter.GetOutputPort())

cutterActor = vtk.vtkActor()
cutterActor.SetMapper(cutterMapper)

# Create an outline around the image
outline = vtk.vtkImageDataOutlineFilter()
outline.SetInputConnection(rta.GetOutputPort())
outline.GenerateFacesOn()
outline.Update()

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetOpacity(0.25)
outlineActor.GetProperty().SetColor(0,1,0)

# Widget to manipulate the plane
# The cut plane
def MovePlane(widget, event_string):
    rep.GetPlane(plane)
    cutter.Modified()

rep = vtk.vtkImplicitPlaneRepresentation()
rep.SetPlaceFactor(1.0);
rep.PlaceWidget(rta.GetOutput().GetBounds())
rep.DrawPlaneOff()
rep.SetPlane(plane)

planeWidget = vtk.vtkImplicitPlaneWidget2()
planeWidget.SetInteractor(iren)
planeWidget.SetRepresentation(rep);
planeWidget.AddObserver("InteractionEvent",MovePlane);
planeWidget.On()

ren1.AddActor(outlineActor)
ren1.AddActor(cutterActor)
ren1.ResetCamera()

renWin.Render()
iren.Start()
