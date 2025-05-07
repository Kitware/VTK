import sys

try:
    import numpy
except ImportError:
    print("This test requires numpy!")
    from vtkmodules.test import Testing
    Testing.skip()

from vtkmodules.vtkAcceleratorsVTKmFilters import vtkmAverageToCells
from vtkmodules.vtkCommonCore import (
    vtkDoubleArray,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import (
    VTK_LINE,
    VTK_QUAD,
    VTK_TRIANGLE,
    vtkDataObject,
    vtkUnstructuredGrid,
)
from vtkmodules.vtkFiltersCore import vtkPointDataToCellData
from vtkmodules.vtkFiltersGeneral import vtkClipDataSet
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
from vtkmodules.numpy_interface import dataset_adapter as dsa
from vtkmodules.numpy_interface import algorithms as algs

def test_dataset(ds):
  p2c = vtkPointDataToCellData()
  p2c.SetInputData(ds)
  p2c.Update()

  d1 = dsa.WrapDataObject(p2c.GetOutput())

  viskores_p2c = vtkmAverageToCells()
  viskores_p2c.SetInputData(ds)
  viskores_p2c.SetInputArrayToProcess(0, 0, 0, vtkDataObject.FIELD_ASSOCIATION_POINTS, "RTData")
  viskores_p2c.Update()

  d2 = dsa.WrapDataObject(viskores_p2c.GetOutput())

  rtD1 = d1.PointData['RTData']
  rtD2 = d2.PointData['RTData']

  assert (algs.max(algs.abs(rtD1 - rtD2)) < 10E-4)

print("Testing simple debugging grid...")
# This dataset matches the example values in vtkmCellSetExplicit:
dbg = vtkUnstructuredGrid()
dbg.SetPoints(vtkPoints())
dbg.GetPoints().SetNumberOfPoints(7)
dbg.InsertNextCell(VTK_TRIANGLE, 3, [0, 1, 2])
dbg.InsertNextCell(VTK_QUAD,     4, [0, 1, 3, 4])
dbg.InsertNextCell(VTK_TRIANGLE, 3, [1, 3, 5])
dbg.InsertNextCell(VTK_LINE,     2, [5, 6])

dbgRt = vtkDoubleArray()
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
source = vtkRTAnalyticSource()
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
clip = vtkClipDataSet()
clip.SetInputData(imgData)
clip.SetValue(clipScalar)
clip.Update()
ugrid = clip.GetOutput()
test_dataset(ugrid)
print("Success!")
