#!/usr/bin/env python

# This tests vtkAMRSliceFilter

from vtkmodules.vtkFiltersAMR import vtkAMRSliceFilter
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkIOAMR import vtkAMREnzoReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCompositePolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

def NumCells(out):
  n =0
  for i in range(out.GetNumberOfLevels()):
    for j in range(out.GetNumberOfDataSets(i)):
      m = out.GetDataSet(i,j).GetNumberOfCells()
      #print (i,j,m)
      n = n + m
  return n

filename= VTK_DATA_ROOT +"/Data/AMR/Enzo/DD0010/moving7_0010.hierarchy"
datafieldname = "TotalEnergy"

reader = vtkAMREnzoReader()
reader.SetFileName(filename)
reader.SetMaxLevel(10)
reader.SetCellArrayStatus(datafieldname,1)

filter = vtkAMRSliceFilter()
filter.SetInputConnection(reader.GetOutputPort())
filter.SetNormal(1)
filter.SetOffsetFromOrigin(0.86)
filter.SetMaxResolution(7)
filter.Update()
out = filter.GetOutputDataObject(0)
out.Audit()
if NumCells(out) != 800:
  print("Wrong number of cells in the output")
  exit(1)

# render
surface = vtkGeometryFilter()
surface.SetInputData(out)

mapper = vtkCompositePolyDataMapper()
mapper.SetInputConnection(surface.GetOutputPort())
mapper.SetScalarModeToUseCellFieldData()
mapper.SelectColorArray(datafieldname)
mapper.SetScalarRange(1.2e-7, 1.5e-3)

actor = vtkActor()
actor.SetMapper(mapper)

renderer = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(renderer)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

renderer.AddActor(actor)
renderer.GetActiveCamera().SetPosition(1, 0, 0)
renderer.ResetCamera()
renWin.SetSize(300, 300)
iren.Initialize()
renWin.Render()
iren.Start()
