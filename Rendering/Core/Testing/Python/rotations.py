#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create renderer stuff
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create pipeline
#
cow = vtk.vtkBYUReader()
cow.SetGeometryFileName("" + str(VTK_DATA_ROOT) + "/Data/Viewpoint/cow.g")
cowMapper = vtk.vtkPolyDataMapper()
cowMapper.SetInputConnection(cow.GetOutputPort())
cowActor = vtk.vtkActor()
cowActor.SetMapper(cowMapper)
cowActor.GetProperty().SetDiffuseColor(0.9608,0.8706,0.7020)
cowAxesSource = vtk.vtkAxes()
cowAxesSource.SetScaleFactor(10)
cowAxesSource.SetOrigin(0,0,0)
cowAxesMapper = vtk.vtkPolyDataMapper()
cowAxesMapper.SetInputConnection(cowAxesSource.GetOutputPort())
cowAxes = vtk.vtkActor()
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
def RotateX (__vtk__temp0=0,__vtk__temp1=0):
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

def RotateY (__vtk__temp0=0,__vtk__temp1=0):
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

def RotateZ (__vtk__temp0=0,__vtk__temp1=0):
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

def RotateXY (__vtk__temp0=0,__vtk__temp1=0):
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
