#!/usr/bin/env python
"""Side data and a supplied side numbering survive writing and reading a .vtu,
in every encoding the XML writer has."""

import unittest

from vtkmodules.vtkCommonCore import vtkDoubleArray, vtkIdTypeArray, vtkPoints
from vtkmodules.vtkCommonDataModel import (
    VTK_HEXAHEDRON,
    vtkDataObject,
    vtkDataSetAttributes,
    vtkUnstructuredGrid,
)
from vtkmodules.vtkIOXML import (
    vtkXMLPUnstructuredGridReader,
    vtkXMLUnstructuredGridReader,
    vtkXMLUnstructuredGridWriter,
)
from vtkmodules.vtkIOParallelXML import vtkXMLPUnstructuredGridWriter
from vtkmodules.util.misc import vtkGetTempDir
from vtkmodules.test import Testing as vtkTesting
import os


def make_hex_grid(nx, ny, nz):
    points = vtkPoints()
    for k in range(nz + 1):
        for j in range(ny + 1):
            for i in range(nx + 1):
                points.InsertNextPoint(i, j, k)

    def pid(i, j, k):
        return i + (nx + 1) * (j + (ny + 1) * k)

    grid = vtkUnstructuredGrid()
    grid.SetPoints(points)
    for k in range(nz):
        for j in range(ny):
            for i in range(nx):
                grid.InsertNextCell(VTK_HEXAHEDRON, 8, [
                    pid(i, j, k), pid(i + 1, j, k), pid(i + 1, j + 1, k), pid(i, j + 1, k),
                    pid(i, j, k + 1), pid(i + 1, j, k + 1), pid(i + 1, j + 1, k + 1),
                    pid(i, j + 1, k + 1)])
    return grid


def add_side_arrays(grid):
    n = grid.GetNumberOfSides()
    flux = vtkDoubleArray()
    flux.SetName("flux")
    flux.SetNumberOfTuples(n)
    velocity = vtkDoubleArray()
    velocity.SetName("velocity")
    velocity.SetNumberOfComponents(3)
    velocity.SetNumberOfTuples(n)
    for s in range(n):
        flux.SetValue(s, 0.25 * s - 3.0)
        velocity.SetTuple3(s, s, -s, 2.0 * s)
    grid.GetSideData().AddArray(flux)
    grid.GetSideData().AddArray(velocity)
    grid.GetSideData().SetActiveScalars("flux")


def round_trip(grid, mode, encode):
    writer = vtkXMLUnstructuredGridWriter()
    writer.SetInputData(grid)
    writer.SetWriteToOutputString(True)
    writer.SetDataMode(mode)
    writer.SetEncodeAppendedData(encode)
    writer.Write()
    text = writer.GetOutputString()
    reader = vtkXMLUnstructuredGridReader()
    reader.ReadFromInputStringOn()
    reader.SetInputString(text)
    reader.Update()
    return (text.encode() if isinstance(text, str) else text), reader.GetOutput()


MODES = [(vtkXMLUnstructuredGridWriter.Ascii, False),
         (vtkXMLUnstructuredGridWriter.Binary, False),
         (vtkXMLUnstructuredGridWriter.Appended, False),
         (vtkXMLUnstructuredGridWriter.Appended, True)]


class TestXMLUnstructuredGridSideData(unittest.TestCase):

    def check(self, original, read):
        self.assertTrue(read.HasSideMap(), "the side map was not read")
        a, b = original.GetSideMap(), read.GetSideMap()
        self.assertEqual(a.GetAuthored(), b.GetAuthored())
        self.assertEqual(a.GetNumberOfSides(), b.GetNumberOfSides())
        for cell in range(original.GetNumberOfCells()):
            for local in range(a.GetNumberOfCellSides(cell)):
                self.assertEqual(a.GetCellSide(cell, local), b.GetCellSide(cell, local))
        for name in ("flux", "velocity"):
            x = original.GetSideData().GetArray(name)
            y = read.GetSideData().GetArray(name)
            self.assertIsNotNone(y, name + " was not read")
            self.assertEqual(x.GetNumberOfComponents(), y.GetNumberOfComponents())
            for s in range(x.GetNumberOfTuples()):
                self.assertEqual(x.GetTuple(s), y.GetTuple(s))
        self.assertEqual(read.GetSideData().GetScalars().GetName(), "flux")
        self.assertEqual(read.GetAttributes(vtkDataObject.SIDE).GetArray("flux"),
                         read.GetSideData().GetArray("flux"))

    def test_derived_numbering(self):
        grid = make_hex_grid(3, 2, 2)
        add_side_arrays(grid)
        for mode, encode in MODES:
            text, read = round_trip(grid, mode, encode)
            self.assertIn(b"<SideData", text)
            self.assertNotIn(b"<SideIds", text)
            self.check(grid, read)

    def test_supplied_numbering(self):
        grid = make_hex_grid(3, 2, 2)
        sides = grid.GetSideMap()
        n = sides.GetNumberOfSides()
        derived = sides.GetHalfSideToSide()
        ids = vtkIdTypeArray()
        ids.SetNumberOfTuples(derived.GetNumberOfTuples())
        for h in range(ids.GetNumberOfTuples()):
            ids.SetValue(h, n - 1 - int(derived.GetTuple1(h)))
        sides.SetSideIds(ids, n)
        add_side_arrays(grid)
        for mode, encode in MODES:
            text, read = round_trip(grid, mode, encode)
            self.assertIn(b"<SideIds", text)
            self.check(grid, read)

    def test_side_array_selection(self):
        grid = make_hex_grid(3, 2, 2)
        add_side_arrays(grid)
        writer = vtkXMLUnstructuredGridWriter()
        writer.SetInputData(grid)
        writer.SetWriteToOutputString(True)
        writer.Write()
        reader = vtkXMLUnstructuredGridReader()
        reader.ReadFromInputStringOn()
        reader.SetInputString(writer.GetOutputString())
        reader.UpdateInformation()
        self.assertEqual(reader.GetNumberOfSideArrays(), 2)
        self.assertEqual({reader.GetSideArrayName(i) for i in range(2)}, {"flux", "velocity"})
        reader.SetSideArrayStatus("velocity", 0)
        reader.Update()
        read = reader.GetOutput()
        self.assertIsNotNone(read.GetSideData().GetArray("flux"))
        self.assertIsNone(read.GetSideData().GetArray("velocity"))

    def test_parallel_file(self):
        grid = make_hex_grid(3, 2, 2)
        sides = grid.GetSideMap()
        n = sides.GetNumberOfSides()
        derived = sides.GetHalfSideToSide()
        ids = vtkIdTypeArray()
        ids.SetNumberOfTuples(derived.GetNumberOfTuples())
        for h in range(ids.GetNumberOfTuples()):
            ids.SetValue(h, n - 1 - int(derived.GetTuple1(h)))
        sides.SetSideIds(ids, n)
        add_side_arrays(grid)
        name = os.path.join(vtkGetTempDir(), "TestXMLUnstructuredGridSideData.pvtu")
        writer = vtkXMLPUnstructuredGridWriter()
        writer.SetInputData(grid)
        writer.SetFileName(name)
        writer.SetNumberOfPieces(1)
        writer.Write()
        reader = vtkXMLPUnstructuredGridReader()
        reader.SetFileName(name)
        reader.Update()
        self.check(grid, reader.GetOutput())

    def test_no_sides(self):
        grid = make_hex_grid(2, 1, 1)
        text, read = round_trip(grid, vtkXMLUnstructuredGridWriter.Ascii, False)
        self.assertNotIn(b"<SideData", text)
        self.assertFalse(read.HasSideMap(), "reading a file without sides built a side map")


if __name__ == "__main__":
    vtkTesting.main([(TestXMLUnstructuredGridSideData, "test")])
