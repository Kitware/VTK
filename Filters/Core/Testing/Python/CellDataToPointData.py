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
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create a 2*2 cell/3*3 pt structuredgrid
points = vtkPoints()
points.InsertNextPoint(-1,1,0)
points.InsertNextPoint(0,1,0)
points.InsertNextPoint(1,1,0)
points.InsertNextPoint(-1,0,0)
points.InsertNextPoint(0,0,0)
points.InsertNextPoint(1,0,0)
points.InsertNextPoint(-1,-1,0)
points.InsertNextPoint(0,-1,0)
points.InsertNextPoint(1,-1,0)
faceColors = vtkFloatArray()
faceColors.InsertNextValue(0)
faceColors.InsertNextValue(1)
faceColors.InsertNextValue(1)
faceColors.InsertNextValue(2)
sgrid = vtkStructuredGrid()
sgrid.SetDimensions(3,3,1)
sgrid.SetPoints(points)
sgrid.GetCellData().SetScalars(faceColors)
Cell2Point = vtkCellDataToPointData()
Cell2Point.SetInputData(sgrid)
Cell2Point.PassCellDataOn()
Cell2Point.Update()
mapper = vtkDataSetMapper()
mapper.SetInputData(Cell2Point.GetStructuredGridOutput())
mapper.SetScalarModeToUsePointData()
mapper.SetScalarRange(0,2)
actor = vtkActor()
actor.SetMapper(mapper)
# Add the actors to the renderer, set the background and size
ren1.AddActor(actor)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(256,256)
# render the image
iren.Initialize()
# --- end of script --
