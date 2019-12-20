/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellValidator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellValidator.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkPointData.h"
#include "vtkShortArray.h"

#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPoints.h"

#include "vtkBezierCurve.h"
#include "vtkBezierHexahedron.h"
#include "vtkBezierQuadrilateral.h"
#include "vtkBezierTetra.h"
#include "vtkBezierTriangle.h"
#include "vtkBezierWedge.h"
#include "vtkBiQuadraticQuad.h"
#include "vtkBiQuadraticQuadraticHexahedron.h"
#include "vtkBiQuadraticQuadraticWedge.h"
#include "vtkBiQuadraticTriangle.h"
#include "vtkConvexPointSet.h"
#include "vtkCubicLine.h"
#include "vtkEmptyCell.h"
#include "vtkGenericCell.h"
#include "vtkHexagonalPrism.h"
#include "vtkHexahedron.h"
#include "vtkLagrangeCurve.h"
#include "vtkLagrangeHexahedron.h"
#include "vtkLagrangeQuadrilateral.h"
#include "vtkLagrangeTetra.h"
#include "vtkLagrangeTriangle.h"
#include "vtkLagrangeWedge.h"
#include "vtkLine.h"
#include "vtkPentagonalPrism.h"
#include "vtkPixel.h"
#include "vtkPolyLine.h"
#include "vtkPolyVertex.h"
#include "vtkPolygon.h"
#include "vtkPolyhedron.h"
#include "vtkPyramid.h"
#include "vtkQuad.h"
#include "vtkQuadraticEdge.h"
#include "vtkQuadraticHexahedron.h"
#include "vtkQuadraticLinearQuad.h"
#include "vtkQuadraticLinearWedge.h"
#include "vtkQuadraticPolygon.h"
#include "vtkQuadraticPyramid.h"
#include "vtkQuadraticQuad.h"
#include "vtkQuadraticTetra.h"
#include "vtkQuadraticTriangle.h"
#include "vtkQuadraticWedge.h"
#include "vtkTetra.h"
#include "vtkTriQuadraticHexahedron.h"
#include "vtkTriangle.h"
#include "vtkTriangleStrip.h"
#include "vtkVertex.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkUnstructuredGrid.h"

#include <cassert>
#include <cmath>
#include <sstream>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCellValidator);

//----------------------------------------------------------------------------
vtkCellValidator::vtkCellValidator()
{
  this->Tolerance = FLT_EPSILON;
}

//----------------------------------------------------------------------------
namespace
{
bool PointsAreCoincident(double p[3], double q[3], double tolerance)
{
  return (std::abs(p[0] - q[0]) < tolerance && std::abs(p[1] - q[1]) < tolerance &&
    std::abs(p[2] - q[2]) < tolerance);
}

bool LineSegmentsIntersect(double p1[3], double p2[3], double q1[3], double q2[3], double tolerance)
{
  double u, v;
  static const int VTK_YES_INTERSECTION = 2;
  if (vtkLine::Intersection3D(p1, p2, q1, q2, u, v) == VTK_YES_INTERSECTION)
  {
    if ((std::abs(u) > tolerance && std::abs(u - 1.) > tolerance) ||
      (std::abs(v) > tolerance && std::abs(v - 1.) > tolerance))
    {
      return true;
    }
  }
  return false;
}
}

//----------------------------------------------------------------------------
bool vtkCellValidator::NoIntersectingEdges(vtkCell* cell, double tolerance)
{
  // Ensures no cell edges intersect.
  //
  // NB: To accommodate higher order cells, we need to first linearize the edges
  //     before testing their intersection.

  double p[2][3], x[2][3];
  vtkIdType nEdges = cell->GetNumberOfEdges();
  vtkCell* edge;
  vtkNew<vtkIdList> idList1, idList2;
  vtkNew<vtkPoints> points1, points2;
  int subId = -1;
  for (vtkIdType i = 0; i < nEdges; i++)
  {
    edge = cell->GetEdge(i);
    edge->Triangulate(subId, idList1.GetPointer(), points1.GetPointer());
    for (vtkIdType e1 = 0; e1 < points1->GetNumberOfPoints(); e1 += 2)
    {
      points1->GetPoint(e1, p[0]);
      points1->GetPoint(e1 + 1, p[1]);
      for (vtkIdType j = i + 1; j < nEdges; j++)
      {
        edge = cell->GetEdge(j);
        edge->Triangulate(subId, idList2.GetPointer(), points2.GetPointer());
        for (vtkIdType e2 = 0; e2 < points2->GetNumberOfPoints(); e2 += 2)
        {
          points2->GetPoint(e2, x[0]);
          points2->GetPoint(e2 + 1, x[1]);

          if (LineSegmentsIntersect(p[0], p[1], x[0], x[1], tolerance))
          {
            return false;
          }
        }
      }
    }
  }
  return true;
}

//----------------------------------------------------------------------------
namespace
{
bool TrianglesIntersect(double p1[3], double p2[3], double p3[3], double q1[3], double q2[3],
  double q3[3], double tolerance)
{
  if (vtkTriangle::TrianglesIntersect(p1, p2, p3, q1, q2, q3) == 1)
  {
    double* p[3] = { p1, p2, p3 };
    double* q[3] = { q1, q2, q3 };

    int nCoincidentPoints = 0;

    for (int i = 0; i < 3; i++)
    {
      for (int j = 0; j < 3; j++)
      {
        if (LineSegmentsIntersect(p[i], p[(i + 1) % 3], q[j], q[(j + 1) % 3], tolerance))
        {
          return false;
        }
        nCoincidentPoints += int(PointsAreCoincident(p[i], q[j], tolerance));
      }
    }
    return (nCoincidentPoints != 1 && nCoincidentPoints != 2);
  }
  return false;
}
}

//----------------------------------------------------------------------------
bool vtkCellValidator::NoIntersectingFaces(vtkCell* cell, double tolerance)
{
  // Ensures no cell faces intersect.

  double p[3][3], x[3][3];
  vtkIdType nFaces = cell->GetNumberOfFaces();
  vtkCell* face;
  vtkNew<vtkIdList> idList1, idList2;
  vtkNew<vtkPoints> points1, points2;
  int subId = -1;
  for (vtkIdType i = 0; i < nFaces; i++)
  {
    face = cell->GetFace(i);
    face->Triangulate(subId, idList1.GetPointer(), points1.GetPointer());
    for (vtkIdType e1 = 0; e1 < points1->GetNumberOfPoints(); e1 += 3)
    {
      points1->GetPoint(e1, p[0]);
      points1->GetPoint(e1 + 1, p[1]);
      points1->GetPoint(e1 + 2, p[2]);
      for (vtkIdType j = i + 1; j < nFaces; j++)
      {
        face = cell->GetFace(j);
        face->Triangulate(subId, idList2.GetPointer(), points2.GetPointer());
        for (vtkIdType e2 = 0; e2 < points2->GetNumberOfPoints(); e2 += 3)
        {
          points2->GetPoint(e2, x[0]);
          points2->GetPoint(e2 + 1, x[1]);
          points2->GetPoint(e2 + 2, x[2]);

          if (TrianglesIntersect(p[0], p[1], p[2], x[0], x[1], x[2], tolerance))
          {
            return false;
          }
        }
      }
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkCellValidator::ContiguousEdges(vtkCell* twoDimensionalCell, double tolerance)
{
  // Ensures that a two-dimensional cell's edges are contiguous.
  //
  // NB: we cannot simply test the values of point ids, since cells have the
  //     tricky habit of generating their edge cells on the fly and cell Ids are
  //     only congruent w.r.t. a single point array. To be thorough, we need to
  //     compare point values.

  assert(twoDimensionalCell->GetCellDimension() == 2);

  double points[4][3];
  double *p[2] = { points[0], points[1] }, *x[2] = { points[2], points[3] }, u, v;
  vtkCell* edge = twoDimensionalCell->GetEdge(0);
  vtkIdType nEdges = twoDimensionalCell->GetNumberOfEdges();
  // Need to use local indices, not global
  edge->GetPoints()->GetPoint(0, p[0]);
  edge->GetPoints()->GetPoint(1, p[1]);
  for (vtkIdType i = 0; i < nEdges; i++)
  {
    edge = twoDimensionalCell->GetEdge((i + 1) % nEdges);
    // Need to use local indices, not global
    edge->GetPoints()->GetPoint(0, x[0]);
    edge->GetPoints()->GetPoint(1, x[1]);

    static const int VTK_NO_INTERSECTION = 0;
    if (vtkLine::Intersection3D(p[0], p[1], x[0], x[1], u, v) == VTK_NO_INTERSECTION)
    {
      return false;
    }
    else if ((std::abs(u) > tolerance && std::abs(1. - u) > tolerance) ||
      (std::abs(v) > tolerance && std::abs(1. - v) > tolerance))
    {
      return false;
    }
    p[0] = x[0];
    p[1] = x[1];
  }
  return true;
}

//----------------------------------------------------------------------------
namespace
{
void Centroid(vtkCell* cell, double* centroid)
{
  // Return the centroid of a cell in world coordinates.
  static double weights[512];
  double pCenter[3];
  int subId = -1;
  cell->GetParametricCenter(pCenter);
  cell->EvaluateLocation(subId, pCenter, centroid, weights);
}

void Normal(vtkCell* twoDimensionalCell, double* normal)
{
  // Return the normal of a 2-dimensional cell.

  assert(twoDimensionalCell->GetCellDimension() == 2);

  vtkPolygon::ComputeNormal(twoDimensionalCell->GetPoints(), normal);
}
}

//----------------------------------------------------------------------------
bool vtkCellValidator::Convex(vtkCell* cell, double vtkNotUsed(tolerance))
{
  // Determine whether or not a cell is convex. vtkPolygon and vtkPolyhedron can
  // conform to any 2- and 3-dimensional cell, and both have IsConvex(). So, we
  // construct instances of these cells, populate them with the cell data, and
  // proceed with the convexity query.
  switch (cell->GetCellDimension())
  {
    case 0:
    case 1:
      return true;
    case 2:
      return vtkPolygon::IsConvex(cell->GetPoints());
    case 3:
    {
      if (vtkPolyhedron* polyhedron = vtkPolyhedron::SafeDownCast(cell))
      {
        return polyhedron->IsConvex();
      }
      vtkNew<vtkCellArray> polyhedronFaces;
      for (vtkIdType i = 0; i < cell->GetNumberOfFaces(); i++)
      {
        polyhedronFaces->InsertNextCell(cell->GetFace(i));
      }
      vtkNew<vtkIdTypeArray> faceBuffer;
      polyhedronFaces->ExportLegacyFormat(faceBuffer);
      vtkNew<vtkUnstructuredGrid> ugrid;
      ugrid->SetPoints(cell->GetPoints());
      ugrid->InsertNextCell(VTK_POLYHEDRON, cell->GetNumberOfPoints(),
        cell->GetPointIds()->GetPointer(0), polyhedronFaces->GetNumberOfCells(),
        faceBuffer->GetPointer(0));

      vtkPolyhedron* polyhedron = vtkPolyhedron::SafeDownCast(ugrid->GetCell(0));
      return polyhedron->IsConvex();
    }
    default:
      return false;
  }
}

//----------------------------------------------------------------------------
namespace
{
// The convention for three-dimensional cells is that the normal of each face
// cell is oriented outwards. Some cells break this convention and remain
// inconsistent to maintain backwards compatibility.
bool outwardOrientation(int cellType)
{
  if (cellType == VTK_QUADRATIC_LINEAR_WEDGE || cellType == VTK_BIQUADRATIC_QUADRATIC_WEDGE ||
    cellType == VTK_QUADRATIC_WEDGE)
  {
    return false;
  }

  return true;
}
}

//----------------------------------------------------------------------------
bool vtkCellValidator::FacesAreOrientedCorrectly(vtkCell* threeDimensionalCell, double tolerance)
{
  // Ensure that a 3-dimensional cell's faces are oriented away from the
  // cell's centroid.

  assert(threeDimensionalCell->GetCellDimension() == 3);

  double faceNorm[3], norm[3], cellCentroid[3], faceCentroid[3];
  vtkCell* face;
  Centroid(threeDimensionalCell, cellCentroid);

  bool hasOutwardOrientation = outwardOrientation(threeDimensionalCell->GetCellType());

  for (vtkIdType i = 0; i < threeDimensionalCell->GetNumberOfFaces(); i++)
  {
    face = threeDimensionalCell->GetFace(i);
    // If the cell face is not valid, there's no point in continuing the test.
    if (vtkCellValidator::Check(face, tolerance) != State::Valid)
    {
      return false;
    }
    Normal(face, faceNorm);
    Centroid(face, faceCentroid);
    for (vtkIdType j = 0; j < 3; j++)
    {
      norm[j] = faceCentroid[j] - cellCentroid[j];
    }
    vtkMath::Normalize(norm);
    double dot = vtkMath::Dot(faceNorm, norm);

    if (hasOutwardOrientation == (dot < 0.))
    {
      return false;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkCell* cell, double tolerance)
{
  // Ensure the number of points is at least as great as the number of point ids
  if (cell->GetPoints()->GetNumberOfPoints() < cell->GetNumberOfPoints())
  {
    return State::WrongNumberOfPoints;
  }

  switch (cell->GetCellType())
  {

#define CheckCase(CellId, CellType)                                                                \
  case CellId:                                                                                     \
    return vtkCellValidator::Check(CellType::SafeDownCast(cell), tolerance)
    CheckCase(VTK_EMPTY_CELL, vtkEmptyCell);
    CheckCase(VTK_VERTEX, vtkVertex);
    CheckCase(VTK_POLY_VERTEX, vtkPolyVertex);
    CheckCase(VTK_LINE, vtkLine);
    CheckCase(VTK_POLY_LINE, vtkPolyLine);
    CheckCase(VTK_TRIANGLE, vtkTriangle);
    CheckCase(VTK_TRIANGLE_STRIP, vtkTriangleStrip);
    CheckCase(VTK_POLYGON, vtkPolygon);
    CheckCase(VTK_PIXEL, vtkPixel);
    CheckCase(VTK_QUAD, vtkQuad);
    CheckCase(VTK_TETRA, vtkTetra);
    CheckCase(VTK_VOXEL, vtkVoxel);
    CheckCase(VTK_HEXAHEDRON, vtkHexahedron);
    CheckCase(VTK_WEDGE, vtkWedge);
    CheckCase(VTK_PYRAMID, vtkPyramid);
    CheckCase(VTK_PENTAGONAL_PRISM, vtkPentagonalPrism);
    CheckCase(VTK_HEXAGONAL_PRISM, vtkHexagonalPrism);
    CheckCase(VTK_QUADRATIC_EDGE, vtkQuadraticEdge);
    CheckCase(VTK_QUADRATIC_TRIANGLE, vtkQuadraticTriangle);
    CheckCase(VTK_QUADRATIC_QUAD, vtkQuadraticQuad);
    CheckCase(VTK_QUADRATIC_POLYGON, vtkQuadraticPolygon);
    CheckCase(VTK_QUADRATIC_TETRA, vtkQuadraticTetra);
    CheckCase(VTK_QUADRATIC_HEXAHEDRON, vtkQuadraticHexahedron);
    CheckCase(VTK_QUADRATIC_WEDGE, vtkQuadraticWedge);
    CheckCase(VTK_QUADRATIC_PYRAMID, vtkQuadraticPyramid);
    CheckCase(VTK_BIQUADRATIC_QUAD, vtkBiQuadraticQuad);
    CheckCase(VTK_TRIQUADRATIC_HEXAHEDRON, vtkTriQuadraticHexahedron);
    CheckCase(VTK_QUADRATIC_LINEAR_QUAD, vtkQuadraticLinearQuad);
    CheckCase(VTK_QUADRATIC_LINEAR_WEDGE, vtkQuadraticLinearWedge);
    CheckCase(VTK_BIQUADRATIC_QUADRATIC_WEDGE, vtkBiQuadraticQuadraticWedge);
    CheckCase(VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON, vtkBiQuadraticQuadraticHexahedron);
    CheckCase(VTK_BIQUADRATIC_TRIANGLE, vtkBiQuadraticTriangle);
    CheckCase(VTK_CUBIC_LINE, vtkCubicLine);
    CheckCase(VTK_CONVEX_POINT_SET, vtkConvexPointSet);
    CheckCase(VTK_POLYHEDRON, vtkPolyhedron);
    CheckCase(VTK_LAGRANGE_CURVE, vtkLagrangeCurve);
    CheckCase(VTK_LAGRANGE_TRIANGLE, vtkLagrangeTriangle);
    CheckCase(VTK_LAGRANGE_QUADRILATERAL, vtkLagrangeQuadrilateral);
    CheckCase(VTK_LAGRANGE_TETRAHEDRON, vtkLagrangeTetra);
    CheckCase(VTK_LAGRANGE_HEXAHEDRON, vtkLagrangeHexahedron);
    CheckCase(VTK_LAGRANGE_WEDGE, vtkLagrangeWedge);
    CheckCase(VTK_BEZIER_CURVE, vtkBezierCurve);
    CheckCase(VTK_BEZIER_TRIANGLE, vtkBezierTriangle);
    CheckCase(VTK_BEZIER_QUADRILATERAL, vtkBezierQuadrilateral);
    CheckCase(VTK_BEZIER_TETRAHEDRON, vtkBezierTetra);
    CheckCase(VTK_BEZIER_HEXAHEDRON, vtkBezierHexahedron);
    CheckCase(VTK_BEZIER_WEDGE, vtkBezierWedge);
#undef CheckCase

    default:
      return State::Valid;
  }
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkGenericCell* cell, double tolerance)
{
  return vtkCellValidator::Check(cell->GetRepresentativeCell(), tolerance);
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkEmptyCell*, double)
{
  return State::Valid;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkVertex* vertex, double vtkNotUsed(tolerance))
{
  State state = State::Valid;

  // Ensure there is an underlying point id for the vertex
  if (vertex->GetNumberOfPoints() != 1)
  {
    state |= State::WrongNumberOfPoints;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(
  vtkPolyVertex* polyVertex, double vtkNotUsed(tolerance))
{
  State state = State::Valid;

  // Ensure there is an a single underlying point id for the polyVertex
  if (polyVertex->GetNumberOfPoints() < 1)
  {
    state |= State::WrongNumberOfPoints;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkLine* line, double vtkNotUsed(tolerance))
{
  State state = State::Valid;

  // Ensure there are two underlying point ids for the line
  if (line->GetNumberOfPoints() != 2)
  {
    state |= State::WrongNumberOfPoints;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkPolyLine* polyLine, double vtkNotUsed(tolerance))
{
  State state = State::Valid;

  // Ensure there are at least two underlying point ids for the polyLine
  if (polyLine->GetNumberOfPoints() < 2)
  {
    state |= State::WrongNumberOfPoints;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkTriangle* triangle, double tolerance)
{
  State state = State::Valid;

  // Ensure there are three underlying point ids for the triangle
  if (triangle->GetNumberOfPoints() != 3)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that adjacent edges are touching
  if (!ContiguousEdges(triangle, tolerance))
  {
    state |= State::NoncontiguousEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkTriangleStrip* triangleStrip, double tolerance)
{
  State state = State::Valid;

  // Ensure there are at least three underlying point ids for the triangleStrip
  if (triangleStrip->GetNumberOfPoints() < 3)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(triangleStrip, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkPolygon* polygon, double tolerance)
{
  State state = State::Valid;

  // Ensure there are at least three underlying point ids for the polygon
  if (polygon->GetNumberOfPoints() < 3)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(polygon, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that adjacent edges are touching
  if (!ContiguousEdges(polygon, tolerance))
  {
    state |= State::NoncontiguousEdges;
  }

  // Ensure that the polygon is convex
  if (!Convex(polygon, tolerance))
  {
    state |= State::Nonconvex;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkPixel* pixel, double tolerance)
{
  State state = State::Valid;

  // Ensure there are four underlying point ids for the pixel
  if (pixel->GetNumberOfPoints() != 4)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that the voxel points are orthogonal and axis-aligned
  double p[4][3];
  for (vtkIdType i = 0; i < 4; i++)
  {
    pixel->GetPoints()->GetPoint(pixel->GetPointId(i), p[i]);
  }

  // pixel points are axis-aligned and orthogonal, so exactly one coordinate
  // must differ by a tolerance along its edges.
  static int edges[4][2] = { { 0, 1 }, { 1, 3 }, { 2, 3 }, { 0, 2 } };
  for (vtkIdType i = 0; i < 4; i++)
  {
    if ((std::abs(p[edges[i][0]][0] - p[edges[i][1]][0]) > tolerance) +
        (std::abs(p[edges[i][0]][1] - p[edges[i][1]][1]) > tolerance) +
        (std::abs(p[edges[i][0]][2] - p[edges[i][1]][2]) > tolerance) !=
      1)
    {
      state |= State::IntersectingEdges;
    }
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkQuad* quad, double tolerance)
{
  State state = State::Valid;

  // Ensure there are four underlying point ids for the quad
  if (quad->GetNumberOfPoints() != 4)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(quad, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that adjacent edges are touching
  if (!ContiguousEdges(quad, tolerance))
  {
    state |= State::NoncontiguousEdges;
  }

  // Ensure that the quad is convex
  if (!Convex(quad, tolerance))
  {
    state |= State::Nonconvex;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkTetra* tetra, double tolerance)
{
  State state = State::Valid;

  // Ensure there are four underlying point ids for the tetra
  if (tetra->GetNumberOfPoints() != 4)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(tetra, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(tetra, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkVoxel* voxel, double tolerance)
{
  State state = State::Valid;

  // Ensure there are four underlying point ids for the voxel
  if (voxel->GetNumberOfPoints() != 8)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that the voxel points are orthogonal and axis-aligned
  double p[8][3];
  for (vtkIdType i = 0; i < 8; i++)
  {
    voxel->GetPoints()->GetPoint(voxel->GetPointId(i), p[i]);
  }

  // voxel points are axis-aligned and orthogonal, so exactly one coordinate
  // must differ by a tolerance along its edges.
  static int edges[12][2] = {
    { 0, 1 },
    { 1, 3 },
    { 2, 3 },
    { 0, 2 },
    { 4, 5 },
    { 5, 7 },
    { 6, 7 },
    { 4, 6 },
    { 0, 4 },
    { 1, 5 },
    { 2, 6 },
    { 3, 7 },
  };
  for (vtkIdType i = 0; i < 12; i++)
  {
    if ((std::abs(p[edges[i][0]][0] - p[edges[i][1]][0]) > tolerance) +
        (std::abs(p[edges[i][0]][1] - p[edges[i][1]][1]) > tolerance) +
        (std::abs(p[edges[i][0]][2] - p[edges[i][1]][2]) > tolerance) !=
      1)
    {
      state |= State::IntersectingEdges;
    }
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkHexahedron* hex, double tolerance)
{
  State state = State::Valid;

  // Ensure there are eight underlying point ids for the hex
  if (hex->GetNumberOfPoints() != 8)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(hex, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(hex, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure that the hex is convex
  if (!Convex(hex, tolerance))
  {
    state |= State::Nonconvex;
  }

  // Ensure the hexahedron's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(hex, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkWedge* wedge, double tolerance)
{
  State state = State::Valid;

  // Ensure there are six underlying point ids for the wedge
  if (wedge->GetNumberOfPoints() != 6)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(wedge, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(wedge, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure that the wedge is convex
  if (!Convex(wedge, tolerance))
  {
    state |= State::Nonconvex;
  }

  // Ensure the wedge's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(wedge, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkPyramid* pyramid, double tolerance)
{
  State state = State::Valid;

  // Ensure there are five underlying point ids for the pyramid
  if (pyramid->GetNumberOfPoints() != 5)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(pyramid, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(pyramid, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure that the pyramid is convex
  if (!Convex(pyramid, tolerance))
  {
    state |= State::Nonconvex;
  }

  // Ensure the wedge's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(pyramid, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(
  vtkPentagonalPrism* pentagonalPrism, double tolerance)
{
  State state = State::Valid;

  // Ensure there are ten underlying point ids for the pentagonal prism
  if (pentagonalPrism->GetNumberOfPoints() != 10)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(pentagonalPrism, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(pentagonalPrism, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure that the pentagonal prism is convex
  if (!Convex(pentagonalPrism, tolerance))
  {
    state |= State::Nonconvex;
  }

  // Ensure the prism's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(pentagonalPrism, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkHexagonalPrism* hexagonalPrism, double tolerance)
{
  State state = State::Valid;

  // Ensure there are ten underlying point ids for the hexagonal prism
  if (hexagonalPrism->GetNumberOfPoints() != 12)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(hexagonalPrism, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(hexagonalPrism, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure that the hexagonal prism is convex
  if (!Convex(hexagonalPrism, tolerance))
  {
    state |= State::Nonconvex;
  }

  // Ensure the prism's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(hexagonalPrism, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkQuadraticEdge* edge, double tolerance)
{
  State state = State::Valid;

  // Ensure there are three underlying point ids for the edge
  if (edge->GetNumberOfPoints() != 3)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(edge, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkQuadraticTriangle* triangle, double tolerance)
{
  State state = State::Valid;

  // Ensure there are six underlying point ids for the triangle
  if (triangle->GetNumberOfPoints() != 6)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(triangle, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that adjacent edges are touching
  if (!ContiguousEdges(triangle, tolerance))
  {
    state |= State::NoncontiguousEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkQuadraticQuad* quad, double tolerance)
{
  State state = State::Valid;

  // Ensure there are eight underlying point ids for the quad
  if (quad->GetNumberOfPoints() != 8)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(quad, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that adjacent edges are touching
  if (!ContiguousEdges(quad, tolerance))
  {
    state |= State::NoncontiguousEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkQuadraticPolygon* polygon, double tolerance)
{
  State state = State::Valid;

  // Ensure there are at least six underlying point ids for the polygon
  if (polygon->GetNumberOfPoints() < 6)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(polygon, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that adjacent edges are touching
  if (!ContiguousEdges(polygon, tolerance))
  {
    state |= State::NoncontiguousEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkQuadraticTetra* tetra, double tolerance)
{
  State state = State::Valid;

  // Ensure there are ten underlying point ids for the tetra
  if (tetra->GetNumberOfPoints() != 10)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(tetra, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(tetra, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the tetra's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(tetra, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkQuadraticHexahedron* hex, double tolerance)
{
  State state = State::Valid;

  // Ensure there are twenty underlying point ids for the hex
  if (hex->GetNumberOfPoints() != 20)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(hex, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(hex, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the hexahedron's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(hex, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkQuadraticWedge* wedge, double tolerance)
{
  State state = State::Valid;

  // Ensure there are fifteen underlying point ids for the wedge
  if (wedge->GetNumberOfPoints() != 15)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(wedge, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(wedge, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the wedge's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(wedge, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkQuadraticPyramid* pyramid, double tolerance)
{
  State state = State::Valid;

  // Ensure there are thirteen underlying point ids for the pyramid
  if (pyramid->GetNumberOfPoints() != 13)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(pyramid, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(pyramid, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure the wedge's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(pyramid, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkBiQuadraticQuad* quad, double tolerance)
{
  State state = State::Valid;

  // Ensure there are nine underlying point ids for the quad
  if (quad->GetNumberOfPoints() != 9)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(quad, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that adjacent edges are touching
  if (!ContiguousEdges(quad, tolerance))
  {
    state |= State::NoncontiguousEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkTriQuadraticHexahedron* hex, double tolerance)
{
  State state = State::Valid;

  // Ensure there are twenty-seven underlying point ids for the hex
  if (hex->GetNumberOfPoints() != 27)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(hex, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(hex, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the hexahedron's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(hex, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkQuadraticLinearQuad* quad, double tolerance)
{
  State state = State::Valid;

  // Ensure there are six underlying point ids for the quad
  if (quad->GetNumberOfPoints() != 6)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(quad, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that adjacent edges are touching
  if (!ContiguousEdges(quad, tolerance))
  {
    state |= State::NoncontiguousEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkQuadraticLinearWedge* wedge, double tolerance)
{
  State state = State::Valid;

  // Ensure there are twelve underlying point ids for the wedge
  if (wedge->GetNumberOfPoints() != 12)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(wedge, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(wedge, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the wedge's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(wedge, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(
  vtkBiQuadraticQuadraticWedge* wedge, double tolerance)
{
  State state = State::Valid;

  // Ensure there are eighteen underlying point ids for the wedge
  if (wedge->GetNumberOfPoints() != 18)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(wedge, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(wedge, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the wedge's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(wedge, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(
  vtkBiQuadraticQuadraticHexahedron* hex, double tolerance)
{
  State state = State::Valid;

  // Ensure there are twenty-four underlying point ids for the hex
  if (hex->GetNumberOfPoints() != 24)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(hex, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(hex, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the hexahedron's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(hex, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkBiQuadraticTriangle* triangle, double tolerance)
{
  State state = State::Valid;

  // Ensure there are seven underlying point ids for the triangle
  if (triangle->GetNumberOfPoints() != 7)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(triangle, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that adjacent edges are touching
  if (!ContiguousEdges(triangle, tolerance))
  {
    state |= State::NoncontiguousEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkCubicLine* line, double vtkNotUsed(tolerance))
{
  State state = State::Valid;

  // Ensure there are four underlying point ids for the edge
  if (line->GetNumberOfPoints() != 4)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkConvexPointSet* pointSet, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the point set
  if (pointSet->GetNumberOfPoints() < 1)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that the point set is convex
  if (!Convex(pointSet, tolerance))
  {
    state |= State::Nonconvex;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkPolyhedron* polyhedron, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the polyhedron
  if (polyhedron->GetNumberOfPoints() < 1)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(polyhedron, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(polyhedron, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure that the polyhedron is convex
  if (!Convex(polyhedron, tolerance))
  {
    state |= State::Nonconvex;
  }

  // Ensure the polyhedron's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(polyhedron, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkLagrangeCurve* curve, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the curve
  if (curve->GetNumberOfPoints() < 2)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(curve, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkLagrangeTriangle* triangle, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the triangle
  if (triangle->GetNumberOfPoints() < 3)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(triangle, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(triangle, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(
  vtkLagrangeQuadrilateral* quadrilateral, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the quadrilateral
  if (quadrilateral->GetNumberOfPoints() < 4)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(quadrilateral, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(quadrilateral, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkLagrangeTetra* tetrahedron, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the tetrahedron
  if (tetrahedron->GetNumberOfPoints() < 4)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(tetrahedron, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(tetrahedron, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the tetrahedron's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(tetrahedron, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkLagrangeHexahedron* hexahedron, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the hexahedron
  if (hexahedron->GetNumberOfPoints() < 8)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(hexahedron, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(hexahedron, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the hexahedron's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(hexahedron, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkLagrangeWedge* wedge, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the wedge
  if (wedge->GetNumberOfPoints() < 8)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(wedge, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(wedge, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the wedge's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(wedge, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkBezierCurve* curve, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the curve
  if (curve->GetNumberOfPoints() < 2)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(curve, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkBezierTriangle* triangle, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the triangle
  if (triangle->GetNumberOfPoints() < 3)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(triangle, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(triangle, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(
  vtkBezierQuadrilateral* quadrilateral, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the quadrilateral
  if (quadrilateral->GetNumberOfPoints() < 4)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(quadrilateral, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(quadrilateral, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkBezierTetra* tetrahedron, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the tetrahedron
  if (tetrahedron->GetNumberOfPoints() < 4)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(tetrahedron, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(tetrahedron, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the tetrahedron's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(tetrahedron, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkBezierHexahedron* hexahedron, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the hexahedron
  if (hexahedron->GetNumberOfPoints() < 8)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(hexahedron, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(hexahedron, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the hexahedron's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(hexahedron, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
vtkCellValidator::State vtkCellValidator::Check(vtkBezierWedge* wedge, double tolerance)
{
  State state = State::Valid;

  // Ensure there are enough underlying point ids for the wedge
  if (wedge->GetNumberOfPoints() < 8)
  {
    state |= State::WrongNumberOfPoints;
    return state;
  }

  // Ensure that no edges intersect
  if (!NoIntersectingEdges(wedge, tolerance))
  {
    state |= State::IntersectingEdges;
  }

  // Ensure that no faces intersect
  if (!NoIntersectingFaces(wedge, tolerance))
  {
    state |= State::IntersectingFaces;
  }

  // Ensure the wedge's faces are oriented correctly
  if (!FacesAreOrientedCorrectly(wedge, tolerance))
  {
    state |= State::FacesAreOrientedIncorrectly;
  }

  return state;
}

//----------------------------------------------------------------------------
int vtkCellValidator::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // copy the input to the output as a starting point
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  vtkNew<vtkShortArray> stateArray;
  stateArray->SetNumberOfComponents(1);
  stateArray->SetName("ValidityState"); // set the name of the value
  stateArray->SetNumberOfTuples(input->GetNumberOfCells());

  vtkGenericCell* cell = vtkGenericCell::New();
  vtkCellIterator* it = input->NewCellIterator();
  vtkIdType counter = 0;
  State state;
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    it->GetCell(cell);
    state = Check(cell, this->Tolerance);
    stateArray->SetValue(counter, static_cast<short>(state));
    if (state != State::Valid)
    {
      std::stringstream s;
      cell->Print(s);
      this->PrintState(state, s, vtkIndent(0));
      vtkOutputWindowDisplayText(s.str().c_str());
    }
    ++counter;
  }
  cell->Delete();
  it->Delete();

  output->GetCellData()->AddArray(stateArray.GetPointer());

  return 1;
}

//----------------------------------------------------------------------------
void vtkCellValidator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkCellValidator::PrintState(vtkCellValidator::State state, ostream& os, vtkIndent indent)
{
  if (state == State::Valid)
  {
    os << indent << "Cell is valid.\n";
  }
  else
  {
    os << indent << "Cell is invalid for the following reason(s):\n";

    if ((state & vtkCellValidator::State::WrongNumberOfPoints) ==
      vtkCellValidator::State::WrongNumberOfPoints)
    {
      os << indent << "  - Wrong number of points\n";
    }
    if ((state & vtkCellValidator::State::IntersectingEdges) ==
      vtkCellValidator::State::IntersectingEdges)
    {
      os << indent << "  - Intersecting edges\n";
    }
    if ((state & vtkCellValidator::State::NoncontiguousEdges) ==
      vtkCellValidator::State::NoncontiguousEdges)
    {
      os << indent << "  - Noncontiguous edges\n";
    }
    if ((state & vtkCellValidator::State::Nonconvex) == vtkCellValidator::State::Nonconvex)
    {
      os << indent << "  - Nonconvex\n";
    }
    if ((state & vtkCellValidator::State::FacesAreOrientedIncorrectly) ==
      vtkCellValidator::State::FacesAreOrientedIncorrectly)
    {
      os << indent << "  - Faces are oriented incorrectly\n";
    }
  }
}
