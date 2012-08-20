#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
#
# Create the data
#
points = vtk.vtkPoints()
polys = vtk.vtkCellArray()
i = 0
z = -5
while z < 30:
    xtraX = 0
    while xtraX < 90:
        xtraY = 0
        while xtraY < 90:
            x = -10
            y = -10
            x = expr.expr(globals(), locals(),["x","+","xtraX"])
            y = expr.expr(globals(), locals(),["y","+","xtraY"])
            if (expr.expr(globals(), locals(),["z","%","12"]) == 0):
                x = x + 1
                pass
            if (expr.expr(globals(), locals(),["z","%","12"]) == 1):
                x = x + 2
                pass
            if (expr.expr(globals(), locals(),["z","%","12"]) == 2):
                x = x + 3
                pass
            if (expr.expr(globals(), locals(),["z","%","12"]) == 3):
                x = x + 3
                y = y + 1
                pass
            if (expr.expr(globals(), locals(),["z","%","12"]) == 4):
                x = x + 3
                y = y + 2
                pass
            if (expr.expr(globals(), locals(),["z","%","12"]) == 5):
                x = x + 3
                y = y + 3
                pass
            if (expr.expr(globals(), locals(),["z","%","12"]) == 6):
                x = x + 2
                y = y + 3
                pass
            if (expr.expr(globals(), locals(),["z","%","12"]) == 7):
                x = x + 1
                y = y + 3
                pass
            if (expr.expr(globals(), locals(),["z","%","12"]) == 8):
                y = y + 3
                pass
            if (expr.expr(globals(), locals(),["z","%","12"]) == 9):
                y = y + 2
                pass
            if (expr.expr(globals(), locals(),["z","%","12"]) == 10):
                y = y + 1
                pass
            if (expr.expr(globals(), locals(),["(","xtraX","!=","30","and","xtraY","!=","30",")","or","(","xtraX","==","xtraY",")"])):
                polys.InsertNextCell(4)
                points.InsertPoint(i,expr.expr(globals(), locals(),["x","+","0"]),expr.expr(globals(), locals(),["y","+","0"]),z)
                polys.InsertCellPoint(i)
                i = i + 1
                points.InsertPoint(i,expr.expr(globals(), locals(),["x","+","20"]),expr.expr(globals(), locals(),["y","+","0"]),z)
                polys.InsertCellPoint(i)
                i = i + 1
                points.InsertPoint(i,expr.expr(globals(), locals(),["x","+","20"]),expr.expr(globals(), locals(),["y","+","20"]),z)
                polys.InsertCellPoint(i)
                i = i + 1
                points.InsertPoint(i,expr.expr(globals(), locals(),["x","+","0"]),expr.expr(globals(), locals(),["y","+","20"]),z)
                polys.InsertCellPoint(i)
                i = i + 1
                polys.InsertNextCell(4)
                points.InsertPoint(i,expr.expr(globals(), locals(),["x","+","4"]),expr.expr(globals(), locals(),["y","+","4"]),z)
                polys.InsertCellPoint(i)
                i = i + 1
                points.InsertPoint(i,expr.expr(globals(), locals(),["x","+","16"]),expr.expr(globals(), locals(),["y","+","4"]),z)
                polys.InsertCellPoint(i)
                i = i + 1
                points.InsertPoint(i,expr.expr(globals(), locals(),["x","+","16"]),expr.expr(globals(), locals(),["y","+","16"]),z)
                polys.InsertCellPoint(i)
                i = i + 1
                points.InsertPoint(i,expr.expr(globals(), locals(),["x","+","4"]),expr.expr(globals(), locals(),["y","+","16"]),z)
                polys.InsertCellPoint(i)
                i = i + 1
                pass
            if (expr.expr(globals(), locals(),["xtraX","!=","30","or","xtraY","!=","30"])):
                polys.InsertNextCell(4)
                points.InsertPoint(i,expr.expr(globals(), locals(),["x","+","8"]),expr.expr(globals(), locals(),["y","+","8"]),z)
                polys.InsertCellPoint(i)
                i = i + 1
                points.InsertPoint(i,expr.expr(globals(), locals(),["x","+","12"]),expr.expr(globals(), locals(),["y","+","8"]),z)
                polys.InsertCellPoint(i)
                i = i + 1
                points.InsertPoint(i,expr.expr(globals(), locals(),["x","+","12"]),expr.expr(globals(), locals(),["y","+","12"]),z)
                polys.InsertCellPoint(i)
                i = i + 1
                points.InsertPoint(i,expr.expr(globals(), locals(),["x","+","8"]),expr.expr(globals(), locals(),["y","+","12"]),z)
                polys.InsertCellPoint(i)
                i = i + 1
                pass
            xtraY = xtraY + 30

        xtraX = xtraX + 30

    z = z + 1

#
# Create a representation of the contours used as input
#
contours = vtk.vtkPolyData()
contours.SetPoints(points)
contours.SetPolys(polys)
contourMapper = vtk.vtkPolyDataMapper()
contourMapper.SetInputData(contours)
contourActor = vtk.vtkActor()
contourActor.SetMapper(contourMapper)
contourActor.GetProperty().SetColor(1,0,0)
contourActor.GetProperty().SetAmbient(1)
contourActor.GetProperty().SetDiffuse(0)
contourActor.GetProperty().SetRepresentationToWireframe()
ren1.AddViewProp(contourActor)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(10)
ren1.GetActiveCamera().Elevation(30)
ren1.ResetCameraClippingRange()
renWin.SetSize(300,300)
renWin.Render()
renWin.Render()
# prevent the tk window from showing up then start the event loop
#
# Create the contour to surface filter
#
f = vtk.vtkVoxelContoursToSurfaceFilter()
f.SetInputData(contours)
f.SetMemoryLimitInBytes(100000)
m = vtk.vtkPolyDataMapper()
m.SetInputConnection(f.GetOutputPort())
m.ScalarVisibilityOff()
m.ImmediateModeRenderingOn()
a = vtk.vtkActor()
a.SetMapper(m)
ren1.AddViewProp(a)
contourActor.VisibilityOff()
ren1.SetBackground(.1,.2,.4)
renWin.Render()
# --- end of script --
