// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME
// .SECTION Description
// this program tests vtkUnstructuredGrid

#include "vtkCell.h"
#include "vtkGenericCell.h"
#include "vtkHexahedron.h"
#include "vtkIdList.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPentagonalPrism.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace
{
// The points of the edge of the built polyhedron `cell` with the 2 points `pts`, in the order of
// `pts`, or none if it has no such edge: the edges of a polyhedron are those of vtkPolyhedron in
// another order, see vtkCellTypeSides::PolyhedronCellSides.
std::vector<vtkIdType> FindPolyhedronEdge(vtkGenericCell* cell, const std::vector<vtkIdType>& pts)
{
  for (int e = 0; pts.size() == 2 && e < cell->GetNumberOfEdges(); ++e)
  {
    vtkIdList* ids = cell->GetEdge(e)->GetPointIds();
    if (std::minmax(ids->GetId(0), ids->GetId(1)) == std::minmax(pts[0], pts[1]))
    {
      return pts;
    }
  }
  return {};
}

// Compare GetCellNumberOfEdges(), GetCellNumberOfFaces(), GetCellEdgePoints() and
// GetCellFacePoints() with the edges and faces of the cells built by GetCell(), the faces of a
// voxel, which are in the order of a polygon, in any order, and the edges of a polyhedron, which
// are those of vtkPolyhedron in any order, see FindPolyhedronEdge().
bool CompareSides(vtkUnstructuredGrid* ug)
{
  vtkNew<vtkIdList> sidePts;
  vtkNew<vtkIdList> cellPts;
  vtkNew<vtkGenericCell> tempCell;
  vtkNew<vtkGenericCell> cell;
  for (vtkIdType cellId = 0; cellId < ug->GetNumberOfCells(); ++cellId)
  {
    ug->GetCell(cellId, cell);
    unsigned char cellType;
    const int numEdges = ug->GetCellNumberOfEdges(cellId, cellType, cell);
    const int numFaces = ug->GetCellNumberOfFaces(cellId, cellType, cell);
    if (cellType != ug->GetCellType(cellId) || numEdges != cell->GetNumberOfEdges() ||
      numFaces != cell->GetNumberOfFaces())
    {
      vtkLog(ERROR, "Wrong number of edges or faces of cell " << cellId);
      return false;
    }
    // The sides of a dimension, and the one beyond them, which has no point: some cells clamp the
    // side id of GetEdge() and GetFace().
    auto compare = [&](int sideDim, int numSides)
    {
      const bool polyhedronEdges = cellType == VTK_POLYHEDRON && sideDim == 1;
      std::vector<std::pair<vtkIdType, vtkIdType>> edges;
      for (int sideId = 0; sideId <= numSides; ++sideId)
      {
        if (sideDim == 1)
        {
          ug->GetCellEdgePoints(cellId, sideId, sidePts, cellType, cellPts, tempCell);
        }
        else
        {
          ug->GetCellFacePoints(cellId, sideId, sidePts, cellType, cellPts, tempCell);
        }
        std::vector<vtkIdType> pts(sidePts->begin(), sidePts->end());
        std::vector<vtkIdType> expected;
        if (sideId < numSides)
        {
          vtkIdList* ids =
            (sideDim == 1 ? cell->GetEdge(sideId) : cell->GetFace(sideId))->GetPointIds();
          expected.assign(ids->begin(), ids->end());
        }
        if (cellType == VTK_VOXEL && sideDim == 2)
        {
          std::sort(pts.begin(), pts.end());
          std::sort(expected.begin(), expected.end());
        }
        if (polyhedronEdges && sideId < numSides)
        {
          expected = FindPolyhedronEdge(cell, pts);
          if (!expected.empty())
          {
            edges.emplace_back(std::minmax(pts[0], pts[1]));
          }
        }
        if (pts != expected)
        {
          vtkLog(ERROR,
            "Wrong points of side " << sideId << " of dimension " << sideDim << " of cell "
                                    << cellId);
          return false;
        }
      }
      // The edges of a polyhedron are distinct, so that they are all those of vtkPolyhedron.
      std::sort(edges.begin(), edges.end());
      if (std::unique(edges.begin(), edges.end()) != edges.end())
      {
        vtkLog(ERROR, "Repeated edges of polyhedron " << cellId);
        return false;
      }
      return true;
    };
    if (!compare(1, numEdges) || !compare(2, numFaces))
    {
      return false;
    }
  }
  return true;
}

// The cells of the unit cube: a voxel, whose faces are polygons, a hexahedron, a wedge, the
// hexahedron as a polyhedron, whose edges are given by Euler's formula, and the corners as a
// convex point set, whose 2 * 8 - 4 faces are triangles.
bool TestCubeCells()
{
  // Point i is (i & 1, (i >> 1) & 1, (i >> 2) & 1), in voxel order.
  vtkNew<vtkPoints> points;
  for (unsigned int i = 0; i < 8; ++i)
  {
    points->InsertNextPoint(i & 1U, (i >> 1U) & 1U, (i >> 2U) & 1U);
  }
  const vtkIdType voxel[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
  const vtkIdType hex[8] = { 0, 1, 3, 2, 4, 5, 7, 6 };
  const vtkIdType wedge[6] = { 0, 1, 2, 4, 5, 6 };
  std::vector<vtkIdType> hexFaces;
  for (int f = 0; f < vtkHexahedron::NumberOfFaces; ++f)
  {
    hexFaces.push_back(4);
    for (int k = 0; k < 4; ++k)
    {
      hexFaces.push_back(hex[vtkHexahedron::GetFaceArray(f)[k]]);
    }
  }
  vtkNew<vtkUnstructuredGrid> ug;
  ug->SetPoints(points);
  ug->InsertNextCell(VTK_VOXEL, 8, voxel);
  ug->InsertNextCell(VTK_HEXAHEDRON, 8, hex);
  ug->InsertNextCell(VTK_WEDGE, 6, wedge);
  ug->InsertNextCell(VTK_POLYHEDRON, 8, hex, vtkHexahedron::NumberOfFaces, hexFaces.data());
  ug->InsertNextCell(VTK_CONVEX_POINT_SET, 8, voxel);
  if (!CompareSides(ug))
  {
    return false;
  }

  // The consecutive points of a face of a voxel differ by one coordinate.
  vtkNew<vtkIdList> facePts;
  vtkNew<vtkIdList> cellPts;
  vtkNew<vtkGenericCell> tempCell;
  for (int faceId = 0; faceId < 6; ++faceId)
  {
    unsigned char cellType;
    ug->GetCellFacePoints(0, faceId, facePts, cellType, cellPts, tempCell);
    for (vtkIdType k = 0; k < 4; ++k)
    {
      const vtkIdType diff = facePts->GetId(k) ^ facePts->GetId((k + 1) % 4);
      if (diff != 1 && diff != 2 && diff != 4)
      {
        vtkLog(ERROR, "Face " << faceId << " of a voxel is not a polygon");
        return false;
      }
    }
  }
  return true;
}

// A pentagonal prism as a polyhedron, whose faces are pentagons and quads, wound as those of
// vtkPentagonalPrism.
bool TestPolyhedron()
{
  vtkNew<vtkPoints> points;
  for (int k = 0; k < 2; ++k)
  {
    for (int i = 0; i < 5; ++i)
    {
      points->InsertNextPoint(
        std::cos(0.4 * vtkMath::Pi() * i), std::sin(0.4 * vtkMath::Pi() * i), k);
    }
  }
  std::vector<vtkIdType> faces;
  for (int f = 0; f < vtkPentagonalPrism::NumberOfFaces; ++f)
  {
    const vtkIdType* face = vtkPentagonalPrism::GetFaceArray(f);
    const vtkIdType numFacePts =
      std::find(face, face + vtkPentagonalPrism::MaximumFaceSize, -1) - face;
    faces.push_back(numFacePts);
    faces.insert(faces.end(), face, face + numFacePts);
  }
  const vtkIdType prism[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  vtkNew<vtkUnstructuredGrid> ug;
  ug->SetPoints(points);
  ug->InsertNextCell(VTK_POLYHEDRON, 10, prism, vtkPentagonalPrism::NumberOfFaces, faces.data());
  return CompareSides(ug);
}

// One cell of every cell type of a fixed size, of a polygon, and of cells built as a fallback,
// with distinct points: point k of cell t is (t + 7 * k) % numPoints.
// Exercises FixedSizeCellSides, PointListCellSides, TriangleStripCellSides,
// HigherOrderCellSides, PolygonCellSides (both linear and quadratic), and ConvexPointSetSides
// with interior points.
bool TestCellTypes()
{
  constexpr vtkIdType numPoints = 31;
  vtkNew<vtkPoints> points;
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    points->InsertNextPoint(
      static_cast<double>(i), static_cast<double>((i * i) % 7), static_cast<double>((i * 3) % 5));
  }
  const std::vector<std::pair<int, int>> types = { { VTK_VERTEX, 1 }, { VTK_LINE, 2 },
    { VTK_TRIANGLE, 3 }, { VTK_PIXEL, 4 }, { VTK_QUAD, 4 }, { VTK_POLYGON, 5 }, { VTK_TETRA, 4 },
    { VTK_VOXEL, 8 }, { VTK_HEXAHEDRON, 8 }, { VTK_WEDGE, 6 }, { VTK_PYRAMID, 5 },
    { VTK_PENTAGONAL_PRISM, 10 }, { VTK_HEXAGONAL_PRISM, 12 }, { VTK_QUADRATIC_EDGE, 3 },
    { VTK_CUBIC_LINE, 4 }, { VTK_QUADRATIC_TRIANGLE, 6 }, { VTK_BIQUADRATIC_TRIANGLE, 7 },
    { VTK_QUADRATIC_QUAD, 8 }, { VTK_BIQUADRATIC_QUAD, 9 }, { VTK_QUADRATIC_LINEAR_QUAD, 6 },
    { VTK_QUADRATIC_TETRA, 10 }, { VTK_QUADRATIC_HEXAHEDRON, 20 }, { VTK_QUADRATIC_WEDGE, 15 },
    { VTK_QUADRATIC_PYRAMID, 13 }, { VTK_TRIQUADRATIC_HEXAHEDRON, 27 },
    { VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON, 24 }, { VTK_QUADRATIC_LINEAR_WEDGE, 12 },
    { VTK_BIQUADRATIC_QUADRATIC_WEDGE, 18 }, { VTK_TRIQUADRATIC_PYRAMID, 19 },
    { VTK_POLY_VERTEX, 3 }, { VTK_POLY_LINE, 3 }, { VTK_TRIANGLE_STRIP, 4 },
    { VTK_QUADRATIC_POLYGON, 8 }, { VTK_LAGRANGE_HEXAHEDRON, 8 }, { VTK_LAGRANGE_WEDGE, 6 },
    { VTK_BEZIER_TETRAHEDRON, 4 }, { VTK_CONVEX_POINT_SET, 9 } };

  vtkNew<vtkUnstructuredGrid> ug;
  ug->SetPoints(points);
  for (size_t t = 0; t < types.size(); ++t)
  {
    std::vector<vtkIdType> pts(types[t].second);
    for (vtkIdType k = 0; k < static_cast<vtkIdType>(pts.size()); ++k)
    {
      pts[k] = (static_cast<vtkIdType>(t) + 7 * k) % numPoints;
    }
    ug->InsertNextCell(types[t].first, static_cast<vtkIdType>(pts.size()), pts.data());
  }
  return CompareSides(ug);
}
}

int otherUnstructuredGrid(int, char*[])
{
  int retVal = EXIT_SUCCESS;

  if (!TestCubeCells() || !TestPolyhedron() || !TestCellTypes())
  {
    retVal = EXIT_FAILURE;
  }

  vtkNew<vtkUnstructuredGrid> ug;
  if (!ug->SupportsGhostArray(vtkDataObject::POINT) || !ug->SupportsGhostArray(vtkDataObject::CELL))
  {
    vtkLog(ERROR, "Unexpected results on SupportsGhostArray");
    retVal = EXIT_FAILURE;
  }

  vtkUnsignedCharArray* distinctCellTypes = ug->GetDistinctCellTypesArray();
  if (!distinctCellTypes)
  {
    vtkLog(ERROR, "vtkUnstructuredGrid::GetDistinctCellTypesArray() should never return nullptr");
    retVal = EXIT_FAILURE;
  }
  else if (distinctCellTypes->GetNumberOfTuples())
  {
    vtkLog(ERROR, "vtkUnstructuredGrid::GetDistinctCellTypesArray() should return an empty array");
    retVal = EXIT_FAILURE;
  }

  return retVal;
}
