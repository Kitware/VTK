"""Tests for the side_data property on vtkUnstructuredGrid.

side_data is the peer of point_data and cell_data for attributes carried on
the sides between cells. What is different about it is that it does not exist
until it is asked for, so most of these check that asking the wrong way round
does not quietly build an O(n) side numbering.
"""

import unittest

import vtkmodules.test.Testing as vtkTesting

try:
    import numpy as np
except ImportError:
    print("This test requires numpy!")
    vtkTesting.skip()

from vtkmodules.vtkCommonCore import vtkDoubleArray, vtkIdList, vtkPoints
from vtkmodules.vtkCommonDataModel import (
    VTK_HEXAHEDRON,
    VTK_POLYGON,
    vtkDataObject,
    vtkUnstructuredGrid,
)


def make_hex_grid(nx, ny, nz):
    points = vtkPoints()
    for k in range(nz + 1):
        for j in range(ny + 1):
            for i in range(nx + 1):
                points.InsertNextPoint(i, j, k)

    def pid(i, j, k):
        return i + j * (nx + 1) + k * (nx + 1) * (ny + 1)

    grid = vtkUnstructuredGrid()
    grid.SetPoints(points)
    grid.AllocateExact(nx * ny * nz, 8 * nx * ny * nz)
    for k in range(nz):
        for j in range(ny):
            for i in range(nx):
                ids = vtkIdList()
                for p in (
                    pid(i, j, k), pid(i + 1, j, k), pid(i + 1, j + 1, k), pid(i, j + 1, k),
                    pid(i, j, k + 1), pid(i + 1, j, k + 1), pid(i + 1, j + 1, k + 1),
                    pid(i, j + 1, k + 1),
                ):
                    ids.InsertNextId(p)
                grid.InsertNextCell(VTK_HEXAHEDRON, ids)
    return grid


def make_quad_sheet(nx, ny):
    """Quads as polygons, every cell wound the same way."""
    points = vtkPoints()
    for j in range(ny + 1):
        for i in range(nx + 1):
            points.InsertNextPoint(i, j, 0)

    grid = vtkUnstructuredGrid()
    grid.SetPoints(points)
    grid.AllocateExact(nx * ny, 4 * nx * ny)
    for j in range(ny):
        for i in range(nx):
            ids = vtkIdList()
            for p in (
                i + j * (nx + 1), i + 1 + j * (nx + 1),
                i + 1 + (j + 1) * (nx + 1), i + (j + 1) * (nx + 1),
            ):
                ids.InsertNextId(p)
            grid.InsertNextCell(VTK_POLYGON, ids)
    return grid


class TestSideDataLaziness(unittest.TestCase):
    """Nothing should build a side numbering behind the caller's back."""

    def setUp(self):
        self.grid = make_hex_grid(2, 2, 2)

    def test_absent_until_asked(self):
        self.assertFalse(self.grid.HasSideMap())
        self.assertFalse(self.grid.has_side_data)
        # Asking again must not have changed the answer.
        self.assertFalse(self.grid.HasSideMap())

    def test_attribute_queries_do_not_build(self):
        side = vtkDataObject.SIDE
        self.assertIs(self.grid.GetAttributesAsFieldData(side), self.grid.GetSideData())
        self.assertEqual(self.grid.GetNumberOfElements(side), 0)
        self.assertFalse(self.grid.HasSideMap())

    def test_reading_side_data_does_not_build(self):
        self.assertEqual(self.grid.side_data.keys(), ())
        self.assertFalse(self.grid.HasSideMap())

    def test_assigning_side_data_builds(self):
        self.grid.side_data["flux"] = 1.0
        self.assertTrue(self.grid.HasSideMap())
        # A 2x2x2 block: 3 * (3*2*2) sides.
        self.assertEqual(self.grid.GetNumberOfSides(), 36)

    def test_equality_does_not_build(self):
        other = make_hex_grid(2, 2, 2)
        self.assertEqual(self.grid, other)
        self.assertFalse(self.grid.HasSideMap())
        self.assertFalse(other.HasSideMap())


class TestSideDataArrays(unittest.TestCase):
    """side_data behaves like point_data and cell_data."""

    def setUp(self):
        self.grid = make_hex_grid(2, 2, 2)
        self.nsides = self.grid.GetNumberOfSides()

    def test_scalar_broadcasts_over_sides(self):
        self.grid.side_data["flux"] = 2.5
        flux = np.asarray(self.grid.side_data["flux"])
        self.assertEqual(flux.shape, (self.nsides,))
        self.assertTrue(np.all(flux == 2.5))

    def test_numpy_array(self):
        values = np.arange(self.nsides, dtype=float)
        self.grid.side_data["v"] = values
        np.testing.assert_allclose(np.asarray(self.grid.side_data["v"]), values)

    def test_vtk_array(self):
        arr = vtkDoubleArray()
        arr.SetName("ignored")
        arr.SetNumberOfTuples(self.nsides)
        arr.FillValue(7.0)
        self.grid.side_data["w"] = arr
        self.assertIn("w", self.grid.side_data.keys())
        self.assertTrue(np.all(np.asarray(self.grid.side_data["w"]) == 7.0))

    def test_setter_replaces(self):
        self.grid.side_data["a"] = 1.0
        self.grid.side_data = {"b": 2.0}
        self.assertEqual(self.grid.side_data.keys(), ("b",))

    def test_has_side_data_tracks_arrays(self):
        self.assertFalse(self.grid.has_side_data)
        self.grid.side_data["flux"] = 1.0
        self.assertTrue(self.grid.has_side_data)

    def test_point_and_cell_data_unaffected(self):
        self.grid.side_data["flux"] = 1.0
        self.grid.cell_data["rho"] = 2.0
        self.grid.point_data["h"] = 3.0
        self.assertEqual(
            len(np.asarray(self.grid.cell_data["rho"])), self.grid.GetNumberOfCells())
        self.assertEqual(
            len(np.asarray(self.grid.point_data["h"])), self.grid.GetNumberOfPoints())
        self.assertEqual(len(np.asarray(self.grid.side_data["flux"])), self.nsides)

    def test_attribute_type_reported(self):
        self.grid.side_data["flux"] = 1.0
        arr = self.grid.side_data["flux"]
        self.assertEqual(
            self.grid.GetAttributeTypeForArray(arr),
            vtkDataObject.SIDE)


class TestSideDataEquality(unittest.TestCase):
    """Side data takes part in comparison once it exists."""

    def test_same_side_data_compares_equal(self):
        a, b = make_hex_grid(2, 2, 2), make_hex_grid(2, 2, 2)
        a.side_data["flux"] = 1.0
        b.side_data["flux"] = 1.0
        self.assertEqual(a, b)

    def test_different_side_data_compares_unequal(self):
        a, b = make_hex_grid(2, 2, 2), make_hex_grid(2, 2, 2)
        a.side_data["flux"] = 1.0
        b.side_data["flux"] = 2.0
        self.assertNotEqual(a, b)

    def test_one_sided_side_data_compares_unequal(self):
        a, b = make_hex_grid(2, 2, 2), make_hex_grid(2, 2, 2)
        a.side_data["flux"] = 1.0
        self.assertNotEqual(a, b)


class TestSideDataOnSurface(unittest.TestCase):
    """A 2D cell's sides are its edges, so a surface mesh carries edge data."""

    def setUp(self):
        self.grid = make_quad_sheet(3, 3)

    def test_edge_count(self):
        # A 3x3 quad sheet: 4*3 vertical edges plus 3*4 horizontal.
        self.assertEqual(self.grid.GetNumberOfSides(), 24)

    def test_edge_data_broadcasts(self):
        self.grid.side_data["u"] = 1.0
        self.assertEqual(len(np.asarray(self.grid.side_data["u"])), 24)

    def test_consistently_wound(self):
        self.assertEqual(self.grid.GetSideMap().CheckWindingConsistency(), 0)

    def test_outward_signs_cancel(self):
        """The identity a finite-volume loop rests on, through the Python API."""
        nsides = self.grid.GetNumberOfSides()
        rng = np.random.default_rng(0)
        values = rng.normal(size=nsides)
        self.grid.side_data["normalVelocity"] = values

        sides = self.grid.GetSideMap()
        ids = vtkIdList()

        over_cells = 0.0
        for cell_id in range(self.grid.GetNumberOfCells()):
            sides.GetCellSides(cell_id, ids)
            for local in range(ids.GetNumberOfIds()):
                over_cells += sides.GetSideOrientation(cell_id, local) * values[ids.GetId(local)]

        over_boundary = 0.0
        for side_id in range(nsides):
            sides.GetSideCells(side_id, ids)
            if ids.GetNumberOfIds() == 1:
                over_boundary += values[side_id]

        self.assertAlmostEqual(over_cells, over_boundary, places=9)


if __name__ == "__main__":
    vtkTesting.main([(TestSideDataLaziness, "test"),
                     (TestSideDataArrays, "test"),
                     (TestSideDataEquality, "test"),
                     (TestSideDataOnSurface, "test")])
