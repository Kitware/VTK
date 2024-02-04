#!/usr/bin/env python
#
# This test exercises usage of rshift operator to build pipelines with vtkAlgorithm and vtkDataObject.
import vtkmodules.test.Testing as vtkTesting
from vtkmodules.vtkFiltersCore import vtkContourFilter, vtkAppendPolyData
from vtkmodules.vtkFiltersGeneral import vtkTableBasedClipDataSet
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
from vtkmodules.util.execution_model import select_ports

class TestUpdateAndCall(vtkTesting.vtkTest):
    def testAll(self):
        w = vtkRTAnalyticSource(whole_extent=(0, 10, 0, 10, 0, 10))
        w1 = w()
        w2 = w.update().output
        self.assertEqual(w1.number_of_cells, 1000)
        self.assertEqual(w2.number_of_cells, 1000)
        self.assertTrue(w1 != w2)

        c = vtkContourFilter()
        c.SetNumberOfContours(1)
        c.SetValue(0, 100)
        contour = c(w1)
        self.assertEqual(contour.number_of_cells, 255)
        self.assertEqual((w >> c).update().output.number_of_cells, 255)

        a = vtkAppendPolyData()
        self.assertEqual(a([contour, contour]).number_of_cells, 255*2)

        clip = vtkTableBasedClipDataSet(value=100, generate_clipped_output=True)
        clips = clip(w())
        self.assertEqual(clips[0].number_of_points, 429)
        self.assertEqual(clips[1].number_of_points, 1266)

        self.assertEqual((w >> select_ports(clip, 1) >> c).update().output.number_of_cells, 421)

if __name__ == '__main__':
    vtkTesting.main([(TestUpdateAndCall, 'test')])
