#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create some data, remove some polygons
pd = vtk.vtkPolyData()
pts = vtk.vtkPoints()
polys = vtk.vtkCellArray()
pd.SetPoints(pts)
pd.SetPolys(polys)
xRes = 10
yRes = 10
xPtsRes = xRes + 1
yPtsRes = yRes + 1

# insert points
j = 0
while j < yPtsRes:
    i = 0
    while i < xPtsRes:
        pts.InsertNextPoint(i, j, 0.0)
        i += 1
    j += 1

# insert cells
j = 1
while j <= yRes:
    i = 1
    while i <= xRes:
        cellId = i - 1 + yRes * (j - 1)
        if (cellId != 48 and cellId != 12 and cellId != 13
             and cellId != 23 and cellId != 60 and cellId != 83
             and cellId != 72 and cellId != 76 and cellId != 77
             and cellId != 78 and cellId != 87):
            polys.InsertNextCell(4)
            polys.InsertCellPoint(i - 1 + ((j - 1) * yPtsRes))
            polys.InsertCellPoint(i + ((j - 1) * yPtsRes))
            polys.InsertCellPoint(i + (j * yPtsRes))
            polys.InsertCellPoint(i - 1 + (j * yPtsRes))
            pass
        i += 1
    j += 1

# Fill the holes
fill = vtk.vtkFillHolesFilter()
fill.SetInputData(pd)
fill.SetHoleSize(20.0)

# Mapping and actor
map = vtk.vtkPolyDataMapper()
#    map SetInput pd
map.SetInputConnection(fill.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(map)
actor.GetProperty().SetColor(1, 0, 0)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(300, 300)

# render the image
#
iren.Initialize()
#iren.Start()
