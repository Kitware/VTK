#!/usr/bin/env python

# This example creates a polygonal model of a cone, and then renders
# it to the screen. It also defines an interaction style by creating a
# set of Command/Observers. (Note: it is far more efficient to create
# new styles by subclassing vtkInteractorStyle. This is just an
# illustrative example.) If you really want trackball behavior, look
# at the vtkInteractorStyleTrackballCamera class.

import vtk
import sys

# We create an instance of vtkConeSource and set some of its
# properties. The instance of vtkConeSource "cone" is part of a
# visualization pipeline (it is a source process object); it produces
# data (output type is vtkPolyData) which other filters may process.
cone = vtk.vtkConeSource()
cone.SetHeight(3.0)
cone.SetRadius(1.0)
cone.SetResolution(10)

# In this example we terminate the pipeline with a mapper process
# object.  (Intermediate filters such as vtkShrinkPolyData could be
# inserted in between the source and the mapper.)  We create an
# instance of vtkPolyDataMapper to map the polygonal data into
# graphics primitives. We connect the output of the cone souece to the
# input of this mapper.
coneMapper = vtk.vtkPolyDataMapper()
coneMapper.SetInput(cone.GetOutput())

# Create an actor to represent the cone. The actor orchestrates
# rendering of the mapper's graphics primitives. An actor also refers
# to properties via a vtkProperty instance, and includes an internal
# transformation matrix. We set this actor's mapper to be coneMapper
# which we created above.
coneActor = vtk.vtkActor()
coneActor.SetMapper(coneMapper)

# Create the Renderer and assign actors to it. A renderer is like a
# viewport. It is part or all of a window on the screen and it is
# responsible for drawing the actors it has.  We also set the
# background color here.
ren = vtk.vtkRenderer()
ren.AddActor(coneActor)
ren.SetBackground(0.1, 0.2, 0.4)

# Finally we create the render window which will show up on the screen
# We put our renderer into the render window using AddRenderer. We
# also set the size to be 300 pixels by 300.
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(300, 300)

# Define custom interaction.
iren = vtk.vtkRenderWindowInteractor()
iren.SetInteractorStyle(None)
iren.SetRenderWindow(renWin)

# Add the observers to watch for particular events. These invoke
# Python functions.
Rotating = 0
Panning = 0
Zooming = 0

# Handle the mouse button events.
def ButtonEvent(obj, event):
    global Rotating, Panning, Zooming
    if event == "LeftButtonPressEvent":
        Rotating = 1
    elif event == "LeftButtonReleaseEvent":
        Rotating = 0
    elif event == "MiddleButtonPressEvent":
        Panning = 1
    elif event == "MiddleButtonReleaseEvent":
        Panning = 0
    elif event == "RightButtonPressEvent":
        Zooming = 1
    elif event == "RightButtonReleaseEvent":
        Zooming = 0

# General high-level logic
def MouseMove(obj, event):
    global Rotating, Panning, Zooming
    global iren, renWin, ren
    lastXYpos = iren.GetLastEventPosition()
    lastX = lastXYpos[0]
    lastY = lastXYpos[1]

    xypos = iren.GetEventPosition()
    x = xypos[0]
    y = xypos[1]

    center = renWin.GetSize()
    centerX = center[0]/2.0
    centerY = center[1]/2.0

    if Rotating:
        Rotate(ren, ren.GetActiveCamera(), x, y, lastX, lastY,
               centerX, centerY)
    elif Panning:
        Pan(ren, ren.GetActiveCamera(), x, y, lastX, lastY, centerX,
            centerY)
    elif Zooming:
        Dolly(ren, ren.GetActiveCamera(), x, y, lastX, lastY,
              centerX, centerY)
  

def Keypress(obj, event):
    key = obj.GetKeySym()
    if key == "e":
        obj.InvokeEvent("DeleteAllObjects")
        sys.exit()
    elif key == "w":
        Wireframe()
    elif key =="s":
        Surface() 
 

# Routines that translate the events into camera motions.

# This one is associated with the left mouse button. It translates x
# and y relative motions into camera azimuth and elevation commands.
def Rotate(renderer, camera, x, y, lastX, lastY, centerX, centerY):    
    camera.Azimuth(lastX-x)
    camera.Elevation(lastY-y)
    camera.OrthogonalizeViewUp()
    renWin.Render()


# Pan translates x-y motion into translation of the focal point and
# position.
def Pan(renderer, camera, x, y, lastX, lastY, centerX, centerY):
    FPoint = camera.GetFocalPoint()
    FPoint0 = FPoint[0]
    FPoint1 = FPoint[1]
    FPoint2 = FPoint[2]

    PPoint = camera.GetPosition()
    PPoint0 = PPoint[0]
    PPoint1 = PPoint[1]
    PPoint2 = PPoint[2]

    renderer.SetWorldPoint(FPoint0, FPoint1, FPoint2, 1.0)
    renderer.WorldToDisplay()
    DPoint = renderer.GetDisplayPoint()
    focalDepth = DPoint[2]

    APoint0 = centerX+(x-lastX)
    APoint1 = centerY+(y-lastY)
    
    renderer.SetDisplayPoint(APoint0, APoint1, focalDepth)
    renderer.DisplayToWorld()
    RPoint = renderer.GetWorldPoint()
    RPoint0 = RPoint[0]
    RPoint1 = RPoint[1]
    RPoint2 = RPoint[2]
    RPoint3 = RPoint[3]
    
    if RPoint3 != 0.0:
        RPoint0 = RPoint0/RPoint3
        RPoint1 = RPoint1/RPoint3
        RPoint2 = RPoint2/RPoint3

    camera.SetFocalPoint( (FPoint0-RPoint0)/2.0 + FPoint0,
                          (FPoint1-RPoint1)/2.0 + FPoint1,
                          (FPoint2-RPoint2)/2.0 + FPoint2)
    camera.SetPosition( (FPoint0-RPoint0)/2.0 + PPoint0,
                        (FPoint1-RPoint1)/2.0 + PPoint1,
                        (FPoint2-RPoint2)/2.0 + PPoint2)
    renWin.Render()
 

# Dolly converts y-motion into a camera dolly commands.
def Dolly(renderer, camera, x, y, lastX, lastY, centerX, centerY):
    dollyFactor = pow(1.02,(0.5*(y-lastY)))
    if camera.GetParallelProjection():
        parallelScale = camera.GetParallelScale()*dollyFactor
        camera.SetParallelScale(parallelScale)
    else:
        camera.Dolly(dollyFactor)
        renderer.ResetCameraClippingRange()

    renWin.Render() 

# Wireframe sets the representation of all actors to wireframe.
def Wireframe():
    actors = ren.GetActors()
    actors.InitTraversal()
    actor = actors.GetNextItem()
    while actor:
        actor.GetProperty().SetRepresentationToWireframe()
        actor = actors.GetNextItem()

    renWin.Render() 

# Surface sets the representation of all actors to surface.
def Surface():
    actors = ren.GetActors()
    actors.InitTraversal()
    actor = actors.GetNextItem()
    while actor:
        actor.GetProperty().SetRepresentationToSurface()
        actor = actors.GetNextItem()
    renWin.Render()


iren.AddObserver("LeftButtonPressEvent", ButtonEvent)
iren.AddObserver("LeftButtonReleaseEvent", ButtonEvent)
iren.AddObserver("MiddleButtonPressEvent", ButtonEvent)
iren.AddObserver("MiddleButtonReleaseEvent", ButtonEvent)
iren.AddObserver("RightButtonPressEvent", ButtonEvent)
iren.AddObserver("RightButtonReleaseEvent", ButtonEvent)
iren.AddObserver("MouseMoveEvent", MouseMove)
iren.AddObserver("KeyPressEvent", Keypress)


iren.Initialize()
renWin.Render()
iren.Start()
