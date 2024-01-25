#!/usr/bin/env python
#
# This test exercises usage of rshift operator to build pipelines with vtkAlgorithm and vtkDataObject.

import vtkmodules.test.Testing as vtkTesting
from vtkmodules.vtkCommonDataModel import vtkImageData, vtkPolyData
from vtkmodules.vtkFiltersCore import vtkElevationFilter, vtkPolyDataConnectivityFilter, vtkPolyDataNormals, VTK_EXTRACT_ALL_REGIONS
from vtkmodules.vtkFiltersGeneral import vtkShrinkFilter
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkFiltersModeling import vtkLinearExtrusionFilter
from vtkmodules.vtkFiltersSources import vtkConeSource, vtkCylinderSource, vtkSphereSource

class TestAlgorithmNumberProtocol(vtkTesting.vtkTest):
    def testCaseAlgorithmToAlgorithm(self):
        ef = vtkSphereSource(radius=10) >> vtkElevationFilter()
        self.assertIsInstance(ef, vtkElevationFilter)
        self.assertIsInstance(ef.GetInputConnection(0, 0).producer, vtkSphereSource)
        self.assertEqual(ef.GetInputConnection(0, 0).producer.radius, 10)

    def testCaseDataObjectToAlgorithm(self):
        ef = vtkImageData(dimensions=(3, 3, 3)) >> vtkElevationFilter()
        self.assertIsInstance(ef, vtkElevationFilter)
        self.assertIsInstance(ef.GetInputDataObject(0, 0), vtkImageData)
        self.assertTupleEqual(ef.GetInputDataObject(0, 0).dimensions, (3, 3, 3))

    def testCaseManyAlgorithms(self):
        pipeline = (
            vtkSphereSource(radius=10, theta_resolution=64, phi_resolution=64)
                >> vtkElevationFilter()
                >> vtkShrinkFilter()
                >> vtkGeometryFilter()
                >> vtkPolyDataConnectivityFilter(color_regions=True, extraction_mode=VTK_EXTRACT_ALL_REGIONS)
                >> vtkPolyDataNormals()
        )
        self.assertIsInstance(pipeline, vtkPolyDataNormals)
        self.assertIsInstance(pipeline.GetInputConnection(0, 0).producer, vtkPolyDataConnectivityFilter)
        self.assertEqual(pipeline.GetInputConnection(0, 0).producer.color_regions, True)

        output = pipeline.execute()
        self.assertIsInstance(output, vtkPolyData)
        self.assertEqual(output.number_of_points, 23808)
        self.assertEqual(output.number_of_cells, 7936)
        self.assertEqual(output.point_data.scalars.name, "RegionId")
        self.assertEqual(output.point_data.scalars.range, (0, 7935))

    def testCaseDataObjectManyAlgorithms(self):
        pipeline = (
            vtkImageData(dimensions=(10, 10, 1))
                >> vtkElevationFilter(low_point=(0, 0, 0), high_point=(10, 10, 0))
                >> vtkGeometryFilter()
                >> vtkPolyDataConnectivityFilter(color_regions=True)
                >> vtkPolyDataNormals()
        )
        self.assertIsInstance(pipeline, vtkPolyDataNormals)
        self.assertIsInstance(pipeline.GetInputConnection(0, 0).producer, vtkPolyDataConnectivityFilter)
        self.assertEqual(
            pipeline.GetInputConnection(0, 0).producer.color_regions, True)
        self.assertEqual(
            pipeline                            # this is vtkPolyDataNormals
                .GetInputConnection(0, 0).producer  # this is vtkPolyDataConnectivityFilter
                .GetInputConnection(0, 0).producer  # this is vtkGeometryFilter
                .GetInputConnection(0, 0).producer  # this is vtkElevationFilter
                .low_point, (0, 0, 0))
        self.assertEqual(
            pipeline                            # this is vtkPolyDataNormals
                .GetInputConnection(0, 0).producer  # this is vtkPolyDataConnectivityFilter
                .GetInputConnection(0, 0).producer  # this is vtkGeometryFilter
                .GetInputConnection(0, 0).producer  # this is vtkElevationFilter
                .high_point, (10, 10, 0))
        self.assertEqual(
            pipeline                            # this is vtkPolyDataNormals
                .GetInputConnection(0, 0).producer  # this is vtkPolyDataConnectivityFilter
                .GetInputConnection(0, 0).producer  # this is vtkGeometryFilter
                .GetInputConnection(0, 0).producer  # this is vtkElevationFilter
                .GetInputDataObject(0, 0)           # this is vtkImageData
                .dimensions, (10, 10, 1))

        output = pipeline.execute()
        self.assertIsInstance(output, vtkPolyData)
        self.assertEqual(output.number_of_points, 100)
        self.assertEqual(output.number_of_cells, 81)
        self.assertEqual(output.point_data.scalars.name, "RegionId")
        self.assertEqual(output.point_data.scalars.range, (0, 0))

    def testCaseManyAlgorithmsWithUserInput(self):
        # A pipeline object can be reused with different input data objects.
        pipeline = (
            vtkElevationFilter()
                >> vtkShrinkFilter()
                >> vtkGeometryFilter()
                >> vtkPolyDataConnectivityFilter(color_regions=True, extraction_mode=VTK_EXTRACT_ALL_REGIONS)
                >> vtkPolyDataNormals()
        )
        self.assertIsInstance(pipeline, vtkPolyDataNormals)
        self.assertIsInstance(pipeline.GetInputConnection(0, 0).producer, vtkPolyDataConnectivityFilter)
        self.assertEqual(pipeline.GetInputConnection(0, 0).producer.color_regions, True)

        cone = vtkConeSource(radius=5, resolution=8, height=2).execute()
        output = pipeline.execute(cone)
        self.assertIsInstance(output, vtkPolyData)
        self.assertEqual(output.number_of_points, 32)
        self.assertEqual(output.number_of_cells, 9)
        self.assertEqual(output.point_data.scalars.name, "RegionId")
        self.assertEqual(output.point_data.scalars.range, (0, 8))

        cylinder = vtkCylinderSource(radius=6, resolution=9, height=3).execute()
        output = pipeline.execute(cylinder)
        self.assertIsInstance(output, vtkPolyData)
        self.assertEqual(output.number_of_points, 54)
        self.assertEqual(output.number_of_cells, 11)
        self.assertEqual(output.point_data.scalars.name, "RegionId")
        self.assertEqual(output.point_data.scalars.range, (0, 10))


if __name__ == '__main__':
    vtkTesting.main([(TestAlgorithmNumberProtocol, 'test')])
