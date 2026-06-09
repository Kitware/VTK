#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkImageData
from vtkmodules.vtkFiltersVerdict import vtkCellSizeFilter
from vtkmodules.test import Testing


class TestCellSizeFilter(Testing.vtkTest):
    def setUp(self):
        self.alg = vtkCellSizeFilter()

    def testEmptyImageDataNoSegfault(self):
        """Empty vtkImageData should not segfault regardless of compute flags."""
        image = vtkImageData()
        self.alg.SetInputDataObject(image)
        self.alg.SetComputeVertexCount(True)
        self.alg.SetComputeLength(True)
        self.alg.SetComputeArea(True)
        self.alg.SetComputeVolume(True)
        # Should complete without crashing
        self.alg.Update()
        output = self.alg.GetOutput()
        self.assertEqual(output.GetNumberOfCells(), 0)

    def testNonEmptyImageDataVertexCount(self):
        """Non-empty vtkImageData should still produce correct results."""
        image = vtkImageData()
        image.SetDimensions(10, 10, 10)
        self.alg.SetInputDataObject(image)
        self.alg.SetComputeVertexCount(True)
        self.alg.Update()
        output = self.alg.GetOutput()
        self.assertGreater(output.GetNumberOfCells(), 0)
        array = output.GetCellData().GetArray(self.alg.GetVertexCountArrayName())
        self.assertIsNotNone(array)


if __name__ == "__main__":
    Testing.main([(TestCellSizeFilter, "test")])
