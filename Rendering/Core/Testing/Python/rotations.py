#!/usr/bin/env python
from vtkmodules.vtkFiltersGeneral import vtkAxes
from vtkmodules.vtkIOGeometry import vtkBYUReader
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

# Create renderer stuff
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create pipeline
#
cow = vtkBYUReader()
cow.SetGeometryFileName(VTK_DATA_ROOT + "/Data/Viewpoint/cow.g")
cowMapper = vtkPolyDataMapper()
cowMapper.SetInputConnection(cow.GetOutputPort())
cowActor = vtkActor()
cowActor.SetMapper(cowMapper)
cowActor.GetProperty().SetDiffuseColor(0.9608,0.8706,0.7020)
cowAxesSource = vtkAxes()
cowAxesSource.SetScaleFactor(10)
cowAxesSource.SetOrigin(0,0,0)
cowAxesMapper = vtkPolyDataMapper()
cowAxesMapper.SetInputConnection(cowAxesSource.GetOutputPort())
cowAxes = vtkActor()
cowAxes.SetMapper(cowAxesMapper)
ren1.AddActor(cowAxes)
cowAxes.VisibilityOff()
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(cowActor)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(320,240)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(0)
ren1.GetActiveCamera().Dolly(1.4)
ren1.ResetCameraClippingRange()
cowAxes.VisibilityOn()
renWin.Render()
# render the image
#
# prevent the tk window from showing up then start the event loop
#
def RotateX():
    cowActor.SetOrientation(0,0,0)
    ren1.ResetCameraClippingRange()
    renWin.Render()
    renWin.Render()
    renWin.EraseOff()
    i = 1
    while i <= 6:
        cowActor.RotateX(60)
        renWin.Render()
        renWin.Render()
        i = i + 1

    renWin.EraseOn()

def RotateY():
    cowActor.SetOrientation(0,0,0)
    ren1.ResetCameraClippingRange()
    renWin.Render()
    renWin.Render()
    renWin.EraseOff()
    i = 1
    while i <= 6:
        cowActor.RotateY(60)
        renWin.Render()
        renWin.Render()
        i = i + 1

    renWin.EraseOn()

def RotateZ():
    cowActor.SetOrientation(0,0,0)
    ren1.ResetCameraClippingRange()
    renWin.Render()
    renWin.Render()
    renWin.EraseOff()
    i = 1
    while i <= 6:
        cowActor.RotateZ(60)
        renWin.Render()
        renWin.Render()
        i = i + 1

    renWin.EraseOn()

def RotateXY():
    cowActor.SetOrientation(0,0,0)
    cowActor.RotateX(60)
    ren1.ResetCameraClippingRange()
    renWin.Render()
    renWin.Render()
    renWin.EraseOff()
    i = 1
    while i <= 6:
        cowActor.RotateY(60)
        renWin.Render()
        renWin.Render()
        i = i + 1

    renWin.EraseOn()

RotateX()
RotateY()
RotateZ()
RotateXY()
renWin.EraseOff()
# --- end of script --
