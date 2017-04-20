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

rt = vtk.vtkRTAnalyticSource()

p2c = vtk.vtkPointDataToCellData()
p2c.SetInputConnection(rt.GetOutputPort())
p2c.Update()

d1 = dsa.WrapDataObject(p2c.GetOutput())

vtkm_p2c = vtk.vtkmAverageToCells()
vtkm_p2c.SetInputData(rt.GetOutput())
vtkm_p2c.SetInputArrayToProcess(0, 0, 0, vtk.vtkDataObject.FIELD_ASSOCIATION_POINTS, "RTData")
vtkm_p2c.Update()

d2 = dsa.WrapDataObject(vtkm_p2c.GetOutput())

assert (algs.max(algs.abs(d1.CellData['RTData'] - d2.CellData['RTData'])) < 10E-4)
