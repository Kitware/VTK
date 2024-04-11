#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()


RENWIN_WIDTH = 512
RENWIN_HEIGHT = 512


def setCameraTravelMatrix(cam, mat):
    global renWin
    cam.SetModelTransformMatrix(mat)
    cam.Modified()
    p2w = vtk.vtkMatrix4x4()
    p2w.DeepCopy(mat)
    p2w.Invert()
    renWin.SetPhysicalToWorldMatrix(p2w)


class MoveObserver(object):
    def __init__(self):
        self.moving = False

    def __call__(self, caller, ev):
        global ren1, renWin
        if ev == "LeftButtonPressEvent":
            self.moving = True
        elif ev == "LeftButtonReleaseEvent":
            self.moving = False
        elif ev == "MouseMoveEvent" and self.moving == True:
            x, y = caller.GetEventPosition()
            cam = ren1.GetActiveCamera()
            mtm = cam.GetModelTransformMatrix()
            tform = vtk.vtkTransform()
            tform.Identity()
            rotate = [3, (x / float(RENWIN_WIDTH)) - 0.5, (y / float(RENWIN_HEIGHT)) - 0.5, 0]
            tform.RotateWXYZ(*rotate)
            mtm.Multiply4x4(mtm, tform.GetMatrix(), mtm)
            setCameraTravelMatrix(cam, mtm)
            renWin.Render()
        elif ev == "RightButtonPressEvent":
            mat = vtk.vtkMatrix4x4()
            mat.Identity()
            setCameraTravelMatrix(ren1.GetActiveCamera(), mat)
            renWin.Render()


cone1 = vtk.vtkConeSource()
cone1.SetResolution(32)

cone2 = vtk.vtkConeSource()
cone2.SetResolution(32)

mapper1 = vtk.vtkPolyDataMapper()
mapper1.SetInputConnection(cone1.GetOutputPort())

mapper2 = vtk.vtkPolyDataMapper()
mapper2.SetInputConnection(cone2.GetOutputPort())

ren1 = vtk.vtkRenderer()

worldActor = vtk.vtkActor()
worldActor.SetMapper(mapper1)
# Red cone in world coordinates (the default)
worldActor.GetProperty().SetColor(1.0, 0.0, 0.0)
worldActor.SetCoordinateSystemToWorld()
worldActor.SetCoordinateSystemRenderer(ren1)

physicalActor = vtk.vtkActor()
physicalActor.SetMapper(mapper2)
# Green cone in physical coordinates
physicalActor.GetProperty().SetColor(0.0, 1.0, 0.0)
physicalActor.SetCoordinateSystemToPhysical()
physicalActor.SetCoordinateSystemRenderer(ren1)

ren1.AddActor(worldActor)
ren1.AddActor(physicalActor)

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(RENWIN_WIDTH, RENWIN_HEIGHT)

# Set up some non-idendity ModelTransformMatrix for the test
mtm = vtk.vtkMatrix4x4()
tform = vtk.vtkTransform()
tform.Identity()
translate = [0.5, 0.5, 0]
tform.Translate(*translate)
mtm.Multiply4x4(mtm, tform.GetMatrix(), mtm)
setCameraTravelMatrix(ren1.GetActiveCamera(),mtm)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

observer = MoveObserver()
iren.AddObserver(vtk.vtkCommand.MouseMoveEvent, observer)
iren.AddObserver(vtk.vtkCommand.LeftButtonPressEvent, observer)
iren.AddObserver(vtk.vtkCommand.LeftButtonReleaseEvent, observer)
iren.AddObserver(vtk.vtkCommand.RightButtonPressEvent, observer)
userStyle = vtk.vtkInteractorStyleUser()
iren.SetInteractorStyle(userStyle)

iren.Initialize()
renWin.Render()
ren1.ResetCamera()
iren.Start()

# --- end of script --
