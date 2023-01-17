#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
)
from vtkmodules.vtkFiltersCore import vtkPointDataToCellData

poly = vtkPolyData()
pts = vtkPoints()
pts.InsertNextPoint( 0,0,0 )
pts.InsertNextPoint( 1,0,0 )
pts.InsertNextPoint( 1,1,0 )
pts.InsertNextPoint( 0,1,0 )
scalars = vtkFloatArray()
scalars.SetName('foo')
for i in range(0,4):
    scalars.InsertNextValue(float(i+1))
poly.GetPointData().SetScalars(scalars)

cells = vtkCellArray()
cells.InsertNextCell(0)
cells.InsertNextCell(1)
for i in range(0,4):
    cells.InsertCellPoint(i)

poly.SetPoints(pts)
poly.SetPolys(cells)
print('PolyData has {} points and {} cells'.format(poly.GetNumberOfPoints(),poly.GetNumberOfCells()))

celldata=vtkPointDataToCellData()
celldata.SetInputData(poly)
celldata.Update()
# --- end of script ---
