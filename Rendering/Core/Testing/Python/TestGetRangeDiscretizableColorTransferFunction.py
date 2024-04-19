#!/usr/bin/env python

from vtkmodules.vtkCommonCore import reference
from vtkmodules.vtkRenderingCore import vtkDiscretizableColorTransferFunction
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.test import Testing

class TestGetRangeDiscretizableColorTransferFunction(Testing.vtkTest):
    def testGetRangeDoubleStarArg(self):
        cmap = vtkDiscretizableColorTransferFunction()

        localRange = [-1, -1]
        cmap.GetRange(localRange)
        self.assertEqual(localRange[0], 0.0)
        self.assertEqual(localRange[1], 0.0)

    def testGetRangeTwoDoubleStarArg(self):
        cmap = vtkDiscretizableColorTransferFunction()

        localMin = reference(-1)
        localMax = reference(-1)
        cmap.GetRange(localMin, localMax)
        self.assertEqual(localMin, 0.0)
        self.assertEqual(localMax, 0.0)

    def testGetRangeNoArg(self):
        cmap = vtkDiscretizableColorTransferFunction()

        crange = cmap.GetRange()
        self.assertEqual(len(crange), 2)
        self.assertEqual(crange[0], 0.0)
        self.assertEqual(crange[1], 0.0)

if __name__ == "__main__":
    Testing.main([(TestGetRangeDiscretizableColorTransferFunction, 'test')])
