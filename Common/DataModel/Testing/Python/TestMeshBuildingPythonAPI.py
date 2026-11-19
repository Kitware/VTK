"""Tests for vtkCellArray and vtkPoints Pythonic constructors."""

from vtkmodules.vtkCommonCore import VTK_DOUBLE, VTK_FLOAT, vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
    vtkStructuredGrid,
    vtkUnstructuredGrid,
)
from vtkmodules.util.numpy_support import numpy_to_vtk
import numpy as np
from vtkmodules.test import Testing


class TestCellArrayConstructor(Testing.vtkTest):
    def test_empty_constructor(self):
        ca = vtkCellArray()
        self.assertEqual(ca.GetNumberOfCells(), 0)

    def test_constructor_from_numpy(self):
        offsets = np.array([0, 3, 6], dtype=np.int64)
        connectivity = np.array([0, 1, 2, 3, 4, 5], dtype=np.int64)
        ca = vtkCellArray(offsets=offsets, connectivity=connectivity)
        self.assertEqual(ca.GetNumberOfCells(), 2)
        np.testing.assert_array_equal(
            np.asarray(ca.GetOffsetsArray()), offsets
        )
        np.testing.assert_array_equal(
            np.asarray(ca.GetConnectivityArray()), connectivity
        )

    def test_constructor_from_lists(self):
        ca = vtkCellArray(offsets=[0, 3, 6], connectivity=[0, 1, 2, 3, 4, 5])
        self.assertEqual(ca.GetNumberOfCells(), 2)

    def test_constructor_from_vtk_arrays(self):
        offsets = numpy_to_vtk(np.array([0, 4], dtype=np.int64))
        connectivity = numpy_to_vtk(np.array([0, 1, 2, 3], dtype=np.int64))
        ca = vtkCellArray(offsets=offsets, connectivity=connectivity)
        self.assertEqual(ca.GetNumberOfCells(), 1)

    def test_partial_kwargs_raises(self):
        with self.assertRaises(ValueError):
            vtkCellArray(offsets=[0, 3])
        with self.assertRaises(ValueError):
            vtkCellArray(connectivity=[0, 1, 2])

    def test_offsets_connectivity_properties(self):
        offsets = np.array([0, 3, 5], dtype=np.int64)
        connectivity = np.array([0, 1, 2, 3, 4], dtype=np.int64)
        ca = vtkCellArray(offsets=offsets, connectivity=connectivity)
        # offsets_array and connectivity_array are auto-generated properties
        self.assertIsNotNone(ca.offsets_array)
        self.assertIsNotNone(ca.connectivity_array)

    def test_repr(self):
        ca = vtkCellArray()
        self.assertEqual(repr(ca), "vtkCellArray(0 cells)")
        offsets = np.array([0, 3, 6, 10], dtype=np.int64)
        connectivity = np.arange(10, dtype=np.int64)
        ca = vtkCellArray(offsets=offsets, connectivity=connectivity)
        self.assertEqual(repr(ca), "vtkCellArray(3 cells)")


class TestPointsConstructor(Testing.vtkTest):
    def test_empty_constructor(self):
        pts = vtkPoints()
        self.assertEqual(pts.GetNumberOfPoints(), 0)

    def test_constructor_from_numpy(self):
        data = np.array([[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]])
        pts = vtkPoints(data=data)
        self.assertEqual(pts.GetNumberOfPoints(), 3)
        np.testing.assert_array_almost_equal(pts.GetPoint(0), [0, 0, 0])
        np.testing.assert_array_almost_equal(pts.GetPoint(1), [1, 0, 0])
        np.testing.assert_array_almost_equal(pts.GetPoint(2), [0, 1, 0])

    def test_constructor_from_list_of_lists(self):
        pts = vtkPoints(data=[[0, 0, 0], [1, 0, 0]])
        self.assertEqual(pts.GetNumberOfPoints(), 2)
        np.testing.assert_array_almost_equal(pts.GetPoint(0), [0, 0, 0])
        np.testing.assert_array_almost_equal(pts.GetPoint(1), [1, 0, 0])

    def test_constructor_from_vtk_array(self):
        data = np.array([[0.0, 0.0, 0.0], [1.0, 2.0, 3.0]])
        vtk_arr = numpy_to_vtk(data)
        pts = vtkPoints(data=vtk_arr)
        self.assertEqual(pts.GetNumberOfPoints(), 2)
        np.testing.assert_array_almost_equal(pts.GetPoint(1), [1, 2, 3])

    def test_repr(self):
        pts = vtkPoints()
        self.assertEqual(repr(pts), "vtkPoints(0 points)")
        data = np.array([[0, 0, 0], [1, 0, 0], [0, 1, 0]], dtype=np.float64)
        pts = vtkPoints(data=data)
        self.assertEqual(repr(pts), "vtkPoints(3 points)")

    def test_getitem(self):
        data = np.array(
            [[1.0, 2.0, 3.0], [4.0, 5.0, 6.0], [7.0, 8.0, 9.0]]
        )
        pts = vtkPoints(data=data)
        self.assertEqual(pts.GetNumberOfPoints(), 3)
        np.testing.assert_array_almost_equal(pts.GetPoint(0), [1, 2, 3])
        np.testing.assert_array_almost_equal(pts.GetPoint(2), [7, 8, 9])

    def test_float_types_kept_without_copy(self):
        for dtype, vtk_type in ((np.float32, VTK_FLOAT),
                                (np.float64, VTK_DOUBLE)):
            data = np.arange(12, dtype=dtype).reshape(4, 3)
            pts = vtkPoints(data=data)
            self.assertEqual(pts.GetDataType(), vtk_type)
            self.assertTrue(np.shares_memory(pts.data, data))
            data[0, 0] = 42
            self.assertEqual(pts.GetPoint(0)[0], 42)

    def test_other_types_become_double(self):
        for data in (np.arange(6, dtype=np.int64).reshape(2, 3),
                     np.arange(6, dtype=np.int32).reshape(2, 3),
                     [[0, 0, 0], [1, 0, 0]],
                     [[0.5, 0, 0], [1, 0, 0]]):
            pts = vtkPoints(data=data)
            self.assertEqual(pts.GetDataType(), VTK_DOUBLE)
            np.testing.assert_array_equal(pts.data, np.asarray(data))

    def test_non_contiguous_float32_stays_float32(self):
        data = np.arange(24, dtype=np.float32).reshape(8, 3)[::2]
        pts = vtkPoints(data=data)
        self.assertEqual(pts.GetDataType(), VTK_FLOAT)
        np.testing.assert_array_equal(pts.data, data)

    def test_explicit_data_type(self):
        data = np.arange(6, dtype=np.float64).reshape(2, 3)
        pts = vtkPoints(data_type=VTK_FLOAT, data=data)
        self.assertEqual(pts.GetDataType(), VTK_FLOAT)
        np.testing.assert_array_equal(pts.data, data)
        pts = vtkPoints(data_type=VTK_DOUBLE, data=data.astype(np.float32))
        self.assertEqual(pts.GetDataType(), VTK_DOUBLE)


class TestDataSetPointsSetter(Testing.vtkTest):
    def test_assign_numpy(self):
        for dtype, vtk_type in ((np.float32, VTK_FLOAT),
                                (np.float64, VTK_DOUBLE)):
            data = np.arange(12, dtype=dtype).reshape(4, 3)
            pd = vtkPolyData()
            pd.points = data
            self.assertEqual(pd.GetNumberOfPoints(), 4)
            self.assertEqual(pd.GetPoints().GetDataType(), vtk_type)
            self.assertTrue(np.shares_memory(pd.points.data, data))

    def test_assign_list(self):
        ug = vtkUnstructuredGrid()
        ug.points = [[0, 0, 0], [1, 0, 0], [0, 1, 0]]
        self.assertEqual(ug.GetNumberOfPoints(), 3)
        self.assertEqual(ug.GetPoints().GetDataType(), VTK_DOUBLE)

    def test_assign_vtk_array(self):
        pd = vtkPolyData()
        pd.points = numpy_to_vtk(np.zeros((2, 3), dtype=np.float32))
        self.assertEqual(pd.GetNumberOfPoints(), 2)
        self.assertEqual(pd.GetPoints().GetDataType(), VTK_FLOAT)

    def test_assign_vtk_points_and_none(self):
        pts = vtkPoints(data=np.zeros((5, 3)))
        sg = vtkStructuredGrid(dimensions=(5, 1, 1))
        sg.points = pts
        self.assertIs(sg.GetPoints(), pts)
        sg.points = None
        self.assertIsNone(sg.GetPoints())

    def test_constructor_keyword(self):
        data = np.random.rand(3, 3).astype(np.float32)
        pd = vtkPolyData(points=data)
        self.assertEqual(pd.GetNumberOfPoints(), 3)
        self.assertTrue(np.shares_memory(pd.points.data, data))


if __name__ == "__main__":
    Testing.main([(TestCellArrayConstructor, "test"),
                   (TestPointsConstructor, "test"),
                   (TestDataSetPointsSetter, "test")])
