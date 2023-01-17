#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkConvexPointSet,
    vtkUnstructuredGrid,
)
from vtkmodules.vtkFiltersCore import (
    vtkContourFilter,
    vtkElevationFilter,
)
from vtkmodules.vtkFiltersGeneral import vtkClipDataSet
from vtkmodules.vtkFiltersGeometry import vtkDataSetSurfaceFilter
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
    vtkDataSetMapper,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2

# create points in the configuration of an octant with one 2:1 face
#
points = vtkPoints()
aConvex = vtkConvexPointSet()
points.InsertPoint(0, 0, 0, 0)
points.InsertPoint(1, 1, 0, 0)
points.InsertPoint(2, 1, 1, 0)
points.InsertPoint(3, 0, 1, 0)
points.InsertPoint(4, 0, 0, 1)
points.InsertPoint(5, 1, 0, 1)
points.InsertPoint(6, 1, 1, 1)
points.InsertPoint(7, 0, 1, 1)
points.InsertPoint(8, 0.5, 0, 0)
points.InsertPoint(9, 1, 0.5, 0)
points.InsertPoint(10, 0.5, 1, 0)
points.InsertPoint(11, 0, 0.5, 0)
points.InsertPoint(12, 0.5, 0.5, 0)
i = 0
while i < 13:
    aConvex.GetPointIds().InsertId(i, i)
    i += 1

aConvexGrid = vtkUnstructuredGrid()
aConvexGrid.Allocate(1, 1)
aConvexGrid.InsertNextCell(aConvex.GetCellType(), aConvex.GetPointIds())
aConvexGrid.SetPoints(points)

# Display the cell
dsm = vtkDataSetMapper()
dsm.SetInputData(aConvexGrid)
a = vtkActor()
a.SetMapper(dsm)
a.GetProperty().SetColor(0, 1, 0)

# Contour and clip the cell with elevation scalars
ele = vtkElevationFilter()
ele.SetInputData(aConvexGrid)
ele.SetLowPoint(-1, -1, -1)
ele.SetHighPoint(1, 1, 1)
ele.SetScalarRange(-1, 1)

# Clip
#
clip = vtkClipDataSet()
clip.SetInputConnection(ele.GetOutputPort())
clip.SetValue(0.5)
g = vtkDataSetSurfaceFilter()
g.SetInputConnection(clip.GetOutputPort())
map = vtkPolyDataMapper()
map.SetInputConnection(g.GetOutputPort())
map.ScalarVisibilityOff()
clipActor = vtkActor()
clipActor.SetMapper(map)
clipActor.GetProperty().SetColor(1, 0, 0)
clipActor.AddPosition(2, 0, 0)

# Contour
#
contour = vtkContourFilter()
contour.SetInputConnection(ele.GetOutputPort())
contour.SetValue(0, 0.5)
g2 = vtkDataSetSurfaceFilter()
g2.SetInputConnection(contour.GetOutputPort())
map2 = vtkPolyDataMapper()
map2.SetInputConnection(g2.GetOutputPort())
map2.ScalarVisibilityOff()
contourActor = vtkActor()
contourActor.SetMapper(map2)
contourActor.GetProperty().SetColor(1, 0, 0)
contourActor.AddPosition(1, 2, 0)

# Create graphics stuff
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(a)
ren1.AddActor(clipActor)
ren1.AddActor(contourActor)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(250, 150)

aCam = vtkCamera()
aCam.SetFocalPoint(1.38705, 1.37031, 0.639901)
aCam.SetPosition(1.89458, -5.07106, -4.17439)
aCam.SetViewUp(0.00355726, 0.598843, -0.800858)
aCam.SetClippingRange(4.82121, 12.1805)

ren1.SetActiveCamera(aCam)

renWin.Render()

cam1 = ren1.GetActiveCamera()
cam1.Zoom(1.5)

# render the image
#
renWin.Render()

#iren.Start()
