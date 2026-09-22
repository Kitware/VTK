# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Test transcription of VTK_QUADRATIC_HEXAHEDRON cells into a vtkCellGrid.

The transcribed cell grid must
+ hold a 20-component connectivity array whose mid-edge nodes are permuted
  from VTK's (bottom, top, vertical) edge order into the (bottom, vertical,
  top) order of the HexI2 basis (the Exodus/IOSS HEX20 convention);
+ mark the shape and point-data attributes as CG HGRAD "I" order-2; and
+ fall back to an order-1 (corners-only) transcription with a warning when
  linear and quadratic hexahedra are mixed in one partition.
"""
import json
import math
import os

from vtkmodules.vtkCommonCore import vtkDoubleArray, vtkPoints
from vtkmodules.vtkCommonDataModel import (
    VTK_HEXAHEDRON,
    VTK_QUADRATIC_HEXAHEDRON,
    vtkCellGridEvaluator,
    vtkUnstructuredGrid,
)
from vtkmodules.vtkFiltersCellGrid import (
    vtkFiltersCellGrid,
    vtkUnstructuredGridToCellGrid,
)
from vtkmodules.vtkIOCellGrid import vtkCellGridWriter
from vtkmodules.test import Testing

vtkFiltersCellGrid.RegisterCellsAndResponders()

# VTK_QUADRATIC_HEXAHEDRON node layout: 8 corners, then mid-edge nodes for
# bottom edges (0,1)(1,2)(2,3)(3,0), top edges (4,5)(5,6)(6,7)(7,4), and
# vertical edges (0,4)(1,5)(2,6)(3,7).
CORNERS = [
    (0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (1.0, 1.0, 0.0), (0.0, 1.0, 0.0),
    (0.0, 0.0, 1.0), (1.0, 0.0, 1.0), (1.0, 1.0, 1.0), (0.0, 1.0, 1.0),
]
VTK_EDGES = [(0, 1), (1, 2), (2, 3), (3, 0),
             (4, 5), (5, 6), (6, 7), (7, 4),
             (0, 4), (1, 5), (2, 6), (3, 7)]
# The expected DG connectivity: slot ii holds VTK node PERMUTATION[ii].
PERMUTATION = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 16, 17, 18, 19, 12, 13, 14, 15]


def curvedHexPoints():
    """Return the 20 points of a curved quadratic hex (mid-edge nodes are
    displaced off the linear edge midpoints so the mapping is genuinely
    quadratic)."""
    pts = list(CORNERS)
    for k, (a, b) in enumerate(VTK_EDGES):
        mid = [(CORNERS[a][i] + CORNERS[b][i]) / 2.0 for i in range(3)]
        mid[(k + 1) % 3] += 0.05 + 0.005 * k  # asymmetric bulge
        pts.append(tuple(mid))
    return pts


def hex20BasisVTK(r, s, t):
    """20-node serendipity basis on the [-1,1]^3 reference element in VTK
    node ordering; used to compute expected interpolated values."""
    c = [(-1, -1, -1), (1, -1, -1), (1, 1, -1), (-1, 1, -1),
         (-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1)]
    basis = []
    for (a, b, g) in c:
        basis.append(
            0.125 * (1 + a * r) * (1 + b * s) * (1 + g * t) * (a * r + b * s + g * t - 2))
    for (i, j) in VTK_EDGES:
        (a1, b1, g1), (a2, b2, g2) = c[i], c[j]
        am, bm, gm = (a1 + a2) / 2, (b1 + b2) / 2, (g1 + g2) / 2
        term = 0.25
        term *= (1 - r * r) if am == 0 else (1 + am * r)
        term *= (1 - s * s) if bm == 0 else (1 + bm * s)
        term *= (1 - t * t) if gm == 0 else (1 + gm * t)
        basis.append(term)
    return basis


def transcribe(ugrid):
    converter = vtkUnstructuredGridToCellGrid()
    converter.SetInputDataObject(0, ugrid)
    converter.Update()
    pdc = converter.GetOutputDataObject(0)
    return pdc.GetPartitionedDataSet(0).GetPartitionAsDataObject(0)


def shapeInfoFromWriter(cgrid, filename):
    """Return the shape attribute's per-cell-type info dictionary by writing
    the grid to a .dg file (vtkCellAttribute::CellTypeInfo is not wrapped)."""
    path = os.path.join(Testing.VTK_TEMP_DIR, filename)
    writer = vtkCellGridWriter()
    writer.SetInputDataObject(0, cgrid)
    writer.SetFileName(path)
    writer.Write()
    with open(path, encoding='utf-8') as fp:
        doc = json.load(fp)
    infos = {}
    for attribute in doc['attributes']:
        infos[attribute['name']] = attribute['cell-info']['vtkDGHex']
    return infos


class TestQuadraticHexToCellGrid(Testing.vtkTest):

    def testQuadraticHexTranscription(self):
        allpts = curvedHexPoints()
        points = vtkPoints()
        scalar = vtkDoubleArray()
        scalar.SetName('distance')
        for pt in allpts:
            points.InsertNextPoint(*pt)
            scalar.InsertNextValue(math.sqrt(sum(c * c for c in pt)))
        ugrid = vtkUnstructuredGrid()
        ugrid.SetPoints(points)
        ugrid.GetPointData().AddArray(scalar)
        ugrid.InsertNextCell(VTK_QUADRATIC_HEXAHEDRON, 20, list(range(20)))

        cgrid = transcribe(ugrid)
        self.assertEqual(cgrid.GetNumberOfCells(), 1)

        # The connectivity must be 20 wide and permuted into HexI2 node order.
        conn = cgrid.GetAttributes('vtkDGHex').GetScalars()
        self.assertEqual(conn.GetNumberOfComponents(), 20)
        self.assertEqual(conn.GetNumberOfTuples(), 1)
        self.assertEqual(
            [int(conn.GetComponent(0, ii)) for ii in range(20)], PERMUTATION)

        # Both the shape and the point-data attribute must be CG HGRAD I2.
        infos = shapeInfoFromWriter(cgrid, 'quadratic-hex.dg')
        for name in ('shape', 'distance'):
            self.assertEqual(infos[name]['dof-sharing'], 'CG')
            self.assertEqual(infos[name]['function-space'], 'HGRAD')
            self.assertEqual(infos[name]['basis'], 'I')
            self.assertEqual(infos[name]['order'], 2)

        # Functional check: interpolate the scalar attribute at probe points
        # computed from the reference HEX20 basis in VTK node ordering. A
        # wrong node permutation (or a linear basis) would not reproduce the
        # analytic values on this curved cell.
        probeParams = [(-0.4, 0.2, 0.3), (0.5, -0.25, -0.2), (0.0, 0.0, 0.0),
                       (0.25, 0.4, -0.3)]
        probe = vtkDoubleArray()
        probe.SetNumberOfComponents(3)
        svals = [scalar.GetValue(ii) for ii in range(20)]
        expected = []
        for rst in probeParams:
            basis = hex20BasisVTK(*rst)
            self.assertAlmostEqual(sum(basis), 1.0, places=12)
            position = [
                sum(basis[kk] * allpts[kk][ii] for kk in range(20)) for ii in range(3)]
            expected.append(sum(basis[kk] * svals[kk] for kk in range(20)))
            probe.InsertNextTuple3(*position)

        evaluator = vtkCellGridEvaluator()
        evaluator.SetCellAttribute(cgrid.GetCellAttributeByName('distance'))
        evaluator.InterpolatePoints(probe)
        cgrid.Query(evaluator)
        values = evaluator.GetInterpolatedValues()
        pointIds = evaluator.GetClassifierPointIDs()
        cellIndices = evaluator.GetClassifierCellIndices()
        classified = 0
        for kk in range(values.GetNumberOfTuples()):
            if int(cellIndices.GetValue(kk)) + 1 == 0:
                continue  # Probe point was not classified to a cell.
            classified += 1
            pid = pointIds.GetValue(kk)
            self.assertAlmostEqual(
                values.GetComponent(kk, 0), expected[pid], places=6)
        # All four probe points are interior; every one must be classified.
        self.assertEqual(classified, len(probeParams))

    def testMixedOrderFallback(self):
        # A partition mixing linear and quadratic hexahedra must be
        # transcribed at order 1 (corners only).
        allpts = curvedHexPoints()
        points = vtkPoints()
        for pt in allpts:
            points.InsertNextPoint(*pt)
        for pt in CORNERS:
            points.InsertNextPoint(pt[0], pt[1] + 2.0, pt[2])
        ugrid = vtkUnstructuredGrid()
        ugrid.SetPoints(points)
        ugrid.InsertNextCell(VTK_QUADRATIC_HEXAHEDRON, 20, list(range(20)))
        ugrid.InsertNextCell(VTK_HEXAHEDRON, 8, [20 + ii for ii in range(8)])

        cgrid = transcribe(ugrid)
        conn = cgrid.GetAttributes('vtkDGHex').GetScalars()
        self.assertEqual(conn.GetNumberOfComponents(), 8)
        self.assertEqual(conn.GetNumberOfTuples(), 2)
        infos = shapeInfoFromWriter(cgrid, 'mixed-order-hex.dg')
        self.assertEqual(infos['shape']['basis'], 'C')
        self.assertEqual(infos['shape']['order'], 1)


if __name__ == '__main__':
    Testing.main([(TestQuadraticHexToCellGrid, 'test')])
