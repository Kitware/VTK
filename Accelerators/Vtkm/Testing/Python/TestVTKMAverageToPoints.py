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

c2p = vtk.vtkCellDataToPointData()
c2p.SetInputConnection(p2c.GetOutputPort())
c2p.Update()

d1 = dsa.WrapDataObject(c2p.GetOutput())

c2p = vtk.vtkmAverageToPoints()
c2p.SetInputData(p2c.GetOutput())
c2p.SetInputArrayToProcess(0, 0, 0, vtk.vtkDataObject.FIELD_ASSOCIATION_CELLS, "RTData")
c2p.Update()

d2 = dsa.WrapDataObject(c2p.GetOutput())

assert (algs.max(algs.abs(d1.PointData['RTData'] - d2.PointData['RTData'])) < 10E-4)
