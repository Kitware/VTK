#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# This is an example of how to define your own interaction methods
# in Python or Tcl

# Create the RenderWindow, Renderer and both Actors
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
style = vtkInteractorStyleUser()
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.SetInteractorStyle(style)

# create a plane source and actor
plane = vtkPlaneSource()
planeMapper = vtkPolyDataMapper()
planeMapper.SetInput(plane.GetOutput())
planeActor = vtkActor()
planeActor.SetMapper(planeMapper)

# Add the actors to the renderer, set the background and size
ren.AddActor(planeActor)
ren.SetBackground(0.1,0.2,0.4)

# make a picker to identify actors
picker = vtkCellPicker()
picker.SetTolerance(0.001)

# push plane along its normal
def PushPlane():
    (x,y) = style.GetLastPos()
    (oldx,oldy) = style.GetOldPos()
    if (x != oldx):
        norm1 = ren.GetActiveCamera().GetViewPlaneNormal()
        norm2 = plane.GetNormal()
        if (norm1[0]*norm2[0] + norm1[1]*norm2[1] + norm1[2]*norm2[2]) < 0:
            plane.Push(0.005*(x-oldx))
        else:
            plane.Push(0.005*(oldx-x))
        iren.Render()

# if user clicked actor, start push interaction
def StartPushPlane():
    (x,y) = iren.GetEventPosition()
    picker.Pick(x,y,0,ren)
    actor = picker.GetActor()
    if (actor == planeActor):
        style.StartUserInteraction()

# end push interaction
def EndPushPlane():
    style.EndUserInteraction()

# set the methods for pushing a plane
style.SetMiddleButtonPressMethod(StartPushPlane)
style.SetMiddleButtonReleaseMethod(EndPushPlane)
style.SetUserInteractionMethod(PushPlane)

# render the image
iren.Initialize()
cam1=ren.GetActiveCamera()
cam1.Elevation(-30)
cam1.Roll(-20)
renWin.Render()

iren.Start()
