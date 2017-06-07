#!/usr/bin/env python

import vtk
from vtk.test import Testing

class TestGetRangeDiscretizableColorTransferFunction(Testing.vtkTest):
    def testGetRangeDoubleStarArg(self):
        cmap = vtk.vtkDiscretizableColorTransferFunction()

        localRange = [-1, -1]
        cmap.GetRange(localRange)
        self.assertEqual(localRange[0], 0.0)
        self.assertEqual(localRange[1], 0.0)

    def testGetRangeTwoDoubleStarArg(self):
        cmap = vtk.vtkDiscretizableColorTransferFunction()

        localMin = vtk.mutable(-1)
        localMax = vtk.mutable(-1)
        cmap.GetRange(localMin, localMax)
        self.assertEqual(localMin, 0.0)
        self.assertEqual(localMax, 0.0)

    def testGetRangeNoArg(self):
        cmap = vtk.vtkDiscretizableColorTransferFunction()

        crange = cmap.GetRange()
        self.assertEqual(len(crange), 2)
        self.assertEqual(crange[0], 0.0)
        self.assertEqual(crange[1], 0.0)

if __name__ == "__main__":
    Testing.main([(TestGetRangeDiscretizableColorTransferFunction, 'test')])
