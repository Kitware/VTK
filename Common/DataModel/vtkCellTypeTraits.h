// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @file vtkCellTypeTraits.h
 * @brief The metadata of the cell types of vtkCellType.h, known at compile time.
 *
 * vtkCellTypeTraits<CellType> gives, for a cell type of vtkCellType.h, e.g., VTK_HEXAHEDRON:
 *
 * - CellType: the cell type;
 * - CellClass: the class of its cells, e.g., vtkHexahedron, only declared by this header, or void
 *   if it has none;
 * - ClassName and Name: the name of its class and its display name, see vtkCellTypeUtilities, or
 *   nullptr if it has none;
 * - Dimension and IsLinear: the dimension of its cells, and whether they are linear, see
 *   vtkCell::IsLinear();
 * - HasFixedSize: whether all its cells have the same number of points;
 * - NumberOfPoints and NumberOfCorners: the number of points of its cells, and of their corners,
 *   i.e., without the mid-edge, mid-face and mid-cell points of the nonlinear cells;
 * - NumberOfEdges and NumberOfFaces: see vtkCell::GetNumberOfEdges() and
 *   vtkCell::GetNumberOfFaces();
 * - MinimumEdgeSize, MaximumEdgeSize, MinimumFaceSize and MaximumFaceSize: the number of points of
 *   its smallest and largest edge and face, 0 if it has none, e.g., 2 and 3 for the edges of
 *   VTK_QUADRATIC_LINEAR_QUAD;
 * - MaximumFaceCorners and MinimumFaceCorners: the number of corners of its faces with the most
 *   and the fewest corners, 0 if it has no face, e.g., 4 and 3 for VTK_WEDGE;
 * - MaximumValence: the maximum number of its edges incident to one of its corners, 0 if it has no
 *   edge, e.g., 3 for VTK_HEXAHEDRON and 4 for VTK_PYRAMID.
 *
 * The sizes are -1 when they depend on the cell: all of them for, e.g., VTK_POLYGON; the number of
 * points and the sizes of the edges and faces for the higher order cells, whose corners, edges and
 * faces are known; all but the number of edges, 0, and the corners of the faces, 3, for
 * VTK_CONVEX_POINT_SET.
 *
 * vtkForEachCellType() calls a functor with the traits of every cell type, e.g., to build lookup
 * tables at compile time, as vtkCellTypeUtilities does.
 *
 * @code{.cpp}
 * static_assert(vtkCellTypeTraits<VTK_HEXAHEDRON>::NumberOfFaces == 6);
 * using Traits = vtkCellTypeTraits<VTK_QUADRATIC_TETRA>;
 * static_assert(Traits::NumberOfPoints == 10 && Traits::NumberOfCorners == 4);
 * @endcode
 *
 * The deprecated parametric cell types, which are no longer used, have no traits.
 *
 * @sa vtkCellType.h vtkCellTypeUtilities vtkCell
 */

#ifndef vtkCellTypeTraits_h
#define vtkCellTypeTraits_h

#include "vtkABINamespace.h"
#include "vtkCellType.h"
#include "vtkTypeList.h" // For vtkTypeList

#include <utility> // For std::forward

VTK_ABI_NAMESPACE_BEGIN
class vtkBezierCurve;
class vtkBezierHexahedron;
class vtkBezierQuadrilateral;
class vtkBezierTetra;
class vtkBezierTriangle;
class vtkBezierWedge;
class vtkBiQuadraticQuad;
class vtkBiQuadraticQuadraticHexahedron;
class vtkBiQuadraticQuadraticWedge;
class vtkBiQuadraticTriangle;
class vtkConvexPointSet;
class vtkCubicLine;
class vtkEmptyCell;
class vtkHexagonalPrism;
class vtkHexahedron;
class vtkHigherOrderCurve;
class vtkHigherOrderHexahedron;
class vtkHigherOrderQuadrilateral;
class vtkHigherOrderTetra;
class vtkHigherOrderTriangle;
class vtkHigherOrderWedge;
class vtkLagrangeCurve;
class vtkLagrangeHexahedron;
class vtkLagrangeQuadrilateral;
class vtkLagrangeTetra;
class vtkLagrangeTriangle;
class vtkLagrangeWedge;
class vtkLine;
class vtkPentagonalPrism;
class vtkPixel;
class vtkPolyLine;
class vtkPolyVertex;
class vtkPolygon;
class vtkPolyhedron;
class vtkPyramid;
class vtkQuad;
class vtkQuadraticEdge;
class vtkQuadraticHexahedron;
class vtkQuadraticLinearQuad;
class vtkQuadraticLinearWedge;
class vtkQuadraticPolygon;
class vtkQuadraticPyramid;
class vtkQuadraticQuad;
class vtkQuadraticTetra;
class vtkQuadraticTriangle;
class vtkQuadraticWedge;
class vtkTetra;
class vtkTriQuadraticHexahedron;
class vtkTriQuadraticPyramid;
class vtkTriangle;
class vtkTriangleStrip;
class vtkVertex;
class vtkVoxel;
class vtkWedge;

/**
 * The traits of the cell type CellType, see the file description.
 */
template <int CellType>
struct vtkCellTypeTraits;

#define vtkCellTypeTraitsMacro(type, cellClass, className, name, dimension, isLinear, points,      \
  corners, edges, faces, minEdgeSize, maxEdgeSize, minFaceSize, maxFaceSize, maxFaceCorners,       \
  minFaceCorners, maxValence)                                                                      \
  template <>                                                                                      \
  struct vtkCellTypeTraits<type>                                                                   \
  {                                                                                                \
    static constexpr int CellType = type;                                                          \
    using CellClass = cellClass;                                                                   \
    static constexpr const char* ClassName = className;                                            \
    static constexpr const char* Name = name;                                                      \
    static constexpr int Dimension = dimension;                                                    \
    static constexpr bool IsLinear = isLinear;                                                     \
    static constexpr bool HasFixedSize = (points) >= 0;                                            \
    static constexpr int NumberOfPoints = points;                                                  \
    static constexpr int NumberOfCorners = corners;                                                \
    static constexpr int NumberOfEdges = edges;                                                    \
    static constexpr int NumberOfFaces = faces;                                                    \
    static constexpr int MinimumEdgeSize = minEdgeSize;                                            \
    static constexpr int MaximumEdgeSize = maxEdgeSize;                                            \
    static constexpr int MinimumFaceSize = minFaceSize;                                            \
    static constexpr int MaximumFaceSize = maxFaceSize;                                            \
    static constexpr int MaximumFaceCorners = maxFaceCorners;                                      \
    static constexpr int MinimumFaceCorners = minFaceCorners;                                      \
    static constexpr int MaximumValence = maxValence;                                              \
  }

// The traits of the cell types with a fixed size, of those without, whose sizes are -1, and of the
// higher order ones, whose number of points and sizes of edges and faces depend on their order.
#define vtkFixedCellTypeTraitsMacro(type, cellClass, name, dimension, isLinear, points, corners,   \
  edges, faces, minEdgeSize, maxEdgeSize, minFaceSize, maxFaceSize, maxFaceCorners,                \
  minFaceCorners, maxValence)                                                                      \
  vtkCellTypeTraitsMacro(type, cellClass, #cellClass, name, dimension, isLinear, points, corners,  \
    edges, faces, minEdgeSize, maxEdgeSize, minFaceSize, maxFaceSize, maxFaceCorners,              \
    minFaceCorners, maxValence)
#define vtkVariableCellTypeTraitsMacro(type, cellClass, name, dimension, isLinear)                 \
  vtkCellTypeTraitsMacro(type, cellClass, #cellClass, name, dimension, isLinear, -1, -1, -1, -1,   \
    -1, -1, -1, -1, -1, -1, -1)
#define vtkHigherOrderCellTypeTraitsMacro(type, cellClass, name, dimension, corners, edges, faces, \
  maxFaceCorners, minFaceCorners, maxValence)                                                      \
  vtkCellTypeTraitsMacro(type, cellClass, #cellClass, name, dimension, false, -1, corners, edges,  \
    faces, -1, -1, -1, -1, maxFaceCorners, minFaceCorners, maxValence)

// The cell types in the order of vtkCellType.h. The arguments of a fixed size cell type are: its
// cell type, class, display name, dimension, linearity, number of points, corners, edges and
// faces, minimum and maximum edge and face size, maximum and minimum number of corners of its
// faces, and maximum valence.
// Linear cells
vtkFixedCellTypeTraitsMacro(
  VTK_EMPTY_CELL, vtkEmptyCell, "Empty Cell", 0, true, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
vtkFixedCellTypeTraitsMacro(
  VTK_VERTEX, vtkVertex, "Vertex", 0, true, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
vtkVariableCellTypeTraitsMacro(VTK_POLY_VERTEX, vtkPolyVertex, "Polyvertex", 0, true);
vtkFixedCellTypeTraitsMacro(VTK_LINE, vtkLine, "Line", 1, true, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0);
vtkVariableCellTypeTraitsMacro(VTK_POLY_LINE, vtkPolyLine, "Polyline", 1, true);
vtkFixedCellTypeTraitsMacro(
  VTK_TRIANGLE, vtkTriangle, "Triangle", 2, true, 3, 3, 3, 0, 2, 2, 0, 0, 0, 0, 2);
vtkVariableCellTypeTraitsMacro(VTK_TRIANGLE_STRIP, vtkTriangleStrip, "Triangle Strip", 2, true);
vtkVariableCellTypeTraitsMacro(VTK_POLYGON, vtkPolygon, "Polygon", 2, true);
vtkFixedCellTypeTraitsMacro(VTK_PIXEL, vtkPixel, "Pixel", 2, true, 4, 4, 4, 0, 2, 2, 0, 0, 0, 0, 2);
vtkFixedCellTypeTraitsMacro(
  VTK_QUAD, vtkQuad, "Quadrilateral", 2, true, 4, 4, 4, 0, 2, 2, 0, 0, 0, 0, 2);
vtkFixedCellTypeTraitsMacro(
  VTK_TETRA, vtkTetra, "Tetrahedron", 3, true, 4, 4, 6, 4, 2, 2, 3, 3, 3, 3, 3);
vtkFixedCellTypeTraitsMacro(
  VTK_VOXEL, vtkVoxel, "Voxel", 3, true, 8, 8, 12, 6, 2, 2, 4, 4, 4, 4, 3);
vtkFixedCellTypeTraitsMacro(
  VTK_HEXAHEDRON, vtkHexahedron, "Hexahedron", 3, true, 8, 8, 12, 6, 2, 2, 4, 4, 4, 4, 3);
vtkFixedCellTypeTraitsMacro(VTK_WEDGE, vtkWedge, "Wedge", 3, true, 6, 6, 9, 5, 2, 2, 3, 4, 4, 3, 3);
vtkFixedCellTypeTraitsMacro(
  VTK_PYRAMID, vtkPyramid, "Pyramid", 3, true, 5, 5, 8, 5, 2, 2, 3, 4, 4, 3, 4);
vtkFixedCellTypeTraitsMacro(VTK_PENTAGONAL_PRISM, vtkPentagonalPrism, "Pentagonal Prism", 3, true,
  10, 10, 15, 7, 2, 2, 4, 5, 5, 4, 3);
vtkFixedCellTypeTraitsMacro(VTK_HEXAGONAL_PRISM, vtkHexagonalPrism, "Hexagonal Prism", 3, true, 12,
  12, 18, 8, 2, 2, 4, 6, 6, 4, 3);
// Quadratic, isoparametric cells
vtkFixedCellTypeTraitsMacro(VTK_QUADRATIC_EDGE, vtkQuadraticEdge, "Quadratic Edge", 1, false, 3, 2,
  0, 0, 0, 0, 0, 0, 0, 0, 0);
vtkFixedCellTypeTraitsMacro(VTK_QUADRATIC_TRIANGLE, vtkQuadraticTriangle, "Quadratic Triangle", 2,
  false, 6, 3, 3, 0, 3, 3, 0, 0, 0, 0, 2);
vtkFixedCellTypeTraitsMacro(VTK_QUADRATIC_QUAD, vtkQuadraticQuad, "Quadratic Quadrilateral", 2,
  false, 8, 4, 4, 0, 3, 3, 0, 0, 0, 0, 2);
vtkVariableCellTypeTraitsMacro(
  VTK_QUADRATIC_POLYGON, vtkQuadraticPolygon, "Quadratic Polygon", 2, false);
vtkFixedCellTypeTraitsMacro(VTK_QUADRATIC_TETRA, vtkQuadraticTetra, "Quadratic Tetrahedron", 3,
  false, 10, 4, 6, 4, 3, 3, 6, 6, 3, 3, 3);
vtkFixedCellTypeTraitsMacro(VTK_QUADRATIC_HEXAHEDRON, vtkQuadraticHexahedron,
  "Quadratic Hexahedron", 3, false, 20, 8, 12, 6, 3, 3, 8, 8, 4, 4, 3);
vtkFixedCellTypeTraitsMacro(VTK_QUADRATIC_WEDGE, vtkQuadraticWedge, "Quadratic Wedge", 3, false, 15,
  6, 9, 5, 3, 3, 6, 8, 4, 3, 3);
vtkFixedCellTypeTraitsMacro(VTK_QUADRATIC_PYRAMID, vtkQuadraticPyramid, "Quadratic Pyramid", 3,
  false, 13, 5, 8, 5, 3, 3, 6, 8, 4, 3, 4);
vtkFixedCellTypeTraitsMacro(VTK_BIQUADRATIC_QUAD, vtkBiQuadraticQuad, "Bi-Quadratic Quadrilateral",
  2, false, 9, 4, 4, 0, 3, 3, 0, 0, 0, 0, 2);
vtkFixedCellTypeTraitsMacro(VTK_TRIQUADRATIC_HEXAHEDRON, vtkTriQuadraticHexahedron,
  "Tri-Quadratic Hexahedron", 3, false, 27, 8, 12, 6, 3, 3, 9, 9, 4, 4, 3);
vtkFixedCellTypeTraitsMacro(VTK_TRIQUADRATIC_PYRAMID, vtkTriQuadraticPyramid,
  "Tri-Quadratic Pyramid", 3, false, 19, 5, 8, 5, 3, 3, 7, 9, 4, 3, 4);
vtkFixedCellTypeTraitsMacro(VTK_QUADRATIC_LINEAR_QUAD, vtkQuadraticLinearQuad,
  "Quadratic Linear Quadrilateral", 2, false, 6, 4, 4, 0, 2, 3, 0, 0, 0, 0, 2);
vtkFixedCellTypeTraitsMacro(VTK_QUADRATIC_LINEAR_WEDGE, vtkQuadraticLinearWedge,
  "Quadratic Linear Wedge", 3, false, 12, 6, 9, 5, 2, 3, 6, 6, 4, 3, 3);
vtkFixedCellTypeTraitsMacro(VTK_BIQUADRATIC_QUADRATIC_WEDGE, vtkBiQuadraticQuadraticWedge,
  "Bi-Quadratic Quadratic Wedge", 3, false, 18, 6, 9, 5, 3, 3, 6, 9, 4, 3, 3);
vtkFixedCellTypeTraitsMacro(VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON, vtkBiQuadraticQuadraticHexahedron,
  "Bi-Quadratic Quadratic Hexahedron", 3, false, 24, 8, 12, 6, 3, 3, 8, 9, 4, 4, 3);
vtkFixedCellTypeTraitsMacro(VTK_BIQUADRATIC_TRIANGLE, vtkBiQuadraticTriangle,
  "Bi-Quadratic Triangle", 2, false, 7, 3, 3, 0, 3, 3, 0, 0, 0, 0, 2);
// Cubic, isoparametric cell
vtkFixedCellTypeTraitsMacro(
  VTK_CUBIC_LINE, vtkCubicLine, "Cubic Line", 1, false, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0);
// Special class of cells formed by convex group of points, which have no edges.
vtkCellTypeTraitsMacro(VTK_CONVEX_POINT_SET, vtkConvexPointSet, "vtkConvexPointSet",
  "Convex Pointset", 3, true, -1, -1, 0, -1, -1, -1, -1, -1, 3, 3, 0);
// Polyhedron cell (consisting of polygonal faces)
vtkVariableCellTypeTraitsMacro(VTK_POLYHEDRON, vtkPolyhedron, "Polyhedron", 3, true);
// Higher order cells, whose pyramid is not implemented
vtkHigherOrderCellTypeTraitsMacro(
  VTK_HIGHER_ORDER_CURVE, vtkHigherOrderCurve, "Higher Order Curve", 1, 2, 0, 0, 0, 0, 0);
vtkHigherOrderCellTypeTraitsMacro(
  VTK_HIGHER_ORDER_TRIANGLE, vtkHigherOrderTriangle, "Higher Order Triangle", 2, 3, 3, 0, 0, 0, 2);
vtkHigherOrderCellTypeTraitsMacro(VTK_HIGHER_ORDER_QUADRILATERAL, vtkHigherOrderQuadrilateral,
  "Higher Order Quadrilateral", 2, 4, 4, 0, 0, 0, 2);
vtkHigherOrderCellTypeTraitsMacro(VTK_HIGHER_ORDER_TETRAHEDRON, vtkHigherOrderTetra,
  "Higher Order Tetrahedron", 3, 4, 6, 4, 3, 3, 3);
vtkHigherOrderCellTypeTraitsMacro(
  VTK_HIGHER_ORDER_WEDGE, vtkHigherOrderWedge, "Higher Order Wedge", 3, 6, 9, 5, 4, 3, 3);
vtkCellTypeTraitsMacro(
  VTK_HIGHER_ORDER_PYRAMID, void, nullptr, nullptr, 3, false, -1, 5, 8, 5, -1, -1, -1, -1, 4, 3, 4);
vtkHigherOrderCellTypeTraitsMacro(VTK_HIGHER_ORDER_HEXAHEDRON, vtkHigherOrderHexahedron,
  "Higher Order Hexahedron", 3, 8, 12, 6, 4, 4, 3);
// Arbitrary order Lagrange elements, whose pyramid is not implemented
vtkHigherOrderCellTypeTraitsMacro(
  VTK_LAGRANGE_CURVE, vtkLagrangeCurve, "Lagrange Curve", 1, 2, 0, 0, 0, 0, 0);
vtkHigherOrderCellTypeTraitsMacro(
  VTK_LAGRANGE_TRIANGLE, vtkLagrangeTriangle, "Lagrange Triangle", 2, 3, 3, 0, 0, 0, 2);
vtkHigherOrderCellTypeTraitsMacro(VTK_LAGRANGE_QUADRILATERAL, vtkLagrangeQuadrilateral,
  "Lagrange Quadrilateral", 2, 4, 4, 0, 0, 0, 2);
vtkHigherOrderCellTypeTraitsMacro(
  VTK_LAGRANGE_TETRAHEDRON, vtkLagrangeTetra, "Lagrange Tetrahedron", 3, 4, 6, 4, 3, 3, 3);
vtkHigherOrderCellTypeTraitsMacro(
  VTK_LAGRANGE_HEXAHEDRON, vtkLagrangeHexahedron, "Lagrange Hexahedron", 3, 8, 12, 6, 4, 4, 3);
vtkHigherOrderCellTypeTraitsMacro(
  VTK_LAGRANGE_WEDGE, vtkLagrangeWedge, "Lagrange Wedge", 3, 6, 9, 5, 4, 3, 3);
vtkCellTypeTraitsMacro(
  VTK_LAGRANGE_PYRAMID, void, nullptr, nullptr, 3, false, -1, 5, 8, 5, -1, -1, -1, -1, 4, 3, 4);
// Arbitrary order Bezier elements, whose pyramid is not implemented
vtkHigherOrderCellTypeTraitsMacro(
  VTK_BEZIER_CURVE, vtkBezierCurve, "Bezier Curve", 1, 2, 0, 0, 0, 0, 0);
vtkHigherOrderCellTypeTraitsMacro(
  VTK_BEZIER_TRIANGLE, vtkBezierTriangle, "Bezier Triangle", 2, 3, 3, 0, 0, 0, 2);
vtkHigherOrderCellTypeTraitsMacro(
  VTK_BEZIER_QUADRILATERAL, vtkBezierQuadrilateral, "Bezier Quadrilateral", 2, 4, 4, 0, 0, 0, 2);
vtkHigherOrderCellTypeTraitsMacro(
  VTK_BEZIER_TETRAHEDRON, vtkBezierTetra, "Bezier Tetrahedron", 3, 4, 6, 4, 3, 3, 3);
vtkHigherOrderCellTypeTraitsMacro(
  VTK_BEZIER_HEXAHEDRON, vtkBezierHexahedron, "Bezier Hexahedron", 3, 8, 12, 6, 4, 4, 3);
vtkHigherOrderCellTypeTraitsMacro(
  VTK_BEZIER_WEDGE, vtkBezierWedge, "Bezier Wedge", 3, 6, 9, 5, 4, 3, 3);
vtkCellTypeTraitsMacro(
  VTK_BEZIER_PYRAMID, void, nullptr, nullptr, 3, false, -1, 5, 8, 5, -1, -1, -1, -1, 4, 3, 4);

#undef vtkHigherOrderCellTypeTraitsMacro
#undef vtkVariableCellTypeTraitsMacro
#undef vtkFixedCellTypeTraitsMacro
#undef vtkCellTypeTraitsMacro

namespace vtkCellTypeTraits_detail
{
// The traits of the cell types, in the order of vtkCellType.h.
using AllCellTypeTraits = vtkTypeList::Create<vtkCellTypeTraits<VTK_EMPTY_CELL>,
  vtkCellTypeTraits<VTK_VERTEX>, vtkCellTypeTraits<VTK_POLY_VERTEX>, vtkCellTypeTraits<VTK_LINE>,
  vtkCellTypeTraits<VTK_POLY_LINE>, vtkCellTypeTraits<VTK_TRIANGLE>,
  vtkCellTypeTraits<VTK_TRIANGLE_STRIP>, vtkCellTypeTraits<VTK_POLYGON>,
  vtkCellTypeTraits<VTK_PIXEL>, vtkCellTypeTraits<VTK_QUAD>, vtkCellTypeTraits<VTK_TETRA>,
  vtkCellTypeTraits<VTK_VOXEL>, vtkCellTypeTraits<VTK_HEXAHEDRON>, vtkCellTypeTraits<VTK_WEDGE>,
  vtkCellTypeTraits<VTK_PYRAMID>, vtkCellTypeTraits<VTK_PENTAGONAL_PRISM>,
  vtkCellTypeTraits<VTK_HEXAGONAL_PRISM>, vtkCellTypeTraits<VTK_QUADRATIC_EDGE>,
  vtkCellTypeTraits<VTK_QUADRATIC_TRIANGLE>, vtkCellTypeTraits<VTK_QUADRATIC_QUAD>,
  vtkCellTypeTraits<VTK_QUADRATIC_POLYGON>, vtkCellTypeTraits<VTK_QUADRATIC_TETRA>,
  vtkCellTypeTraits<VTK_QUADRATIC_HEXAHEDRON>, vtkCellTypeTraits<VTK_QUADRATIC_WEDGE>,
  vtkCellTypeTraits<VTK_QUADRATIC_PYRAMID>, vtkCellTypeTraits<VTK_BIQUADRATIC_QUAD>,
  vtkCellTypeTraits<VTK_TRIQUADRATIC_HEXAHEDRON>, vtkCellTypeTraits<VTK_TRIQUADRATIC_PYRAMID>,
  vtkCellTypeTraits<VTK_QUADRATIC_LINEAR_QUAD>, vtkCellTypeTraits<VTK_QUADRATIC_LINEAR_WEDGE>,
  vtkCellTypeTraits<VTK_BIQUADRATIC_QUADRATIC_WEDGE>,
  vtkCellTypeTraits<VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON>,
  vtkCellTypeTraits<VTK_BIQUADRATIC_TRIANGLE>, vtkCellTypeTraits<VTK_CUBIC_LINE>,
  vtkCellTypeTraits<VTK_CONVEX_POINT_SET>, vtkCellTypeTraits<VTK_POLYHEDRON>,
  vtkCellTypeTraits<VTK_HIGHER_ORDER_CURVE>, vtkCellTypeTraits<VTK_HIGHER_ORDER_TRIANGLE>,
  vtkCellTypeTraits<VTK_HIGHER_ORDER_QUADRILATERAL>,
  vtkCellTypeTraits<VTK_HIGHER_ORDER_TETRAHEDRON>, vtkCellTypeTraits<VTK_HIGHER_ORDER_WEDGE>,
  vtkCellTypeTraits<VTK_HIGHER_ORDER_PYRAMID>, vtkCellTypeTraits<VTK_HIGHER_ORDER_HEXAHEDRON>,
  vtkCellTypeTraits<VTK_LAGRANGE_CURVE>, vtkCellTypeTraits<VTK_LAGRANGE_TRIANGLE>,
  vtkCellTypeTraits<VTK_LAGRANGE_QUADRILATERAL>, vtkCellTypeTraits<VTK_LAGRANGE_TETRAHEDRON>,
  vtkCellTypeTraits<VTK_LAGRANGE_HEXAHEDRON>, vtkCellTypeTraits<VTK_LAGRANGE_WEDGE>,
  vtkCellTypeTraits<VTK_LAGRANGE_PYRAMID>, vtkCellTypeTraits<VTK_BEZIER_CURVE>,
  vtkCellTypeTraits<VTK_BEZIER_TRIANGLE>, vtkCellTypeTraits<VTK_BEZIER_QUADRILATERAL>,
  vtkCellTypeTraits<VTK_BEZIER_TETRAHEDRON>, vtkCellTypeTraits<VTK_BEZIER_HEXAHEDRON>,
  vtkCellTypeTraits<VTK_BEZIER_WEDGE>, vtkCellTypeTraits<VTK_BEZIER_PYRAMID>>;
} // namespace vtkCellTypeTraits_detail

/**
 * Call functor(vtkCellTypeTraits<CellType>{}) for every cell type with traits, in the order of
 * vtkCellType.h. It can be called at compile time, e.g., to build a lookup table:
 *
 * @code{.cpp}
 * constexpr std::array<int, VTK_NUMBER_OF_CELL_TYPES> MakeDimensions()
 * {
 *   std::array<int, VTK_NUMBER_OF_CELL_TYPES> dimensions{};
 *   vtkForEachCellType(
 *     [&](auto traits) { dimensions[decltype(traits)::CellType] = decltype(traits)::Dimension; });
 *   return dimensions;
 * }
 * @endcode
 */
template <class Functor>
constexpr void vtkForEachCellType(Functor&& functor)
{
  vtkTypeList::ForEach<vtkCellTypeTraits_detail::AllCellTypeTraits>(std::forward<Functor>(functor));
}

VTK_ABI_NAMESPACE_END

/**
 * Expose the traits of `cellType` in the public section of the class of its cells, e.g.,
 * vtkHexahedron: Traits, its vtkCellTypeTraits, and its members as static constexpr members,
 * IsLinear as IsLinearCell, since vtkCell::IsLinear() is a function, and the sizes as vtkIdType.
 */
#define vtkCellTypeTraitsMembersMacro(cellType)                                                    \
  using Traits = vtkCellTypeTraits<cellType>;                                                      \
  static constexpr int CellType = Traits::CellType;                                                \
  static constexpr int Dimension = Traits::Dimension;                                              \
  static constexpr bool IsLinearCell = Traits::IsLinear;                                           \
  static constexpr bool HasFixedSize = Traits::HasFixedSize;                                       \
  static constexpr vtkIdType NumberOfPoints = Traits::NumberOfPoints;                              \
  static constexpr vtkIdType NumberOfCorners = Traits::NumberOfCorners;                            \
  static constexpr vtkIdType NumberOfEdges = Traits::NumberOfEdges;                                \
  static constexpr vtkIdType NumberOfFaces = Traits::NumberOfFaces;                                \
  static constexpr vtkIdType MinimumEdgeSize = Traits::MinimumEdgeSize;                            \
  static constexpr vtkIdType MaximumEdgeSize = Traits::MaximumEdgeSize;                            \
  static constexpr vtkIdType MinimumFaceSize = Traits::MinimumFaceSize;                            \
  static constexpr vtkIdType MaximumFaceSize = Traits::MaximumFaceSize;                            \
  static constexpr vtkIdType MaximumFaceCorners = Traits::MaximumFaceCorners;                      \
  static constexpr vtkIdType MinimumFaceCorners = Traits::MinimumFaceCorners;                      \
  static constexpr vtkIdType MaximumValence = Traits::MaximumValence

#endif // vtkCellTypeTraits_h
// VTK-HeaderTest-Exclude: vtkCellTypeTraits.h
