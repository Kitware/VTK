#!/usr/bin/env python
"""Round-trip a vtkPolyData carrying all four cell-array kinds
(verts/lines/polys/strips) through fides and confirm bit-exact recovery
of points and per-role connectivity."""

import os.path
from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray, vtkPartitionedDataSetCollection, vtkPolyData)
from vtkmodules.vtkIOFides import vtkFidesReader, vtkFidesWriter
from vtkmodules.util import numpy_support
from vtkmodules.util.misc import vtkGetTempDir

import numpy as np
VTK_TEMP_DIR = vtkGetTempDir()

from vtk.test import Testing


def _build_mixed_polydata():
    """An 11-point 2D layout with verts, lines (incl. polyline), polys
    (triangle + quad + 5-vertex polygon), and a triangle strip."""
    coords = [
        (0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (2.0, 0.0, 0.0), (3.0, 0.0, 0.0),
        (0.0, 1.0, 0.0), (1.0, 1.0, 0.0), (2.0, 1.0, 0.0), (3.0, 1.0, 0.0),
        (0.0, 2.0, 0.0), (1.0, 2.0, 0.0), (2.0, 2.0, 0.0),
    ]
    pts = vtkPoints()
    for c in coords:
        pts.InsertNextPoint(*c)

    pd = vtkPolyData()
    pd.SetPoints(pts)

    verts = vtkCellArray()
    for i in (0, 3):
        verts.InsertNextCell(1); verts.InsertCellPoint(i)
    pd.SetVerts(verts)

    lines = vtkCellArray()
    lines.InsertNextCell(2); lines.InsertCellPoint(0); lines.InsertCellPoint(1)
    lines.InsertNextCell(3); lines.InsertCellPoint(1); lines.InsertCellPoint(2); lines.InsertCellPoint(3)
    pd.SetLines(lines)

    polys = vtkCellArray()
    polys.InsertNextCell(3)
    for i in (0, 1, 4):
        polys.InsertCellPoint(i)
    polys.InsertNextCell(4)
    for i in (1, 2, 6, 5):
        polys.InsertCellPoint(i)
    polys.InsertNextCell(5)
    for i in (4, 5, 9, 8, 0):
        polys.InsertCellPoint(i)
    pd.SetPolys(polys)

    strips = vtkCellArray()
    strips.InsertNextCell(5)
    for i in (4, 5, 8, 9, 10):
        strips.InsertCellPoint(i)
    pd.SetStrips(strips)
    return pd


def _build_polys_only_polydata():
    """Two triangles only — exercises the case where only one of the four
    roles has cells, so the absent roles get no on-disk variables."""
    pts = vtkPoints()
    for c in [(0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (1.0, 1.0, 0.0), (0.0, 1.0, 0.0)]:
        pts.InsertNextPoint(*c)
    pd = vtkPolyData(); pd.SetPoints(pts)
    polys = vtkCellArray()
    polys.InsertNextCell(3); polys.InsertCellPoint(0); polys.InsertCellPoint(1); polys.InsertCellPoint(2)
    polys.InsertNextCell(3); polys.InsertCellPoint(0); polys.InsertCellPoint(2); polys.InsertCellPoint(3)
    pd.SetPolys(polys)
    return pd


def _role_cells(role_accessor):
    """Return a list of per-cell vertex lists for one of vtkPolyData's
    cell arrays, or [] if the array is empty/missing."""
    ca = role_accessor()
    if not ca or ca.GetNumberOfCells() == 0:
        return []
    offs = numpy_support.vtk_to_numpy(ca.GetOffsetsArray())
    conn = numpy_support.vtk_to_numpy(ca.GetConnectivityArray())
    return [conn[offs[i]:offs[i + 1]].tolist() for i in range(ca.GetNumberOfCells())]


def _polydata_signature(pd):
    return (
        numpy_support.vtk_to_numpy(pd.GetPoints().GetData()).tolist(),
        _role_cells(pd.GetVerts),
        _role_cells(pd.GetLines),
        _role_cells(pd.GetPolys),
        _role_cells(pd.GetStrips),
    )


class TestFidesPolyData(Testing.vtkTest):
    def _round_trip(self, builder, fname):
        original = builder()
        out_bp = os.path.join(VTK_TEMP_DIR, fname)
        writer = vtkFidesWriter()
        writer.SetFileName(out_bp)
        writer.SetInputData(original)
        writer.Write()

        reader = vtkFidesReader()
        reader.SetFileName(out_bp)
        reader.Update()
        pdsc = reader.GetOutputDataObject(0)
        self.assertIsInstance(pdsc, vtkPartitionedDataSetCollection)
        rt = pdsc.GetPartitionedDataSet(0).GetPartitionAsDataObject(0)
        self.assertIsInstance(rt, vtkPolyData)
        self.assertEqual(_polydata_signature(rt), _polydata_signature(original))

    def testAllFourCellArrayKinds(self):
        self._round_trip(_build_mixed_polydata,
                         "TestFidesPolyData_mixed.bp")

    def testPolysOnly(self):
        self._round_trip(_build_polys_only_polydata,
                         "TestFidesPolyData_polys.bp")


if __name__ == "__main__":
    Testing.main([(TestFidesPolyData, 'test')])
