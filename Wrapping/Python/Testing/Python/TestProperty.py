#!/usr/bin/env python
#
# This test exercises setting and getting properties of wrapped C++ objects.

import vtkmodules.test.Testing as vtkTesting
from vtkmodules.vtkCommonDataModel import vtkBoundingBox, vtkImageData
from vtkmodules.vtkFiltersCore import vtkContourFilter
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource

class TestProperty(vtkTesting.vtkTest):
    def testVtkObject(self):
        i = vtkImageData()

        i.dimensions = (2, 2, 3)
        self.assertTupleEqual(i.dimensions, (2, 2, 3))
        self.assertTupleEqual(i.GetDimensions(), (2, 2, 3))

        i.SetDimensions(2, 2, 4)
        self.assertTupleEqual(i.dimensions, (2, 2, 4))

        # check that i.number_of_points = 2 fails because number_of_points is a read-only property.
        with self.assertRaisesRegex(AttributeError, "attribute 'number_of_points' of 'vtkmodules.vtkCommonDataModel.vtkImageData' objects is not writable"):
            i.number_of_points = 2

        i.debug = True
        self.assertEqual(i.debug, True)
        self.assertEqual(i.GetDebug(), True)

        i.SetDebug(False)
        self.assertEqual(i.debug, False)

    def testSpecialObject(self):
        b = vtkBoundingBox()
        b.bounds = [-10, 10, -10, 10, 0, 10]
        self.assertTupleEqual(b.max_point, (10.0, 10.0, 10.0))
        self.assertTupleEqual(b.min_point, (-10.0, -10.0, 0.0))


if __name__ == '__main__':
    vtkTesting.main([(TestProperty, 'test')])
