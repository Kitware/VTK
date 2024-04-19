#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
)
from vtkmodules.vtkFiltersGeneral import vtkVoxelContoursToSurfaceFilter
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

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
#
# Create the data
#
points = vtkPoints()
polys = vtkCellArray()
i = 0
z = -5
while z < 30:
    xtraX = 0
    while xtraX < 90:
        xtraY = 0
        while xtraY < 90:
            x = -10
            y = -10
            x = x + xtraX
            y = y + xtraY
            if z % 12 == 0:
                x = x + 1
            if z % 12 == 1:
                x = x + 2
            if z % 12 == 2:
                x = x + 3
            if z % 12 == 3:
                x = x + 3
                y = y + 1
            if z % 12 == 4:
                x = x + 3
                y = y + 2
            if z % 12 == 5:
                x = x + 3
                y = y + 3
            if z % 12 == 6:
                x = x + 2
                y = y + 3
            if z % 12 == 7:
                x = x + 1
                y = y + 3
            if z % 12 == 8:
                y = y + 3
            if z % 12 == 9:
                y = y + 2
            if z % 12 == 10:
                y = y + 1

            if (xtraX != 30 and xtraY != 30) or (xtraX == xtraY):
                polys.InsertNextCell(4)
                points.InsertPoint(i, x + 0, y + 0, z)
                polys.InsertCellPoint(i)
                i += 1
                points.InsertPoint(i, x + 20, y + 0, z)
                polys.InsertCellPoint(i)
                i += 1
                points.InsertPoint(i, x + 20, y + 20, z)
                polys.InsertCellPoint(i)
                i += 1
                points.InsertPoint(i, x + 0, y + 20, z)
                polys.InsertCellPoint(i)
                i += 1
                polys.InsertNextCell(4)
                points.InsertPoint(i, x + 4, y + 4, z)
                polys.InsertCellPoint(i)
                i += 1
                points.InsertPoint(i, x + 16, y + 4, z)
                polys.InsertCellPoint(i)
                i += 1
                points.InsertPoint(i, x + 16, y + 16, z)
                polys.InsertCellPoint(i)
                i += 1
                points.InsertPoint(i, x + 4, y + 16, z)
                polys.InsertCellPoint(i)
                i += 1

            if xtraX != 30 or xtraY != 30:
                polys.InsertNextCell(4)
                points.InsertPoint(i, x + 8, y + 8, z)
                polys.InsertCellPoint(i)
                i += 1
                points.InsertPoint(i, x + 12, y + 8, z)
                polys.InsertCellPoint(i)
                i += 1
                points.InsertPoint(i, x + 12, y + 12, z)
                polys.InsertCellPoint(i)
                i += 1
                points.InsertPoint(i, x + 8, y + 12, z)
                polys.InsertCellPoint(i)
                i += 1

            xtraY += 30

        xtraX += 30

    z += 1

#
# Create a representation of the contours used as input
#
contours = vtkPolyData()
contours.SetPoints(points)
contours.SetPolys(polys)

contourMapper = vtkPolyDataMapper()
contourMapper.SetInputData(contours)

contourActor = vtkActor()
contourActor.SetMapper(contourMapper)
contourActor.GetProperty().SetColor(1, 0, 0)
contourActor.GetProperty().SetAmbient(1)
contourActor.GetProperty().SetDiffuse(0)
contourActor.GetProperty().SetRepresentationToWireframe()

ren1.AddViewProp(contourActor)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(10)
ren1.GetActiveCamera().Elevation(30)
ren1.ResetCameraClippingRange()

renWin.SetSize(300, 300)

renWin.Render()
# renWin.Render()

# Create the contour to surface filter
#
f = vtkVoxelContoursToSurfaceFilter()
f.SetInputData(contours)
f.SetMemoryLimitInBytes(100000)

m = vtkPolyDataMapper()
m.SetInputConnection(f.GetOutputPort())
m.ScalarVisibilityOff()

a = vtkActor()
a.SetMapper(m)

ren1.AddViewProp(a)

contourActor.VisibilityOff()

ren1.SetBackground(.1, .2, .4)

renWin.Render()
#iren.Start()
