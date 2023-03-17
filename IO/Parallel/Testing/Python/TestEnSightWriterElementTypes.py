#!/usr/bin/env python

"""
vtkEnSightWriter test for element types

This test reads an EnSight Gold case with one part which has a single cell
for each of the non-ghost cell types that EnSight Gold supports, using
vtkEnSightGoldBinaryReader.

The resulting vtkUnstructuredGrid is then written back to EnSight Gold using
vtkEnSightWriter, and read again via vtkEnSightGoldBinaryReader.
We assert that going through vtkEnSightWriter does not lose any geometry data.

"""

import vtk
from vtk.util.misc import vtkGetDataRoot, vtkGetTempDir
import os.path
VTK_DATA_ROOT = vtkGetDataRoot()
VTK_TEMP_DIR = vtkGetTempDir()


def get_cell_by_cell_type(mesh, cell_type):
    """Return first vtkCell of given type"""
    for i in range(mesh.GetNumberOfCells()):
        if mesh.GetCellType(i) == cell_type:
            return mesh.GetCell(i)
    assert False


def get_edge_set(cell):
    """Return set of point ID tuples of cell edges"""
    edges = set()
    for i in range(cell.GetNumberOfEdges()):
        e = cell.GetEdge(i)
        point_ids = []
        for j in range(e.GetNumberOfPoints()):
            point_ids.append(e.GetPointId(j))
        edges.add(tuple(sorted(point_ids)))
    return edges


def assert_cells_equal(c1, c2):
    """Assert that two vtkCells are equal"""
    assert c1.GetCellType() == c2.GetCellType()
    assert c1.GetNumberOfPoints() == c2.GetNumberOfPoints()
    assert get_edge_set(c1) == get_edge_set(c2)


CELL_TYPES_TO_TEST = [
    vtk.VTK_VERTEX,
    vtk.VTK_LINE,
    vtk.VTK_TRIANGLE,
    vtk.VTK_QUAD,
    vtk.VTK_POLYGON,
    vtk.VTK_TETRA,
    vtk.VTK_HEXAHEDRON,
    vtk.VTK_WEDGE,
    vtk.VTK_PYRAMID,
    vtk.VTK_POLYHEDRON,
    vtk.VTK_QUADRATIC_EDGE,
    vtk.VTK_QUADRATIC_TRIANGLE,
    vtk.VTK_QUADRATIC_QUAD,
    vtk.VTK_QUADRATIC_TETRA,
    vtk.VTK_QUADRATIC_HEXAHEDRON,
    vtk.VTK_QUADRATIC_WEDGE,
    vtk.VTK_QUADRATIC_PYRAMID,
]
# VTK_CONVEX_POINT_SET is tested separately in TestEnSightWriterConvexPointSet.py

reference_reader = vtk.vtkEnSightGoldBinaryReader()
reference_reader.SetCaseFileName(os.path.join(VTK_DATA_ROOT, "Data/EnSight/elementTypesTest.case"))
reference_reader.Update()
reference_mesh = reference_reader.GetOutput().GetBlock(0)

writer = vtk.vtkEnSightWriter()
writer.SetFileName(os.path.join(VTK_TEMP_DIR, "ensightWriterElementTypesTest.case"))
writer.SetInputData(reference_mesh)
writer.Write()
writer.WriteCaseFile(0)

reader = vtk.vtkEnSightGoldBinaryReader()
reader.SetCaseFileName(os.path.join(VTK_TEMP_DIR, "ensightWriterElementTypesTest.0.case"))
reader.Update()
mesh = reader.GetOutput().GetBlock(0)

assert mesh.GetNumberOfPoints() == reference_mesh.GetNumberOfPoints()
assert mesh.GetNumberOfCells() == reference_mesh.GetNumberOfCells()

for cell_type in CELL_TYPES_TO_TEST:
    reference_cell = get_cell_by_cell_type(reference_mesh, cell_type)
    cell = get_cell_by_cell_type(mesh, cell_type)
    assert_cells_equal(reference_cell, cell)
