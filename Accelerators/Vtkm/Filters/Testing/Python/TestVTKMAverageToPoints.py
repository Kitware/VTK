import sys

try:
    import numpy
except ImportError:
    print("Numpy (http://numpy.scipy.org) not found.")
    print("This test requires numpy!")
    from vtk.test import Testing
    Testing.skip()

import vtk
from vtk.numpy_interface import dataset_adapter as dsa
from vtk.numpy_interface import algorithms as algs

def test_dataset(ds):
  p2c = vtk.vtkPointDataToCellData()
  p2c.SetInputData(ds)
  p2c.Update()

  c2p = vtk.vtkCellDataToPointData()
  c2p.SetInputConnection(p2c.GetOutputPort())
  c2p.Update()

  d1 = dsa.WrapDataObject(c2p.GetOutput())

  c2p = vtk.vtkmAverageToPoints()
  c2p.SetInputData(p2c.GetOutput())
  c2p.SetInputArrayToProcess(0, 0, 0, vtk.vtkDataObject.FIELD_ASSOCIATION_CELLS, "RTData")
  c2p.Update()

  d2 = dsa.WrapDataObject(c2p.GetOutput())

  rtD1 = d1.PointData['RTData']
  rtD2 = d2.PointData['RTData']

  assert (algs.max(algs.abs(rtD1 - rtD2)) < 10E-4)

print("Testing simple debugging grid...")
# This dataset matches the example values in vtkmCellSetExplicit:
dbg = vtk.vtkUnstructuredGrid()
dbg.SetPoints(vtk.vtkPoints())
dbg.GetPoints().SetNumberOfPoints(7)
dbg.InsertNextCell(vtk.VTK_TRIANGLE, 3, [0, 1, 2])
dbg.InsertNextCell(vtk.VTK_QUAD,     4, [0, 1, 3, 4])
dbg.InsertNextCell(vtk.VTK_TRIANGLE, 3, [1, 3, 5])
dbg.InsertNextCell(vtk.VTK_LINE,     2, [5, 6])

dbgRt = vtk.vtkDoubleArray()
dbgRt.SetNumberOfTuples(7)
dbgRt.SetName('RTData')
dbgRt.SetValue(0, 17.40)
dbgRt.SetValue(1, 123.0)
dbgRt.SetValue(2, 28.60)
dbgRt.SetValue(3, 19.47)
dbgRt.SetValue(4, 3.350)
dbgRt.SetValue(5, 0.212)
dbgRt.SetValue(6, 1023.)
dbg.GetPointData().AddArray(dbgRt)

test_dataset(dbg)
print("Success!")

print("Testing homogeneous image data...")
source = vtk.vtkRTAnalyticSource()
source.Update()
imgData = source.GetOutput()
test_dataset(imgData)
print("Success!")

d = dsa.WrapDataObject(imgData)
rtData = d.PointData['RTData']
rtMin = algs.min(rtData)
rtMax = algs.max(rtData)
clipScalar = 0.5 * (rtMin + rtMax)

print("Testing non-homogeneous unstructured grid...")
clip = vtk.vtkClipDataSet()
clip.SetInputData(imgData)
clip.SetValue(clipScalar)
clip.Update()
ugrid = clip.GetOutput()
test_dataset(ugrid)
print("Success!")
