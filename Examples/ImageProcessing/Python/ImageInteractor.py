#!/usr/bin/env python

# This example shows how to use the InteractorStyleImage and add your
# own event handling.  The InteractorStyleImage is a special
# interactor designed to be used with vtkImageActor in a rendering
# window context. It forces the camera to stay perpendicular to the
# x-y plane.

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the image
reader = vtk.vtkPNGReader()
reader.SetDataSpacing(0.8, 0.8, 1.5)
reader.SetFileName(VTK_DATA_ROOT + "/Data/fullhead15.png")

shiftScale = vtk.vtkImageShiftScale()
shiftScale.SetInput(reader.GetOutput())
shiftScale.SetShift(0)
shiftScale.SetScale(0.07)
shiftScale.SetOutputScalarTypeToUnsignedChar()

ia = vtk.vtkImageActor()
ia.SetInput(shiftScale.GetOutput())

# Create the RenderWindow, Renderer and both Actors
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()

# Add the actors to the renderer, set the background and size
ren.AddActor(ia)
ren.SetBackground(0.1, 0.2, 0.4)
renWin.SetSize(400, 400)

renWin.Render()

cam1 = ren.GetActiveCamera()

ren.ResetCameraClippingRange()
renWin.Render()

### Supporting data for callbacks
pts = vtk.vtkPoints()
pts.SetNumberOfPoints(4)
lines = vtk.vtkCellArray()
lines.InsertNextCell(5)
lines.InsertCellPoint(0)
lines.InsertCellPoint(1)
lines.InsertCellPoint(2)
lines.InsertCellPoint(3)
lines.InsertCellPoint(0)
pd = vtk.vtkPolyData()
pd.SetPoints(pts)
pd.SetLines(lines)
bboxMapper = vtk.vtkPolyDataMapper2D()
bboxMapper.SetInput(pd)
bboxActor = vtk.vtkActor2D()
bboxActor.SetMapper(bboxMapper)
bboxActor.GetProperty().SetColor(1, 0, 0)
ren.AddProp(bboxActor)

### Functions for callbacks 
X = 0
Y = 0
bboxEnabled = 0

def StartZoom(obj, event):
    global X, Y, bboxEnabled
    xy = iren.GetEventPosition()
    X, Y = xy

    pts.SetPoint(0, X, Y, 0)
    pts.SetPoint(1, X, Y, 0)
    pts.SetPoint(2, X, Y, 0)
    pts.SetPoint(3, X, Y, 0)

    bboxEnabled = 1
    bboxActor.VisibilityOn()
 

def MouseMove(obj, event):
    global X, Y, bboxEnabled

    if bboxEnabled:
        xy = iren.GetEventPosition()
        x, y = xy
        pts.SetPoint(1, x, Y, 0)
        pts.SetPoint(2, x, y, 0)
        pts.SetPoint(3, X, y, 0)
        renWin.Render() 
 

# Do the hard stuff: pan and dolly
def EndZoom(obj, event):
    global bboxEnabled

    p1 = pts.GetPoint(0)

    p2 = pts.GetPoint(2)
    x1, y1, z1 = p1
    x2, y2, z2 = p2

    ren.SetDisplayPoint(x1, y1, 0)
    ren.DisplayToWorld()
    p1 = ren.GetWorldPoint()
    ren.SetDisplayPoint(x2, y2, 0)
    ren.DisplayToWorld()
    p2 = ren.GetWorldPoint()

    p1X, p1Y, p1Z = p1[:3]

    p2X, p2Y, p2Z = p2[:3]

    camera = ren.GetActiveCamera()
    focalPt = camera.GetFocalPoint()
    focalX, focalY, focalZ = focalPt
    
    position = camera.GetPosition()
    positionX, positionY, positionZ = position

    deltaX = focalX-(p1X+p2X)/2.0
    deltaY = focalY-(p1Y+p2Y)/2.0

    # Set camera focal point to the center of the box
    camera.SetFocalPoint((p1X+p2X)/2.0, (p1Y+p2Y)/2.0, focalZ)
    camera.SetPosition(positionX-deltaX, positionY-deltaY,positionZ)

    # Now dolly the camera to fill the box
    # This is a half-assed hack for demonstration purposes
    if p1X > p2X:
        deltaX = p1X-p2X
    else:
        deltaX = p2X-p1X
    if p1Y > p2Y:
        deltaY = p1Y-p2Y
    else:
       deltaY = p2Y-p1Y
 
    winSize = renWin.GetSize()
    winX, winY = winSize

    sx = deltaX/winX
    sy = deltaY/winY


    if sx > sy:
        dolly = 1.0+1.0/(2.0*sx)
    else:
        dolly = 1.0+1.0/(2.0*sy)
 
    camera.Dolly(dolly)
    ren.ResetCameraClippingRange()

    bboxEnabled = 0
    bboxActor.VisibilityOff()
    renWin.Render()

# Create an image interactor style and associate it with the
# interactive renderer. Then assign some callbacks with the
# appropriate events. THe callbacks are implemented as Python functions.
interactor = vtk.vtkInteractorStyleImage()
iren.SetInteractorStyle(interactor)
interactor.AddObserver("LeftButtonPressEvent", StartZoom)
interactor.AddObserver("MouseMoveEvent", MouseMove)
interactor.AddObserver("LeftButtonReleaseEvent", EndZoom)

renWin.Render()
iren.Start()
