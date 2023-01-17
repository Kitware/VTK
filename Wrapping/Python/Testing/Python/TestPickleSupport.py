#!/usr/bin/env python

# This tests the pickle support in vtk.util.pickle_support

import vtkmodules.test.Testing as vtkTesting

try:
    import numpy
except ImportError:
    print("This test requires numpy!")
    vtkTesting.skip()

import vtkmodules.util.pickle_support

from vtkmodules.vtkCommonCore import vtkIntArray
from vtkmodules.vtkCommonDataModel import vtkPolyData, vtkUnstructuredGrid, vtkImageData, vtkPointData
from vtkmodules.vtkFiltersCore import vtkAppendFilter
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource

import pickle

class TestPickleSupport(vtkTesting.vtkTest):

    def addPointData(self, data_set):
        arr = vtkIntArray()
        arr.SetName("Iota")
        arr.SetNumberOfComponents(1)
        arr.SetNumberOfTuples(data_set.GetNumberOfPoints())
        for i in range(0, data_set.GetNumberOfPoints()):
            arr.SetValue(i, i)
        data_set.GetPointData().AddArray(arr)


    def genericDataSetTest(self, data_set):
        num_points = data_set.GetNumberOfPoints()
        num_cells = data_set.GetNumberOfCells()
        self.addPointData(data_set)

        pickled = pickle.dumps(data_set)
        unpickled = pickle.loads(pickled)

        self.assertEqual(num_points, unpickled.GetNumberOfPoints())
        self.assertEqual(num_cells, unpickled.GetNumberOfCells())
        arr =  unpickled.GetPointData().GetArray("Iota")
        self.assertSequenceEqual(range(num_points), [arr.GetComponent(i, 0) for i in range(num_points)])

    def testPolyDataPickle(self):
        sphereSrc = vtkSphereSource()
        sphereSrc.Update()
        self.genericDataSetTest(sphereSrc.GetOutput())

    def testImageDataPickle(self):
        waveletSrc = vtkRTAnalyticSource()
        waveletSrc.Update()
        self.genericDataSetTest(waveletSrc.GetOutput())

    def testUnstructuredGridPickle(self):
        sphereSrc = vtkSphereSource()
        append = vtkAppendFilter()
        append.SetInputConnection(sphereSrc.GetOutputPort())
        append.Update()
        self.genericDataSetTest(append.GetOutput())


if __name__ == '__main__':
    vtkTesting.main([(TestPickleSupport, 'test')])
