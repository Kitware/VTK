#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import vtkStructuredGrid
from vtkmodules.vtkFiltersCore import vtkCellDataToPointData
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# cell scalars to point scalars
# get the interactor ui
# Create the RenderWindow, Renderer and RenderWindowInteractor
ren1 = vtkRenderer(background=[0.1, 0.2, 0.4])
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create a 2*2 cell/3*3 pt structuredgrid
points = vtkPoints()
for pt in [(-1,1,0), (0,1,0), (1,1,0), (-1,0,0),
           (0,0,0), (1,0,0), (-1,-1,0), (0,-1,0),
           (1,-1,0)]:
    points.InsertNextPoint(pt)
faceColors = vtkFloatArray()
for val in (0, 1, 1, 2):
    faceColors.InsertNextValue(val)
sgrid = vtkStructuredGrid(dimensions=(3,3,1))
sgrid.points = points
sgrid.GetCellData().SetScalars(faceColors)
Cell2Point = vtkCellDataToPointData(pass_cell_data=True)
mapper = vtkDataSetMapper()
mapper.SetScalarModeToUsePointData()
mapper.SetScalarRange(0,2)
sgrid >> Cell2Point >> mapper
actor = vtkActor(mapper=mapper)
# Add the actors to the renderer, set the background and size
ren1.AddActor(actor)
renWin.SetSize(256,256)
# render the image
iren.Initialize()
# --- end of script --
