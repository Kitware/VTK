#!/usr/bin/env python
import vtk

poly = vtk.vtkPolyData()
pts = vtk.vtkPoints()
pts.InsertNextPoint( 0,0,0 )
pts.InsertNextPoint( 1,0,0 )
pts.InsertNextPoint( 1,1,0 )
pts.InsertNextPoint( 0,1,0 )
scalars = vtk.vtkFloatArray()
scalars.SetName('foo')
for i in range(0,4):
    scalars.InsertNextValue(float(i+1))
poly.GetPointData().SetScalars(scalars)

cells = vtk.vtkCellArray()
cells.InsertNextCell(0)
cells.InsertNextCell(1)
for i in range(0,4):
    cells.InsertCellPoint(i)

poly.SetPoints(pts)
poly.SetPolys(cells)
print('PolyData has {} points and {} cells'.format(poly.GetNumberOfPoints(),poly.GetNumberOfCells()))

celldata=vtk.vtkPointDataToCellData()
celldata.SetInputData(poly)
celldata.Update()
# --- end of script ---
