#!/usr/bin/env python

import vtk
from vtk.test import Testing

class TestGetRangeColorTransferFunction(Testing.vtkTest):
    def testGetRangeDoubleStarArg(self):
        cmap = vtk.vtkColorTransferFunction()

        localRange = [-1, -1]
        cmap.GetRange(localRange)
        self.assertEqual(localRange[0], 0.0)
        self.assertEqual(localRange[1], 0.0)

    def testGetRangeTwoDoubleStarArg(self):
        cmap = vtk.vtkColorTransferFunction()

        localMin = vtk.mutable(-1)
        localMax = vtk.mutable(-1)
        cmap.GetRange(localMin, localMax)
        self.assertEqual(localMin, 0.0)
        self.assertEqual(localMax, 0.0)

    def testGetRangeNoArg(self):
        cmap = vtk.vtkColorTransferFunction()

        crange = cmap.GetRange()
        self.assertEqual(len(crange), 2)
        self.assertEqual(crange[0], 0.0)
        self.assertEqual(crange[1], 0.0)

if __name__ == "__main__":
    Testing.main([(TestGetRangeColorTransferFunction, 'test')])
