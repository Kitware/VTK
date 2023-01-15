#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
)
from vtkmodules.vtkFiltersModeling import vtkFillHolesFilter
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

# Create some data, remove some polygons
pd = vtkPolyData()
pts = vtkPoints()
polys = vtkCellArray()
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
        i += 1
    j += 1

# Fill the holes
fill = vtkFillHolesFilter()
fill.SetInputData(pd)
fill.SetHoleSize(20.0)

# Mapping and actor
map = vtkPolyDataMapper()
#    map SetInput pd
map.SetInputConnection(fill.GetOutputPort())

actor = vtkActor()
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
