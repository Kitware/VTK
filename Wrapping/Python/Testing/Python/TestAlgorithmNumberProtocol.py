#!/usr/bin/env python
#
# This test exercises usage of rshift operator to build pipelines with vtkAlgorithm and vtkDataObject.

import vtkmodules.test.Testing as vtkTesting
from vtkmodules.vtkCommonDataModel import vtkImageData, vtkPolyData
from vtkmodules.vtkFiltersCore import vtkElevationFilter, vtkPolyDataConnectivityFilter, vtkPolyDataNormals, VTK_EXTRACT_ALL_REGIONS, vtkAppendFilter
from vtkmodules.vtkFiltersGeneral import vtkShrinkFilter
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkFiltersModeling import vtkLinearExtrusionFilter
from vtkmodules.vtkFiltersSources import vtkConeSource, vtkCylinderSource, vtkSphereSource
from vtkmodules.util.execution_model import Pipeline


class MockValidConnector(object):
    ExecutedSetInputConnection = 0
    ExecutedSetInputDataObject = 0

    def __init__(self, alg, input_port=0, output_port=0):
        self.algorithm = alg
        self.input_port = input_port
        self.output_port = output_port

    def SetInputConnection(self, other):
        MockValidConnector.ExecutedSetInputConnection += 1
        self.algorithm.SetInputConnection(self.input_port, other)

    def GetInputPortInformation(self, port):
        return self.algorithm.GetInputPortInformation(self.input_port)


class MockInvalidConnector(object):
    def __init__(self, alg, input_port=0, output_port=0):
        self.algorithm = alg
        pass


class TestAlgorithmNumberProtocol(vtkTesting.vtkTest):
    def testCaseAlgorithmToAlgorithm(self):
        ef = vtkSphereSource(radius=10) >> vtkElevationFilter()
        self.assertIsInstance(ef, Pipeline)
        self.assertIsInstance(ef.last.GetInputConnection(0, 0).producer, vtkSphereSource)
        self.assertEqual(ef.last.GetInputConnection(0, 0).producer.radius, 10)

    def testCaseDataObjectToAlgorithm(self):
        ef = vtkImageData(dimensions=(3, 3, 3)) >> vtkElevationFilter()
        self.assertIsInstance(ef, Pipeline)
        self.assertIsInstance(ef.last.GetInputDataObject(0, 0), vtkImageData)
        self.assertTupleEqual(ef.last.GetInputDataObject(0, 0).dimensions, (3, 3, 3))

    def testCaseManyAlgorithms(self):
        pipeline = (
            vtkSphereSource(radius=10, theta_resolution=64, phi_resolution=64)
                >> vtkElevationFilter()
                >> vtkShrinkFilter()
                >> vtkGeometryFilter()
                >> vtkPolyDataConnectivityFilter(color_regions=True, extraction_mode=VTK_EXTRACT_ALL_REGIONS)
                >> vtkPolyDataNormals()
        )
        self.assertIsInstance(pipeline, Pipeline)
        self.assertIsInstance(pipeline.last.GetInputConnection(0, 0).producer, vtkPolyDataConnectivityFilter)
        self.assertEqual(pipeline.last.GetInputConnection(0, 0).producer.color_regions, True)

        output = pipeline()
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
        self.assertIsInstance(pipeline.last, vtkPolyDataNormals)
        self.assertIsInstance(pipeline.last.GetInputConnection(0, 0).producer, vtkPolyDataConnectivityFilter)
        self.assertEqual(
            pipeline.last.GetInputConnection(0, 0).producer.color_regions, True)
        self.assertEqual(
            pipeline.last                            # this is vtkPolyDataNormals
                .GetInputConnection(0, 0).producer  # this is vtkPolyDataConnectivityFilter
                .GetInputConnection(0, 0).producer  # this is vtkGeometryFilter
                .GetInputConnection(0, 0).producer  # this is vtkElevationFilter
                .low_point, (0, 0, 0))
        self.assertEqual(
            pipeline.last                            # this is vtkPolyDataNormals
                .GetInputConnection(0, 0).producer  # this is vtkPolyDataConnectivityFilter
                .GetInputConnection(0, 0).producer  # this is vtkGeometryFilter
                .GetInputConnection(0, 0).producer  # this is vtkElevationFilter
                .high_point, (10, 10, 0))
        self.assertEqual(
            pipeline.last                            # this is vtkPolyDataNormals
                .GetInputConnection(0, 0).producer  # this is vtkPolyDataConnectivityFilter
                .GetInputConnection(0, 0).producer  # this is vtkGeometryFilter
                .GetInputConnection(0, 0).producer  # this is vtkElevationFilter
                .GetInputDataObject(0, 0)           # this is vtkImageData
                .dimensions, (10, 10, 1))

        output = pipeline()
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
        self.assertIsInstance(pipeline, Pipeline)
        self.assertIsInstance(pipeline.last.GetInputConnection(0, 0).producer, vtkPolyDataConnectivityFilter)
        self.assertEqual(pipeline.last.GetInputConnection(0, 0).producer.color_regions, True)

        cone = vtkConeSource(radius=5, resolution=8, height=2)()
        output = pipeline(cone)
        self.assertIsInstance(output, vtkPolyData)
        self.assertEqual(output.number_of_points, 32)
        self.assertEqual(output.number_of_cells, 9)
        self.assertEqual(output.point_data.scalars.name, "RegionId")
        self.assertEqual(output.point_data.scalars.range, (0, 8))

        cylinder = vtkCylinderSource(radius=6, resolution=9, height=3)()
        output = pipeline(cylinder)
        self.assertIsInstance(output, vtkPolyData)
        self.assertEqual(output.number_of_points, 54)
        self.assertEqual(output.number_of_cells, 11)
        self.assertEqual(output.point_data.scalars.name, "RegionId")
        self.assertEqual(output.point_data.scalars.range, (0, 10))

    def testCaseAcceptValidRHS(self):
        # Initially, both counters are 0.
        self.assertEqual(MockValidConnector.ExecutedSetInputConnection, 0)
        self.assertEqual(MockValidConnector.ExecutedSetInputDataObject, 0)

        ef = vtkSphereSource() >> MockValidConnector(vtkElevationFilter())
        self.assertEqual(MockValidConnector.ExecutedSetInputConnection, 1, "VTK wrapping did not call MockValidConnector.SetInputConnection")

        ef = vtkImageData(dimensions=(10, 10, 1)) >> MockValidConnector(vtkElevationFilter())
        self.assertEqual(MockValidConnector.ExecutedSetInputConnection, 2, "VTK wrapping did not call MockValidConnector.SetInputDataObject")

    def testCaseRejectInvalidRHS(self):
        # Tests that you can't place just about anything on the right hand side of a vtkAlgorithm.__rshift__ operator.
        # The RHS must have a function called SetInputConnection(self, other: vtkAlgorithmOutput).
        with self.assertRaisesRegex(TypeError, "unsupported operand type\(s\) for >>: vtkSphereSource and str"):
            pipeline = vtkSphereSource() >> "helloThere"

        with self.assertRaisesRegex(TypeError, "unsupported operand type\(s\) for >>: vtkSphereSource and int"):
            pipeline = vtkSphereSource() >> 2

        with self.assertRaisesRegex(TypeError, "unsupported operand type\(s\) for >>: str and vtkElevationFilter"):
            pipeline = "ImageData" >> vtkElevationFilter()

        with self.assertRaisesRegex(TypeError, "unsupported operand type\(s\) for >>: int and vtkElevationFilter"):
            pipeline = 2 >> vtkElevationFilter()

        with self.assertRaisesRegex(TypeError, "unsupported operand type\(s\) for >>: vtkSphereSource and MockInvalidConnector"):
            pipeline = vtkSphereSource() >> MockInvalidConnector(vtkElevationFilter())

        # Tests that you can't place just about anything on the right hand side of a vtkDataObject.__rshift__ operator.
        # The RHS must have a function called SetInputDataObject(self, other: vtkDataObject).
        with self.assertRaisesRegex(TypeError, "unsupported operand type\(s\) for >>: vtkImageData and str"):
            pipeline = vtkImageData(dimensions=(10, 10, 1)) >> "helloThere"

        with self.assertRaisesRegex(TypeError, "unsupported operand type\(s\) for >>: vtkImageData and int"):
            pipeline = vtkImageData(dimensions=(10, 10, 1)) >> 2

        with self.assertRaisesRegex(TypeError, "unsupported operand type\(s\) for >>: vtkImageData and MockInvalidConnector"):
            pipeline = vtkImageData(dimensions=(10, 10, 1)) >> MockInvalidConnector(vtkElevationFilter())
    def testClearInput(self):
        s = vtkShrinkFilter()
        vtkSphereSource() >> s
        self.assertTrue(s.GetInputAlgorithm(0, 0).IsA("vtkSphereSource"))
        None >> s
        self.assertEqual(s.GetInputAlgorithm(0,0), None)
        a = vtkAppendFilter()
        (vtkConeSource(), vtkSphereSource()) >> a
        self.assertEqual(a.GetNumberOfInputConnections(0), 2)
        [] >> a
        self.assertEqual(a.GetNumberOfInputConnections(0), 0)

if __name__ == '__main__':
    vtkTesting.main([(TestAlgorithmNumberProtocol, 'test')])
