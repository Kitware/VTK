#!/usr/bin/env python
"""Test vtkOBBTree::IntersectWithLine on polydata whose cells reference
points that are not at the start of the point array (i.e. the cell point ids
are global point ids, not local cell point ids)."""

from vtkmodules.vtkCommonCore import vtkIdList, vtkPoints
from vtkmodules.vtkCommonDataModel import vtkCellArray, vtkPolyData
from vtkmodules.vtkFiltersGeneral import vtkOBBTree
from vtkmodules.test import Testing


def make_mesh(offset, closed):
    vertices = [(0, 0, 0), (1, 0, 0), (0, 1, 0)]
    faces = [(0, 1, 2)]
    if closed:
        vertices.append((0, 0, 1))
        faces = [(0, 2, 1), (0, 1, 3), (0, 3, 2), (1, 2, 3)]

    # All cells reference valid points at the end of the array.
    points = vtkPoints()
    points.SetNumberOfPoints(offset + len(vertices))
    for index in range(offset):
        points.SetPoint(index, 0, 0, 0)
    for index, xyz in enumerate(vertices):
        points.SetPoint(offset + index, *xyz)

    cells = vtkCellArray()
    for face in faces:
        cells.InsertNextCell(3)
        for index in face:
            cells.InsertCellPoint(offset + index)

    mesh = vtkPolyData()
    mesh.SetPoints(points)
    mesh.SetPolys(cells)
    return mesh


class TestOBBTreeIntersectWithLine(Testing.vtkTest):
    OFFSET = 100000

    def intersect(self, closed, tolerance=None):
        tree = vtkOBBTree()
        tree.SetDataSet(make_mesh(self.OFFSET, closed))
        tree.BuildLocator()

        hits, cellIds = vtkPoints(), vtkIdList()
        start, end = (0.25, 0.25, -1), (0.25, 0.25, 1)
        if tolerance is None:
            tree.IntersectWithLine(start, end, hits, cellIds)
        else:
            tree.IntersectWithLine(start, end, tolerance, hits, cellIds)
        ids = [cellIds.GetId(i) for i in range(cellIds.GetNumberOfIds())]
        coords = [hits.GetPoint(i) for i in range(hits.GetNumberOfPoints())]
        return ids, coords

    def testTriangle(self):
        for tolerance in (None, 1e-6):
            ids, coords = self.intersect(False, tolerance)
            self.assertEqual(ids, [0])
            self.assertEqual(coords, [(0.25, 0.25, 0.0)])

    def testClosedTetrahedron(self):
        for tolerance in (None, 1e-6):
            ids, coords = self.intersect(True, tolerance)
            self.assertEqual(ids, [0, 3])
            self.assertEqual(coords, [(0.25, 0.25, 0.0), (0.25, 0.25, 0.5)])


if __name__ == "__main__":
    Testing.main([(TestOBBTreeIntersectWithLine, 'test')])
