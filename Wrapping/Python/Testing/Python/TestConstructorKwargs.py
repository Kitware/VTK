#!/usr/bin/env python
#
# This test exercises passing kwargs in the constructor of wrapped VTK classes.

import vtkmodules.test.Testing as vtkTesting
from vtkmodules.vtkCommonDataModel import vtkImageData
from vtkmodules.vtkFiltersCore import vtkElevationFilter
from vtkmodules.vtkFiltersSources import vtkSphereSource

class TestConstructorKwargs(vtkTesting.vtkTest):
    def testCase1KwargsBasic(self):
        s = vtkSphereSource(radius=10)
        self.assertEqual(s.radius, 10)

    def testCase2KwargsMulti(self):
        s = vtkSphereSource(center=(1, 0, 0), generate_normals=False, radius=10, theta_resolution=20)
        self.assertTupleEqual(s.center, (1, 0, 0))
        self.assertEqual(s.generate_normals, False)
        self.assertEqual(s.radius, 10)
        self.assertEqual(s.theta_resolution, 20)

    def testCase3KwargsVTKObjectAttribute(self):
        s = vtkSphereSource(center=(1, 0, 0), generate_normals=False, radius=10, theta_resolution=20)
        e = vtkElevationFilter(low_point=(1, 0, -10), high_point=(1, 0, 10), input_connection=s.output_port)
        self.assertEqual(e.GetInputConnection(0, 0), s.output_port)
        self.assertEqual(e.input_algorithm, s)

    def testCase4KwargsIncorrectPropertyName(self):
        # Check that constructor fails when a kwarg has a typo, here 'centre' instead of 'center'
        with self.assertRaisesRegex(TypeError, "Unexpected keyword argument 'centre' for 'vtkmodules.vtkFiltersSources.vtkSphereSource' constructor"):
            s = vtkSphereSource(centre=(1, 0, 0), generate_normals=False, radius=10, theta_resolution=20)

    def testCase5KwargsIncorrectPropertyValueType(self):
        # Check that constructor fails when a kwarg is assigned a wrong value, here a 2-element tuple is passed to dimensions instead of a 3-element tuple.
        with self.assertRaisesRegex(TypeError, "no overloads of SetDimensions\(\) take 2 arguments"):
            i = vtkImageData(dimensions=(1, 2))

    def testCase6KwargsPipeline(self):
        s = vtkSphereSource(center=(1, 0, 0), generate_normals=False, radius=10, theta_resolution=20)
        e = vtkElevationFilter(low_point=(1, 0, -10), high_point=(1, 0, 10), input_connection=s.output_port)
        e.Update()
        self.assertEqual(e.output.number_of_points, 122)
        self.assertEqual(e.output.number_of_polys, 240)
        self.assertEqual(e.output.point_data.scalars.range, (0, 1))

    def testCase7SubclassKwargs(self):

        class MyHigherOrderSphereSource(vtkSphereSource):
            def __init__(self, *args, **kwargs):
                if 'order' in kwargs:
                    self.order = kwargs.pop('order') # so that VTK C++ class doesn't see it.
                super().__init__(**kwargs)

        s = MyHigherOrderSphereSource(center=(1, 0, 0), generate_normals=False, radius=10, theta_resolution=20, order=2)
        self.assertEqual(s.order, 2)
        self.assertTupleEqual(s.center, (1, 0, 0))
        self.assertEqual(s.generate_normals, False)
        self.assertEqual(s.radius, 10)
        self.assertEqual(s.theta_resolution, 20)


if __name__ == '__main__':
    vtkTesting.main([(TestConstructorKwargs, 'test')])
