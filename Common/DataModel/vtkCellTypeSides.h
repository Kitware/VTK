// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @file vtkCellTypeSides.h
 * @brief The sides of the cells of a dataset, by cell type, read without building the cells.
 *
 * The vertices, edges and faces of the cells of a dataset, e.g., a vtkUnstructuredGrid or a
 * vtkPolyData, by classes of cells with a common interface, in the order of vtkCellType.h:
 * FixedSizeCellSides (the cell types of a fixed size, read from static tables sized by
 * vtkCellTypeTraits), PointListCellSides (the poly-vertices and poly-lines),
 * TriangleStripCellSides, PolygonCellSides (the polygons and quadratic polygons),
 * ConvexPointSetSides, PolyhedronCellSides and HigherOrderCellSides. The cell types without
 * traits, which cannot be built, are empty cells, as in vtkGenericCell::SetCellType(). The sides
 * are in the order of vtkCell::GetEdge() and vtkCell::GetFace(), but the faces of a voxel, in the
 * order of a polygon.
 *
 * GetCellNumberOfSides() gives the number of sides of a dimension of a cell, GetCellSideType() the
 * cell type of one of them, GetCellSideSize() its number of points and GetCellSidePoints() its
 * points, as vtkUnstructuredGrid::GetCellEdgePoints() and GetCellFacePoints(), which use them.
 * DispatchCellSides() calls a functor with the class of sides of a cell type, so that hot loops
 * read the sides with their sizes known at compile time when possible.
 *
 * @code{.cpp}
 * vtkCellTypeSides::DispatchCellSides(cellType,
 *   [&](auto sides) VTK_ALWAYS_INLINE_LAMBDA
 *   {
 *     const int numEdges = sides.GetNumberOfSides(grid, cellId, cellType, 1, npts);
 *     ...
 *   });
 * @endcode
 *
 * The nonlinear cells are dispatched out of these loops, by NonLinearCellSides.
 *
 * The interface of the classes of sides, for the sides of dimension sideDim (0 for the vertices, 1
 * for the edges, 2 for the faces):
 * - UsesNumberOfPoints: whether GetNumberOfSides() reads the number of points of the cell;
 * - MaximumFaceCorners and MinimumFaceCorners: the most and the fewest corners of a face, known at
 *   compile time, or 0;
 * - GetDimension(): the dimension of the cells;
 * - GetNumberOfSides(): the number of sides of the cell cellId, of type cellType and npts points;
 * - GetSideType(): the cell type of the side sideId, given by the cell type only, e.g.,
 *   VTK_QUADRATIC_QUAD for a face of a VTK_QUADRATIC_HEXAHEDRON;
 * - GetSideSize(): its number of points, without reading them;
 * - GetSidePoints(): its points, from the points `pts` of the cell, in `ptIds`, of
 *   MaximumCellSidePoints points, or in `sidePts`. Returns their number, sets `ids` to them and
 *   `numCorners` to the number of its corners, its first points: all of them for a linear side,
 *   without the mid-edge and mid-face points otherwise. Two sides are the same if they have the
 *   same corners.
 *
 * The vertices of a cell are its points, mid-edge and mid-face points included. The functions take
 * the dataset of the cell and the scratch objects of the caller: `cell`, to build the cells whose
 * sides are not read from static tables, e.g., the higher order cells, and `sidePts`, for the
 * points of the sides of the built cells and of the faces of the polyhedra.
 *
 * @sa vtkCellTypeTraits
 */

#ifndef vtkCellTypeSides_h
#define vtkCellTypeSides_h

#include "vtkBiQuadraticQuad.h"
#include "vtkBiQuadraticQuadraticHexahedron.h"
#include "vtkBiQuadraticQuadraticWedge.h"
#include "vtkBiQuadraticTriangle.h"
#include "vtkCellType.h"
#include "vtkCellTypeTraits.h"
#include "vtkGenericCell.h"
#include "vtkHexagonalPrism.h"
#include "vtkHexahedron.h"
#include "vtkIdList.h"
#include "vtkPentagonalPrism.h"
#include "vtkPixel.h"
#include "vtkPolyhedronCellArray.h"
#include "vtkPyramid.h"
#include "vtkQuad.h"
#include "vtkQuadraticHexahedron.h"
#include "vtkQuadraticLinearQuad.h"
#include "vtkQuadraticLinearWedge.h"
#include "vtkQuadraticPyramid.h"
#include "vtkQuadraticQuad.h"
#include "vtkQuadraticTetra.h"
#include "vtkQuadraticTriangle.h"
#include "vtkQuadraticWedge.h"
#include "vtkSetGet.h"
#include "vtkTetra.h"
#include "vtkTriQuadraticHexahedron.h"
#include "vtkTriQuadraticPyramid.h"
#include "vtkTriangle.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace vtkCellTypeSides
{
VTK_ABI_NAMESPACE_BEGIN

// The maximum number of points of a side read from the static tables of the cells, the largest
// edge or face of the cell types of a fixed size, see vtkCellTypeTraits: a face of a tri-quadratic
// hexahedron.
constexpr int GetMaximumCellSidePoints()
{
  int maximum = 2; // an edge of a polygon
  vtkForEachCellType(
    [&maximum](auto traits)
    {
      using Traits = decltype(traits);
      if constexpr (Traits::HasFixedSize)
      {
        maximum = std::max({ maximum, Traits::MaximumEdgeSize, Traits::MaximumFaceSize });
      }
    });
  return maximum;
}
inline constexpr int MaximumCellSidePoints = GetMaximumCellSidePoints();

//------------------------------------------------------------------------------
// The cell type of a side of a cell of a fixed size, given by its number of points and of corners:
// a vertex, an edge, or a face, linear or nonlinear, e.g., a face of 8 points and 4 corners is a
// VTK_QUADRATIC_QUAD, and a face of 5 or 6 corners, of a pentagonal or hexagonal prism, is a
// VTK_POLYGON, so that the faces of a voxel, in the order of a polygon, are quads.
constexpr int GetFixedSizeSideType(int numPts, int numCorners)
{
  switch (numCorners)
  {
    case 1:
      return VTK_VERTEX;
    case 2:
      return numPts == 2 ? VTK_LINE : VTK_QUADRATIC_EDGE;
    case 3:
      switch (numPts)
      {
        case 3:
          return VTK_TRIANGLE;
        case 6:
          return VTK_QUADRATIC_TRIANGLE;
        default:
          return VTK_BIQUADRATIC_TRIANGLE;
      }
    case 4:
      switch (numPts)
      {
        case 4:
          return VTK_QUAD;
        case 6:
          return VTK_QUADRATIC_LINEAR_QUAD;
        case 8:
          return VTK_QUADRATIC_QUAD;
        default:
          return VTK_BIQUADRATIC_QUAD;
      }
    default:
      return VTK_POLYGON;
  }
}

//------------------------------------------------------------------------------
// The edges and faces of the cells of the cell type of a fixed size CellType, whose sizes are given
// by vtkCellTypeTraits, as small tables built once, since CellClass::GetEdgeArray() and
// CellClass::GetFaceArray() are not inlined: their local points, number of points, number of
// corners, which are the points of the cell of index less than its number of corners, and cell
// type, see GetFixedSizeSideType(). The rows of the tables of the cells with fewer points end
// with -1, and the faces of a voxel are in the order of a polygon instead of a pixel.
template <int CellType>
struct FixedSizeCellSideTables
{
  using Traits = vtkCellTypeTraits<CellType>;
  using CellClass = typename Traits::CellClass;
  std::array<std::array<std::uint8_t, Traits::MaximumEdgeSize>, Traits::NumberOfEdges> Edges{};
  std::array<std::uint8_t, Traits::NumberOfEdges> EdgeSizes{};
  std::array<std::uint8_t, Traits::NumberOfEdges> EdgeCorners{};
  std::array<std::uint8_t, Traits::NumberOfEdges> EdgeTypes{};
  std::array<std::array<std::uint8_t, Traits::MaximumFaceSize>, Traits::NumberOfFaces> Faces{};
  std::array<std::uint8_t, Traits::NumberOfFaces> FaceSizes{};
  std::array<std::uint8_t, Traits::NumberOfFaces> FaceCorners{};
  std::array<std::uint8_t, Traits::NumberOfFaces> FaceTypes{};

  FixedSizeCellSideTables()
  {
    if constexpr (Traits::NumberOfEdges > 0)
    {
      for (int e = 0; e < Traits::NumberOfEdges; ++e)
      {
        SetRow(CellClass::GetEdgeArray(e), Traits::MaximumEdgeSize, this->Edges[e].data(),
          this->EdgeSizes[e], this->EdgeCorners[e], this->EdgeTypes[e]);
      }
    }
    if constexpr (Traits::NumberOfFaces > 0)
    {
      for (int f = 0; f < Traits::NumberOfFaces; ++f)
      {
        SetRow(CellClass::GetFaceArray(f), Traits::MaximumFaceSize, this->Faces[f].data(),
          this->FaceSizes[f], this->FaceCorners[f], this->FaceTypes[f]);
        if constexpr (CellType == VTK_VOXEL)
        {
          std::swap(this->Faces[f][2], this->Faces[f][3]);
        }
      }
    }
  }

  static void SetRow(const vtkIdType* row, int maxSize, std::uint8_t* side, std::uint8_t& size,
    std::uint8_t& corners, std::uint8_t& type)
  {
    int n = 0;
    for (; n < maxSize && row[n] >= 0; ++n)
    {
      side[n] = static_cast<std::uint8_t>(row[n]);
    }
    int c = 0;
    while (c < n && row[c] < Traits::NumberOfCorners)
    {
      ++c;
    }
    size = static_cast<std::uint8_t>(n);
    corners = static_cast<std::uint8_t>(c);
    type = static_cast<std::uint8_t>(GetFixedSizeSideType(n, c));
  }
};

//------------------------------------------------------------------------------
// The cells of the cell type of a fixed size CellType, see vtkCellTypeTraits::HasFixedSize, e.g.,
// VTK_TETRA and VTK_QUADRATIC_HEXAHEDRON, whose sides are read from static tables, see
// FixedSizeCellSideTables. The other cells have their own classes, see DispatchCellSides().
template <int CellType>
struct FixedSizeCellSides
{
  using Traits = vtkCellTypeTraits<CellType>;
  static_assert(Traits::HasFixedSize, "only the cell types of a fixed size have static tables");
  static_assert((Traits::NumberOfEdges > 0) == (Traits::Dimension >= 2) &&
      (Traits::NumberOfFaces > 0) == (Traits::Dimension == 3),
    "the cells have edges if they are 2D or 3D, and faces if they are 3D");
  static constexpr bool UsesNumberOfPoints = false;
  static constexpr int MaximumFaceCorners = Traits::MaximumFaceCorners;
  static constexpr int MinimumFaceCorners = Traits::MinimumFaceCorners;
  // The tables are built once, when the program is loaded, so that reading them needs no guard.
  static inline const FixedSizeCellSideTables<CellType> Tables{};

  static constexpr int GetDimension(unsigned char) { return Traits::Dimension; }

  template <class DataSetT>
  static constexpr int GetNumberOfSides(
    DataSetT*, vtkIdType, unsigned char, int sideDim, vtkIdType, vtkGenericCell*)
  {
    switch (sideDim)
    {
      case 0:
        return Traits::NumberOfPoints;
      case 1:
        return Traits::NumberOfEdges;
      case 2:
      default:
        return Traits::NumberOfFaces;
    }
  }

  static int GetSideType(unsigned char, int sideDim, int sideId)
  {
    switch (sideDim)
    {
      case 0:
        return VTK_VERTEX;
      case 1:
        if constexpr (Traits::Dimension >= 2)
        {
          return Tables.EdgeTypes[sideId];
        }
        else
        {
          return VTK_EMPTY_CELL;
        }
      case 2:
      default:
        if constexpr (Traits::Dimension == 3)
        {
          return Tables.FaceTypes[sideId];
        }
        else
        {
          return VTK_EMPTY_CELL;
        }
    }
  }

  // The number of points of an edge, of a face and of its corners, known at compile time if all
  // the edges or faces have the same.
  static VTK_ALWAYS_INLINE int GetEdgeSize(int edge)
  {
    if constexpr (Traits::MinimumEdgeSize == Traits::MaximumEdgeSize)
    {
      return Traits::MaximumEdgeSize;
    }
    else
    {
      return Tables.EdgeSizes[edge];
    }
  }
  static VTK_ALWAYS_INLINE int GetFaceSize(int face)
  {
    if constexpr (Traits::MinimumFaceSize == Traits::MaximumFaceSize)
    {
      return Traits::MaximumFaceSize;
    }
    else
    {
      return Tables.FaceSizes[face];
    }
  }
  static VTK_ALWAYS_INLINE int GetFaceCorners(int face)
  {
    if constexpr (MaximumFaceCorners == MinimumFaceCorners)
    {
      return MaximumFaceCorners;
    }
    else
    {
      return Tables.FaceCorners[face];
    }
  }

  template <class DataSetT>
  static int GetSideSize(
    DataSetT*, vtkIdType, unsigned char, int sideDim, int sideId, vtkGenericCell*)
  {
    switch (sideDim)
    {
      case 0:
        return 1;
      case 1:
        if constexpr (Traits::Dimension >= 2)
        {
          return FixedSizeCellSides::GetEdgeSize(sideId);
        }
        else
        {
          return 0;
        }
      case 2:
      default:
        if constexpr (Traits::Dimension == 3)
        {
          return FixedSizeCellSides::GetFaceSize(sideId);
        }
        else
        {
          return 0;
        }
    }
  }

  template <class DataSetT, class PointsIterator>
  static vtkIdType GetSidePoints(DataSetT*, vtkIdType, unsigned char, int sideDim, int sideId,
    vtkIdType, PointsIterator pts, vtkIdType* ptIds, vtkIdList*, const vtkIdType*& ids,
    vtkIdType& numCorners, vtkGenericCell*)
  {
    ids = ptIds;
    switch (sideDim)
    {
      case 0:
        ptIds[0] = static_cast<vtkIdType>(pts[sideId]);
        return numCorners = 1;
      case 1:
        if constexpr (Traits::Dimension >= 2)
        {
          const auto& edge = Tables.Edges[sideId];
          const int numEdgePts = FixedSizeCellSides::GetEdgeSize(sideId);
          for (int k = 0; k < numEdgePts; ++k)
          {
            ptIds[k] = static_cast<vtkIdType>(pts[edge[k]]);
          }
          numCorners = 2;
          return numEdgePts;
        }
        else
        {
          return numCorners = 0;
        }
      case 2:
      default:
        if constexpr (Traits::Dimension == 3)
        {
          const auto& face = Tables.Faces[sideId];
          const int numFacePts = FixedSizeCellSides::GetFaceSize(sideId);
          for (int k = 0; k < numFacePts; ++k)
          {
            ptIds[k] = static_cast<vtkIdType>(pts[face[k]]);
          }
          numCorners = FixedSizeCellSides::GetFaceCorners(sideId);
          return numFacePts;
        }
        else
        {
          return numCorners = 0;
        }
    }
  }
};

//------------------------------------------------------------------------------
// The cells of the cell type CellType, VTK_POLY_VERTEX or VTK_POLY_LINE, whose only sides are their
// vertices, their points: a poly-line has no edges, see vtkPolyLine::GetNumberOfEdges().
template <int CellType>
struct PointListCellSides
{
  using Traits = vtkCellTypeTraits<CellType>;
  static_assert(CellType == VTK_POLY_VERTEX || CellType == VTK_POLY_LINE,
    "only the cell types whose only sides are their vertices");
  static constexpr bool UsesNumberOfPoints = true;
  static constexpr int MaximumFaceCorners = 0;
  static constexpr int MinimumFaceCorners = 0;

  static constexpr int GetDimension(unsigned char) { return Traits::Dimension; }

  template <class DataSetT>
  static constexpr int GetNumberOfSides(
    DataSetT*, vtkIdType, unsigned char, int sideDim, vtkIdType npts, vtkGenericCell*)
  {
    return sideDim == 0 ? static_cast<int>(npts) : 0;
  }

  static constexpr int GetSideType(unsigned char, int sideDim, int)
  {
    return sideDim == 0 ? VTK_VERTEX : VTK_EMPTY_CELL;
  }

  template <class DataSetT>
  static constexpr int GetSideSize(
    DataSetT*, vtkIdType, unsigned char, int sideDim, int, vtkGenericCell*)
  {
    return sideDim == 0 ? 1 : 0;
  }

  template <class DataSetT, class PointsIterator>
  static vtkIdType GetSidePoints(DataSetT*, vtkIdType, unsigned char, int sideDim, int sideId,
    vtkIdType, PointsIterator pts, vtkIdType* ptIds, vtkIdList*, const vtkIdType*& ids,
    vtkIdType& numCorners, vtkGenericCell*)
  {
    ids = ptIds;
    if (sideDim != 0)
    {
      return numCorners = 0;
    }
    ptIds[0] = static_cast<vtkIdType>(pts[sideId]);
    return numCorners = 1;
  }
};

//------------------------------------------------------------------------------
// The triangle strips, whose npts edges are their boundary, as vtkTriangleStrip::GetEdge(): the
// first edge (0, 1), then the edges (k - 1, k + 1) along both sides of the strip, and the last
// edge (npts - 2, npts - 1).
struct TriangleStripCellSides
{
  static constexpr bool UsesNumberOfPoints = true;
  static constexpr int MaximumFaceCorners = 0;
  static constexpr int MinimumFaceCorners = 0;

  static constexpr int GetDimension(unsigned char) { return 2; }

  template <class DataSetT>
  static constexpr int GetNumberOfSides(
    DataSetT*, vtkIdType, unsigned char, int sideDim, vtkIdType npts, vtkGenericCell*)
  {
    return sideDim < 2 ? static_cast<int>(npts) : 0;
  }

  static constexpr int GetSideType(unsigned char, int sideDim, int)
  {
    switch (sideDim)
    {
      case 0:
        return VTK_VERTEX;
      case 1:
        return VTK_LINE;
      case 2:
      default:
        return VTK_EMPTY_CELL;
    }
  }

  template <class DataSetT>
  static constexpr int GetSideSize(
    DataSetT*, vtkIdType, unsigned char, int sideDim, int, vtkGenericCell*)
  {
    switch (sideDim)
    {
      case 0:
        return 1;
      case 1:
        return 2;
      case 2:
      default:
        return 0;
    }
  }

  template <class DataSetT, class PointsIterator>
  static vtkIdType GetSidePoints(DataSetT*, vtkIdType, unsigned char, int sideDim, int sideId,
    vtkIdType npts, PointsIterator pts, vtkIdType* ptIds, vtkIdList*, const vtkIdType*& ids,
    vtkIdType& numCorners, vtkGenericCell*)
  {
    ids = ptIds;
    switch (sideDim)
    {
      case 0:
        ptIds[0] = static_cast<vtkIdType>(pts[sideId]);
        return numCorners = 1;
      case 1:
      {
        // The edges along the sides of the strip join every other point, between the first and the
        // last edges.
        vtkIdType first = sideId - 1;
        vtkIdType second = sideId + 1;
        if (sideId == 0)
        {
          first = 0;
          second = 1;
        }
        else if (sideId == npts - 1)
        {
          first = npts - 2;
          second = npts - 1;
        }
        ptIds[0] = static_cast<vtkIdType>(pts[first]);
        ptIds[1] = static_cast<vtkIdType>(pts[second]);
        return numCorners = 2;
      }
      case 2:
      default:
        return numCorners = 0;
    }
  }
};

//------------------------------------------------------------------------------
// The polygons, whose edges join consecutive points, and, with Quadratic, the quadratic polygons,
// see vtkQuadraticPolygon, whose first n = npts / 2 points are the corners, joined by the edges,
// and whose last n points are the mid-edge points: the edge i is (i, (i + 1) % n, i + n), a
// VTK_QUADRATIC_EDGE.
template <bool Quadratic>
struct PolygonCellSides
{
  static constexpr bool UsesNumberOfPoints = true;
  static constexpr int MaximumFaceCorners = 0;
  static constexpr int MinimumFaceCorners = 0;

  static constexpr int GetDimension(unsigned char) { return 2; }

  // The number of corners of a polygon of npts points.
  static constexpr vtkIdType GetNumberOfCorners(vtkIdType npts)
  {
    return Quadratic ? npts / 2 : npts;
  }

  template <class DataSetT>
  static constexpr int GetNumberOfSides(
    DataSetT*, vtkIdType, unsigned char, int sideDim, vtkIdType npts, vtkGenericCell*)
  {
    switch (sideDim)
    {
      case 0:
        return npts;
      case 1:
        return PolygonCellSides::GetNumberOfCorners(npts);
      case 2:
      default:
        return 0;
    }
  }

  static constexpr int GetSideType(unsigned char, int sideDim, int)
  {
    switch (sideDim)
    {
      case 0:
        return VTK_VERTEX;
      case 1:
        return Quadratic ? VTK_QUADRATIC_EDGE : VTK_LINE;
      case 2:
      default:
        return VTK_EMPTY_CELL;
    }
  }

  template <class DataSetT>
  static constexpr int GetSideSize(
    DataSetT*, vtkIdType, unsigned char, int sideDim, int, vtkGenericCell*)
  {
    switch (sideDim)
    {
      case 0:
        return 1;
      case 1:
        return Quadratic ? 3 : 2;
      case 2:
      default:
        return 0;
    }
  }

  template <class DataSetT, class PointsIterator>
  static vtkIdType GetSidePoints(DataSetT*, vtkIdType, unsigned char, int sideDim, int sideId,
    vtkIdType npts, PointsIterator pts, vtkIdType* ptIds, vtkIdList*, const vtkIdType*& ids,
    vtkIdType& numCorners, vtkGenericCell*)
  {
    ids = ptIds;
    switch (sideDim)
    {
      case 0:
        ptIds[0] = static_cast<vtkIdType>(pts[sideId]);
        return numCorners = 1;
      case 1:
      {
        const vtkIdType n = PolygonCellSides::GetNumberOfCorners(npts);
        ptIds[0] = static_cast<vtkIdType>(pts[sideId]);
        ptIds[1] = static_cast<vtkIdType>(pts[(sideId + 1) % n]);
        numCorners = 2;
        if constexpr (Quadratic)
        {
          ptIds[2] = static_cast<vtkIdType>(pts[sideId + n]);
          return 3;
        }
        else
        {
          return 2;
        }
      }
      case 2:
      default:
        return numCorners = 0;
    }
  }
};

//------------------------------------------------------------------------------
// The number of corners of a side of type sideType and numSidePts points, which are its first
// points: its points for a linear side.
constexpr vtkIdType GetNumberOfSideCorners(int sideType, vtkIdType numSidePts)
{
  switch (sideType)
  {
    case VTK_QUADRATIC_EDGE:
    case VTK_CUBIC_LINE:
    case VTK_HIGHER_ORDER_CURVE:
    case VTK_LAGRANGE_CURVE:
    case VTK_BEZIER_CURVE:
      return 2;
    case VTK_QUADRATIC_TRIANGLE:
    case VTK_BIQUADRATIC_TRIANGLE:
    case VTK_HIGHER_ORDER_TRIANGLE:
    case VTK_LAGRANGE_TRIANGLE:
    case VTK_BEZIER_TRIANGLE:
      return 3;
    case VTK_QUADRATIC_QUAD:
    case VTK_BIQUADRATIC_QUAD:
    case VTK_QUADRATIC_LINEAR_QUAD:
    case VTK_HIGHER_ORDER_QUADRILATERAL:
    case VTK_LAGRANGE_QUADRILATERAL:
    case VTK_BEZIER_QUADRILATERAL:
      return 4;
    case VTK_QUADRATIC_POLYGON:
      return numSidePts / 2;
    default:
      return numSidePts;
  }
}

//------------------------------------------------------------------------------
// The edge (sideDim 1) or face (sideDim 2) sideId of the cell cellId, built in `cell`, or nullptr.
// GetNumberOfFaces() comes first: a vtkConvexPointSet finds its faces there.
template <class DataSetT>
vtkCell* GetGenericSide(
  DataSetT* input, vtkIdType cellId, int sideDim, int sideId, vtkGenericCell* cell)
{
  // if the previous queried cell is still the same one, no need to extract the cell
  if (cellId != cell->GetCellId())
  {
    input->GetCell(cellId, cell);
  }
  switch (sideDim)
  {
    case 0:
      return nullptr;
    case 1:
      return cell->GetEdge(sideId);
    case 2:
    default:
      return cell->GetFace(sideId);
  }
}

//------------------------------------------------------------------------------
// The points of the edge or face sideId of the cell cellId, built in `cell`, copied into
// `sidePts`, as the GetSidePoints() of the classes of sides, or none if it cannot be built. Not
// inlined, to keep it out of the hot loops.
template <class DataSetT>
VTK_NEVER_INLINE vtkIdType GetGenericSidePoints(DataSetT* input, vtkIdType cellId, int sideDim,
  int sideId, vtkIdList* sidePts, const vtkIdType*& ids, vtkIdType& numCorners,
  vtkGenericCell* cell)
{
  vtkCell* side = GetGenericSide(input, cellId, sideDim, sideId, cell);
  if (!side)
  {
    ids = sidePts->GetPointer(0);
    return numCorners = 0;
  }
  // Not DeepCopy(), which squeezes the list: its memory is kept from side to side.
  vtkIdList* sideIds = side->GetPointIds();
  sidePts->SetNumberOfIds(sideIds->GetNumberOfIds());
  std::copy_n(sideIds->GetPointer(0), sideIds->GetNumberOfIds(), sidePts->GetPointer(0));
  ids = sidePts->GetPointer(0);
  const vtkIdType numSidePts = sidePts->GetNumberOfIds();
  numCorners = std::min(numSidePts, GetNumberOfSideCorners(side->GetCellType(), numSidePts));
  return numSidePts;
}

//------------------------------------------------------------------------------
// The convex point sets, which have no edges, see vtkConvexPointSet::GetNumberOfEdges(), and whose
// faces are the triangles of the triangulation of their points.
struct ConvexPointSetSides
{
  static constexpr bool UsesNumberOfPoints = true;
  static constexpr int MaximumFaceCorners = 3;
  static constexpr int MinimumFaceCorners = 3;

  static constexpr int GetDimension(unsigned char) { return 3; }

  template <class DataSetT>
  static constexpr int GetNumberOfSides(DataSetT* input, vtkIdType cellId, unsigned char,
    int sideDim, vtkIdType npts, vtkGenericCell* cell)
  {
    switch (sideDim)
    {
      case 0:
        return static_cast<int>(npts);
      case 1:
        return 0;
      case 2:
      default:
        if (cellId != cell->GetCellId())
        {
          input->GetCell(cellId, cell);
        }
        return cell->GetNumberOfFaces();
    }
  }

  static constexpr int GetSideType(unsigned char, int sideDim, int)
  {
    switch (sideDim)
    {
      case 0:
        return VTK_VERTEX;
      case 1:
        return VTK_EMPTY_CELL;
      case 2:
      default:
        return VTK_TRIANGLE;
    }
  }

  template <class DataSetT>
  static constexpr int GetSideSize(
    DataSetT*, vtkIdType, unsigned char, int sideDim, int, vtkGenericCell*)
  {
    switch (sideDim)
    {
      case 0:
        return 1;
      case 1:
        return 0;
      case 2:
      default:
        return 3;
    }
  }

  template <class DataSetT, class PointsIterator>
  static vtkIdType GetSidePoints(DataSetT* input, vtkIdType cellId, unsigned char, int sideDim,
    int sideId, vtkIdType, PointsIterator pts, vtkIdType* ptIds, vtkIdList* sidePts,
    const vtkIdType*& ids, vtkIdType& numCorners, vtkGenericCell* cell)
  {
    ids = ptIds;
    switch (sideDim)
    {
      case 0:
        ptIds[0] = static_cast<vtkIdType>(pts[sideId]);
        return numCorners = 1;
      case 1:
        return numCorners = 0;
      case 2:
      default:
        return GetGenericSidePoints(input, cellId, 2, sideId, sidePts, ids, numCorners, cell);
    }
  }
};

//------------------------------------------------------------------------------
// The points of the edge edgeId of the polyhedron cellId, in `ptIds`, read from its faces without
// building the vtkPolyhedron. The faces of a polyhedron are consistently wound and each of its
// edges is shared by two of them, see vtkPolyhedron, so that an edge is once a pair of consecutive
// points (a, b) of a face with a < b: the edge edgeId is the edgeId-th such pair, in the order of
// the faces. An edge beyond them, of a polyhedron that is not closed, has no points.
inline vtkIdType GetPolyhedronEdgePoints(vtkPolyhedronCellArray* polyhedra, vtkIdType cellId,
  int edgeId, vtkIdType* ptIds, const vtkIdType*& ids, vtkIdType& numCorners)
{
  ids = ptIds;
  vtkCellArray* faces = polyhedra->GetFaces();
  int numEdges = 0;
  const int numFaces = polyhedra->GetCellNumberOfFaces(cellId);
  for (int face = 0; face < numFaces; ++face)
  {
    const vtkIdType faceId = polyhedra->GetFaceId(cellId, face);
    const vtkIdType numFacePts = faces->GetCellSize(faceId);
    if (numFacePts == 0)
    {
      continue;
    }
    const vtkIdType firstPt = faces->GetCellPointAtId(faceId, 0);
    vtkIdType a = firstPt;
    for (vtkIdType i = 0; i < numFacePts; ++i)
    {
      const vtkIdType b = i + 1 < numFacePts ? faces->GetCellPointAtId(faceId, i + 1) : firstPt;
      if (a < b && numEdges++ == edgeId)
      {
        ptIds[0] = a;
        ptIds[1] = b;
        return numCorners = 2;
      }
      a = b;
    }
  }
  return numCorners = 0;
}

//------------------------------------------------------------------------------
// The polyhedra of a vtkUnstructuredGrid, whose faces are given by its vtkPolyhedronCellArray. The
// other datasets have no polyhedron. A polyhedron is assumed to be closed and of genus 0, as by
// vtkPolyhedron, so that its number of edges is given by Euler's formula, V - E + F = 2, from its
// number of points and of faces. Its edges are those of vtkPolyhedron, in another order, read from
// its faces, see GetPolyhedronEdgePoints(), and an edge beyond them, of a polyhedron that is not
// closed, has no points.
struct PolyhedronCellSides
{
  static constexpr bool UsesNumberOfPoints = true;
  static constexpr int MaximumFaceCorners = 0;
  static constexpr int MinimumFaceCorners = 0;

  static constexpr int GetDimension(unsigned char) { return 3; }

  template <class DataSetT>
  static int GetNumberOfSides(
    DataSetT* input, vtkIdType cellId, unsigned char, int sideDim, vtkIdType npts, vtkGenericCell*)
  {
    if constexpr (!std::is_same_v<DataSetT, vtkUnstructuredGrid>)
    {
      return 0;
    }
    else
    {
      switch (sideDim)
      {
        case 0:
          return static_cast<int>(npts);
        case 1:
          return static_cast<int>(
            input->GetPolyhedronCellArray()->GetCellNumberOfFacePoints(cellId) / 2);
        case 2:
        default:
          return input->GetPolyhedronCellArray()->GetCellNumberOfFaces(cellId);
      }
    }
  }

  static constexpr int GetSideType(unsigned char, int sideDim, int)
  {
    switch (sideDim)
    {
      case 0:
        return VTK_VERTEX;
      case 1:
        return VTK_LINE;
      case 2:
      default:
        return VTK_POLYGON;
    }
  }

  template <class DataSetT>
  static int GetSideSize(
    DataSetT* input, vtkIdType cellId, unsigned char, int sideDim, int sideId, vtkGenericCell*)
  {
    if constexpr (!std::is_same_v<DataSetT, vtkUnstructuredGrid>)
    {
      return 0;
    }
    else
    {
      switch (sideDim)
      {
        case 0:
          return 1;
        case 1:
          return 2;
        case 2:
        default:
          return static_cast<int>(input->GetPolyhedronCellArray()->GetFaceSize(cellId, sideId));
      }
    }
  }

  template <class DataSetT, class PointsIterator>
  static vtkIdType GetSidePoints(DataSetT* input, vtkIdType cellId, unsigned char, int sideDim,
    int sideId, vtkIdType, PointsIterator pts, vtkIdType* ptIds, vtkIdList* sidePts,
    const vtkIdType*& ids, vtkIdType& numCorners, vtkGenericCell*)
  {
    if constexpr (!std::is_same_v<DataSetT, vtkUnstructuredGrid>)
    {
      return numCorners = 0;
    }
    else
    {
      switch (sideDim)
      {
        case 0:
          ids = ptIds;
          ptIds[0] = static_cast<vtkIdType>(pts[sideId]);
          return numCorners = 1;
        case 1:
          return GetPolyhedronEdgePoints(
            input->GetPolyhedronCellArray(), cellId, sideId, ptIds, ids, numCorners);
        case 2:
        default:
        {
          vtkIdType numFacePts = 0;
          input->GetPolyhedronCellArray()->GetFacePoints(cellId, sideId, numFacePts, ids, sidePts);
          return numCorners = numFacePts;
        }
      }
    }
  }
};

//------------------------------------------------------------------------------
// The number of points of the edge or face sideId of the cell cellId, built in `cell`, or 0 if it
// cannot be built. Not inlined, to keep it out of the hot loops.
template <class DataSetT>
VTK_NEVER_INLINE int GetGenericSideSize(
  DataSetT* input, vtkIdType cellId, int sideDim, int sideId, vtkGenericCell* cell)
{
  vtkCell* side = GetGenericSide(input, cellId, sideDim, sideId, cell);
  return side ? static_cast<int>(side->GetNumberOfPoints()) : 0;
}

//------------------------------------------------------------------------------
// The higher order cells of the cell type CellType, the Lagrange and Bezier ones, e.g.,
// VTK_LAGRANGE_HEXAHEDRON, whose numbers and types of sides are given by their traits: their edges
// are the curves and their faces the triangles or quadrilaterals of the same family, e.g.,
// VTK_LAGRANGE_CURVE and VTK_LAGRANGE_QUADRILATERAL. The sizes and points of their sides depend on
// the order of the cell, which can differ by direction, see vtkCellData::GetHigherOrderDegrees(),
// so that they are given by the cell built in `cell`, see GetGenericSideSize() and
// GetGenericSidePoints(). The higher order cell types that cannot be built, see
// vtkGenericCell::SetCellType(), the abstract VTK_HIGHER_ORDER_ ones and the pyramids, which are
// not implemented, are empty cells, see DispatchNonLinearCellSides().
template <int CellType>
struct HigherOrderCellSides
{
  using Traits = vtkCellTypeTraits<CellType>;
  static_assert(!Traits::IsLinear && !Traits::HasFixedSize && Traits::NumberOfCorners > 0,
    "only the higher order cell types, whose corners, edges and faces are known");
  static constexpr bool UsesNumberOfPoints = true;
  static constexpr int MaximumFaceCorners = Traits::MaximumFaceCorners;
  static constexpr int MinimumFaceCorners = Traits::MinimumFaceCorners;

  // The cell types of the curves, triangles and quadrilaterals of the family of the cell type.
  static constexpr bool IsBezier = CellType >= VTK_BEZIER_CURVE && CellType <= VTK_BEZIER_PYRAMID;
  static constexpr int CurveType = IsBezier ? VTK_BEZIER_CURVE : VTK_LAGRANGE_CURVE;
  static constexpr int TriangleType = IsBezier ? VTK_BEZIER_TRIANGLE : VTK_LAGRANGE_TRIANGLE;
  static constexpr int QuadrilateralType =
    IsBezier ? VTK_BEZIER_QUADRILATERAL : VTK_LAGRANGE_QUADRILATERAL;

  static constexpr int GetDimension(unsigned char) { return Traits::Dimension; }

  template <class DataSetT>
  static constexpr int GetNumberOfSides(
    DataSetT*, vtkIdType, unsigned char, int sideDim, vtkIdType npts, vtkGenericCell*)
  {
    switch (sideDim)
    {
      case 0:
        return static_cast<int>(npts);
      case 1:
        return Traits::NumberOfEdges;
      case 2:
      default:
        return Traits::NumberOfFaces;
    }
  }

  static constexpr int GetSideType(unsigned char, int sideDim, int sideId)
  {
    switch (sideDim)
    {
      case 0:
        return VTK_VERTEX;
      case 1:
        return Traits::Dimension >= 2 ? CurveType : VTK_EMPTY_CELL;
      case 2:
      default:
        if constexpr (Traits::Dimension < 3)
        {
          return VTK_EMPTY_CELL;
        }
        else if constexpr (MinimumFaceCorners == MaximumFaceCorners)
        {
          // The faces of 3 corners are triangles, of 4 quadrilaterals.
          return MaximumFaceCorners == 3 ? TriangleType : QuadrilateralType;
        }
        else
        {
          // The faces 0 and 1 of a wedge are its triangles, see vtkLagrangeWedge::GetFace().
          return sideId < 2 ? TriangleType : QuadrilateralType;
        }
    }
  }

  template <class DataSetT>
  static int GetSideSize(
    DataSetT* input, vtkIdType cellId, unsigned char, int sideDim, int sideId, vtkGenericCell* cell)
  {
    return sideDim == 0 ? 1 : GetGenericSideSize(input, cellId, sideDim, sideId, cell);
  }

  template <class DataSetT, class PointsIterator>
  static vtkIdType GetSidePoints(DataSetT* input, vtkIdType cellId, unsigned char, int sideDim,
    int sideId, vtkIdType, PointsIterator pts, vtkIdType* ptIds, vtkIdList* sidePts,
    const vtkIdType*& ids, vtkIdType& numCorners, vtkGenericCell* cell)
  {
    if (sideDim == 0)
    {
      ids = ptIds;
      ptIds[0] = static_cast<vtkIdType>(pts[sideId]);
      return numCorners = 1;
    }
    return GetGenericSidePoints(input, cellId, sideDim, sideId, sidePts, ids, numCorners, cell);
  }
};

//------------------------------------------------------------------------------
// The object of a class of sides given to the functors of the dispatch functions, constant
// initialized once, whose tables are static members, see FixedSizeCellSides::Tables.
template <class Sides>
inline constexpr Sides CellSidesInstance{};

//------------------------------------------------------------------------------
// Call functor(sides) with the sides of the class of the nonlinear cell type cellType, and return
// its value, if any, in the order of vtkCellType.h: the cell types that DispatchCellSides() gives
// to NonLinearCellSides, which calls this function out of the hot loops.
template <class Functor>
VTK_ALWAYS_INLINE constexpr decltype(auto) DispatchNonLinearCellSides(
  unsigned char cellType, Functor&& functor)
{
  switch (cellType)
  {
    case VTK_QUADRATIC_EDGE:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_QUADRATIC_EDGE>>);
    case VTK_QUADRATIC_TRIANGLE:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_QUADRATIC_TRIANGLE>>);
    case VTK_QUADRATIC_QUAD:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_QUADRATIC_QUAD>>);
    case VTK_QUADRATIC_POLYGON:
      return functor(CellSidesInstance<PolygonCellSides<true>>);
    case VTK_QUADRATIC_TETRA:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_QUADRATIC_TETRA>>);
    case VTK_QUADRATIC_HEXAHEDRON:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_QUADRATIC_HEXAHEDRON>>);
    case VTK_QUADRATIC_WEDGE:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_QUADRATIC_WEDGE>>);
    case VTK_QUADRATIC_PYRAMID:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_QUADRATIC_PYRAMID>>);
    case VTK_BIQUADRATIC_QUAD:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_BIQUADRATIC_QUAD>>);
    case VTK_TRIQUADRATIC_HEXAHEDRON:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_TRIQUADRATIC_HEXAHEDRON>>);
    case VTK_TRIQUADRATIC_PYRAMID:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_TRIQUADRATIC_PYRAMID>>);
    case VTK_QUADRATIC_LINEAR_QUAD:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_QUADRATIC_LINEAR_QUAD>>);
    case VTK_QUADRATIC_LINEAR_WEDGE:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_QUADRATIC_LINEAR_WEDGE>>);
    case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_BIQUADRATIC_QUADRATIC_WEDGE>>);
    case VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON>>);
    case VTK_BIQUADRATIC_TRIANGLE:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_BIQUADRATIC_TRIANGLE>>);
    case VTK_CUBIC_LINE:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_CUBIC_LINE>>);
    case VTK_LAGRANGE_CURVE:
      return functor(CellSidesInstance<HigherOrderCellSides<VTK_LAGRANGE_CURVE>>);
    case VTK_LAGRANGE_TRIANGLE:
      return functor(CellSidesInstance<HigherOrderCellSides<VTK_LAGRANGE_TRIANGLE>>);
    case VTK_LAGRANGE_QUADRILATERAL:
      return functor(CellSidesInstance<HigherOrderCellSides<VTK_LAGRANGE_QUADRILATERAL>>);
    case VTK_LAGRANGE_TETRAHEDRON:
      return functor(CellSidesInstance<HigherOrderCellSides<VTK_LAGRANGE_TETRAHEDRON>>);
    case VTK_LAGRANGE_HEXAHEDRON:
      return functor(CellSidesInstance<HigherOrderCellSides<VTK_LAGRANGE_HEXAHEDRON>>);
    case VTK_LAGRANGE_WEDGE:
      return functor(CellSidesInstance<HigherOrderCellSides<VTK_LAGRANGE_WEDGE>>);
    case VTK_BEZIER_CURVE:
      return functor(CellSidesInstance<HigherOrderCellSides<VTK_BEZIER_CURVE>>);
    case VTK_BEZIER_TRIANGLE:
      return functor(CellSidesInstance<HigherOrderCellSides<VTK_BEZIER_TRIANGLE>>);
    case VTK_BEZIER_QUADRILATERAL:
      return functor(CellSidesInstance<HigherOrderCellSides<VTK_BEZIER_QUADRILATERAL>>);
    case VTK_BEZIER_TETRAHEDRON:
      return functor(CellSidesInstance<HigherOrderCellSides<VTK_BEZIER_TETRAHEDRON>>);
    case VTK_BEZIER_HEXAHEDRON:
      return functor(CellSidesInstance<HigherOrderCellSides<VTK_BEZIER_HEXAHEDRON>>);
    case VTK_BEZIER_WEDGE:
      return functor(CellSidesInstance<HigherOrderCellSides<VTK_BEZIER_WEDGE>>);
    default:
      // Not reached from DispatchCellSides(), which dispatches the other cell types.
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_EMPTY_CELL>>);
  }
}
static_assert(!vtkCellTypeTraits<VTK_QUADRATIC_EDGE>::IsLinear &&
    !vtkCellTypeTraits<VTK_QUADRATIC_TRIANGLE>::IsLinear &&
    !vtkCellTypeTraits<VTK_QUADRATIC_QUAD>::IsLinear &&
    !vtkCellTypeTraits<VTK_QUADRATIC_POLYGON>::IsLinear &&
    !vtkCellTypeTraits<VTK_QUADRATIC_TETRA>::IsLinear &&
    !vtkCellTypeTraits<VTK_QUADRATIC_HEXAHEDRON>::IsLinear &&
    !vtkCellTypeTraits<VTK_QUADRATIC_WEDGE>::IsLinear &&
    !vtkCellTypeTraits<VTK_QUADRATIC_PYRAMID>::IsLinear &&
    !vtkCellTypeTraits<VTK_BIQUADRATIC_QUAD>::IsLinear &&
    !vtkCellTypeTraits<VTK_TRIQUADRATIC_HEXAHEDRON>::IsLinear &&
    !vtkCellTypeTraits<VTK_TRIQUADRATIC_PYRAMID>::IsLinear &&
    !vtkCellTypeTraits<VTK_QUADRATIC_LINEAR_QUAD>::IsLinear &&
    !vtkCellTypeTraits<VTK_QUADRATIC_LINEAR_WEDGE>::IsLinear &&
    !vtkCellTypeTraits<VTK_BIQUADRATIC_QUADRATIC_WEDGE>::IsLinear &&
    !vtkCellTypeTraits<VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON>::IsLinear &&
    !vtkCellTypeTraits<VTK_BIQUADRATIC_TRIANGLE>::IsLinear &&
    !vtkCellTypeTraits<VTK_CUBIC_LINE>::IsLinear &&
    !vtkCellTypeTraits<VTK_LAGRANGE_CURVE>::IsLinear &&
    !vtkCellTypeTraits<VTK_LAGRANGE_TRIANGLE>::IsLinear &&
    !vtkCellTypeTraits<VTK_LAGRANGE_QUADRILATERAL>::IsLinear &&
    !vtkCellTypeTraits<VTK_LAGRANGE_TETRAHEDRON>::IsLinear &&
    !vtkCellTypeTraits<VTK_LAGRANGE_HEXAHEDRON>::IsLinear &&
    !vtkCellTypeTraits<VTK_LAGRANGE_WEDGE>::IsLinear &&
    !vtkCellTypeTraits<VTK_BEZIER_CURVE>::IsLinear &&
    !vtkCellTypeTraits<VTK_BEZIER_TRIANGLE>::IsLinear &&
    !vtkCellTypeTraits<VTK_BEZIER_QUADRILATERAL>::IsLinear &&
    !vtkCellTypeTraits<VTK_BEZIER_TETRAHEDRON>::IsLinear &&
    !vtkCellTypeTraits<VTK_BEZIER_HEXAHEDRON>::IsLinear &&
    !vtkCellTypeTraits<VTK_BEZIER_WEDGE>::IsLinear,
  "DispatchNonLinearCellSides() dispatches only nonlinear cell types");

//------------------------------------------------------------------------------
// The dimension of the cells of every cell type with traits, from vtkCellTypeTraits, read for the
// nonlinear ones, see NonLinearCellSides: DispatchCellSides() dispatches the cell types without
// traits to the empty cells.
constexpr std::array<int, VTK_NUMBER_OF_CELL_TYPES> GetDimensions()
{
  std::array<int, VTK_NUMBER_OF_CELL_TYPES> dimensions{};
  vtkForEachCellType(
    [&](auto traits)
    {
      using Traits = decltype(traits);
      dimensions[Traits::CellType] = Traits::Dimension;
    });
  return dimensions;
}
inline constexpr std::array<int, VTK_NUMBER_OF_CELL_TYPES> Dimensions = GetDimensions();

//------------------------------------------------------------------------------
// The nonlinear cells, dispatched out of the hot loops of DispatchCellSides(): their functions are
// not inlined, and call DispatchNonLinearCellSides() with the arguments of the sides, not the
// functor of the loop, whose references to the locals of the loop would escape, so that they would
// be kept in memory for every cell.
struct NonLinearCellSides
{
  static constexpr bool UsesNumberOfPoints = true;
  static constexpr int MaximumFaceCorners = 0;
  static constexpr int MinimumFaceCorners = 0;

  static constexpr int GetDimension(unsigned char cellType) { return Dimensions[cellType]; }

  template <class DataSetT>
  static VTK_NEVER_INLINE int GetNumberOfSides(DataSetT* input, vtkIdType cellId,
    unsigned char cellType, int sideDim, vtkIdType npts, vtkGenericCell* cell)
  {
    return DispatchNonLinearCellSides(cellType, [&](auto sides) VTK_ALWAYS_INLINE_LAMBDA
      { return sides.GetNumberOfSides(input, cellId, cellType, sideDim, npts, cell); });
  }

  static VTK_NEVER_INLINE int GetSideType(unsigned char cellType, int sideDim, int sideId)
  {
    return DispatchNonLinearCellSides(cellType, [&](auto sides) VTK_ALWAYS_INLINE_LAMBDA
      { return sides.GetSideType(cellType, sideDim, sideId); });
  }

  template <class DataSetT>
  static VTK_NEVER_INLINE int GetSideSize(DataSetT* input, vtkIdType cellId, unsigned char cellType,
    int sideDim, int sideId, vtkGenericCell* cell)
  {
    return DispatchNonLinearCellSides(cellType, [&](auto sides) VTK_ALWAYS_INLINE_LAMBDA
      { return sides.GetSideSize(input, cellId, cellType, sideDim, sideId, cell); });
  }

  template <class DataSetT, class PointsIterator>
  static vtkIdType GetSidePoints(DataSetT* input, vtkIdType cellId, unsigned char cellType,
    int sideDim, int sideId, vtkIdType npts, PointsIterator pts, vtkIdType* ptIds,
    vtkIdList* sidePts, const vtkIdType*& ids, vtkIdType& numCorners, vtkGenericCell* cell)
  {
    switch (sideDim)
    {
      case 0:
        ids = ptIds;
        ptIds[0] = static_cast<vtkIdType>(pts[sideId]);
        return numCorners = 1;
      case 1:
      case 2:
      default:
        return DispatchNonLinearCellSides(cellType,
          [&](auto sides) VTK_ALWAYS_INLINE_LAMBDA
          {
            return sides.GetSidePoints(input, cellId, cellType, sideDim, sideId, npts, pts, ptIds,
              sidePts, ids, numCorners, cell);
          });
    }
  }
};

//------------------------------------------------------------------------------
// Call functor(sides) with the sides of the class of the cell type cellType, and return its value,
// if any, in the order of vtkCellType.h: the linear cell types, see vtkCellTypeTraits, to their
// classes, the nonlinear ones to NonLinearCellSides, to keep the hot loops that call it small, and
// the cell types that cannot be built, see vtkGenericCell::SetCellType(), to the empty cells. The
// functor is inlined for every class, and must be, otherwise its members are read through memory
// for every side: a struct with a VTK_ALWAYS_INLINE call operator, or a lambda marked with
// VTK_ALWAYS_INLINE_LAMBDA.
template <class Functor>
VTK_ALWAYS_INLINE constexpr decltype(auto) DispatchCellSides(
  unsigned char cellType, Functor&& functor)
{
  switch (cellType)
  {
    case VTK_EMPTY_CELL:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_EMPTY_CELL>>);
    case VTK_VERTEX:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_VERTEX>>);
    case VTK_POLY_VERTEX:
      return functor(CellSidesInstance<PointListCellSides<VTK_POLY_VERTEX>>);
    case VTK_LINE:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_LINE>>);
    case VTK_POLY_LINE:
      return functor(CellSidesInstance<PointListCellSides<VTK_POLY_LINE>>);
    case VTK_TRIANGLE:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_TRIANGLE>>);
    case VTK_TRIANGLE_STRIP:
      return functor(CellSidesInstance<TriangleStripCellSides>);
    case VTK_POLYGON:
      return functor(CellSidesInstance<PolygonCellSides<false>>);
    case VTK_PIXEL:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_PIXEL>>);
    case VTK_QUAD:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_QUAD>>);
    case VTK_TETRA:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_TETRA>>);
    case VTK_VOXEL:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_VOXEL>>);
    case VTK_HEXAHEDRON:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_HEXAHEDRON>>);
    case VTK_WEDGE:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_WEDGE>>);
    case VTK_PYRAMID:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_PYRAMID>>);
    case VTK_PENTAGONAL_PRISM:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_PENTAGONAL_PRISM>>);
    case VTK_HEXAGONAL_PRISM:
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_HEXAGONAL_PRISM>>);
    case VTK_QUADRATIC_EDGE:
    case VTK_QUADRATIC_TRIANGLE:
    case VTK_QUADRATIC_QUAD:
    case VTK_QUADRATIC_POLYGON:
    case VTK_QUADRATIC_TETRA:
    case VTK_QUADRATIC_HEXAHEDRON:
    case VTK_QUADRATIC_WEDGE:
    case VTK_QUADRATIC_PYRAMID:
    case VTK_BIQUADRATIC_QUAD:
    case VTK_TRIQUADRATIC_HEXAHEDRON:
    case VTK_TRIQUADRATIC_PYRAMID:
    case VTK_QUADRATIC_LINEAR_QUAD:
    case VTK_QUADRATIC_LINEAR_WEDGE:
    case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
    case VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
    case VTK_BIQUADRATIC_TRIANGLE:
    case VTK_CUBIC_LINE:
      // The cell types of DispatchNonLinearCellSides(), out of the hot loops.
      return functor(CellSidesInstance<NonLinearCellSides>);
    case VTK_CONVEX_POINT_SET:
      return functor(CellSidesInstance<ConvexPointSetSides>);
    case VTK_POLYHEDRON:
      return functor(CellSidesInstance<PolyhedronCellSides>);
    case VTK_LAGRANGE_CURVE:
    case VTK_LAGRANGE_TRIANGLE:
    case VTK_LAGRANGE_QUADRILATERAL:
    case VTK_LAGRANGE_TETRAHEDRON:
    case VTK_LAGRANGE_HEXAHEDRON:
    case VTK_LAGRANGE_WEDGE:
    case VTK_BEZIER_CURVE:
    case VTK_BEZIER_TRIANGLE:
    case VTK_BEZIER_QUADRILATERAL:
    case VTK_BEZIER_TETRAHEDRON:
    case VTK_BEZIER_HEXAHEDRON:
    case VTK_BEZIER_WEDGE:
      // The cell types of DispatchNonLinearCellSides(), out of the hot loops.
      return functor(CellSidesInstance<NonLinearCellSides>);
    default:
      // The cell types that cannot be built, see vtkGenericCell::SetCellType(): without traits, the
      // abstract VTK_HIGHER_ORDER_ ones, and the higher order pyramids, are empty cells.
      return functor(CellSidesInstance<FixedSizeCellSides<VTK_EMPTY_CELL>>);
  }
}
static_assert(vtkCellTypeTraits<VTK_EMPTY_CELL>::IsLinear &&
    vtkCellTypeTraits<VTK_VERTEX>::IsLinear && vtkCellTypeTraits<VTK_POLY_VERTEX>::IsLinear &&
    vtkCellTypeTraits<VTK_LINE>::IsLinear && vtkCellTypeTraits<VTK_POLY_LINE>::IsLinear &&
    vtkCellTypeTraits<VTK_TRIANGLE>::IsLinear && vtkCellTypeTraits<VTK_TRIANGLE_STRIP>::IsLinear &&
    vtkCellTypeTraits<VTK_POLYGON>::IsLinear && vtkCellTypeTraits<VTK_PIXEL>::IsLinear &&
    vtkCellTypeTraits<VTK_QUAD>::IsLinear && vtkCellTypeTraits<VTK_TETRA>::IsLinear &&
    vtkCellTypeTraits<VTK_VOXEL>::IsLinear && vtkCellTypeTraits<VTK_HEXAHEDRON>::IsLinear &&
    vtkCellTypeTraits<VTK_WEDGE>::IsLinear && vtkCellTypeTraits<VTK_PYRAMID>::IsLinear &&
    vtkCellTypeTraits<VTK_PENTAGONAL_PRISM>::IsLinear &&
    vtkCellTypeTraits<VTK_HEXAGONAL_PRISM>::IsLinear &&
    vtkCellTypeTraits<VTK_CONVEX_POINT_SET>::IsLinear &&
    vtkCellTypeTraits<VTK_POLYHEDRON>::IsLinear,
  "DispatchCellSides() dispatches only the linear cell types to their classes, the others to "
  "NonLinearCellSides");

//------------------------------------------------------------------------------
// The number of sides of dimension sideDim (0 for the vertices, 1 for the edges, 2 for the faces)
// of the cell cellId, of type cellType, or 0 if it has none, e.g., the faces of a 2D cell: see
// vtkUnstructuredGrid::GetCellNumberOfEdges() and vtkUnstructuredGrid::GetCellNumberOfFaces(). It
// is given by the cell type, the number of points of the cell, or the faces of a polyhedron,
// without building the cell.
template <class DataSetT>
int GetCellNumberOfSides(
  DataSetT* input, vtkIdType cellId, unsigned char cellType, int sideDim, vtkGenericCell* cell)
{
  return DispatchCellSides(cellType,
    [&](auto sides) VTK_ALWAYS_INLINE_LAMBDA
    {
      if (VTK_UNLIKELY(sideDim < 0 || sideDim >= sides.GetDimension(cellType)))
      {
        return 0;
      }
      vtkIdType npts = 0;
      if constexpr (decltype(sides)::UsesNumberOfPoints)
      {
        npts = input->GetCellSize(cellId);
      }
      return sides.GetNumberOfSides(input, cellId, cellType, sideDim, npts, cell);
    });
}

//------------------------------------------------------------------------------
// The cell type of the side sideId of dimension sideDim (0 for the vertices, 1 for the edges, 2 for
// the faces) of the cells of type cellType, 0 <= sideId < its number of sides of this dimension,
// or VTK_EMPTY_CELL if they have no side of this dimension: the type of vtkCell::GetEdge() and
// vtkCell::GetFace(), e.g., VTK_QUADRATIC_QUAD for a face of a VTK_QUADRATIC_HEXAHEDRON, but for
// the faces of a voxel, which are quads, in the order of GetCellSidePoints(). It is given by the
// cell type, without the cell.
inline int GetCellSideType(unsigned char cellType, int sideDim, int sideId)
{
  return DispatchCellSides(cellType,
    [&](auto sides) VTK_ALWAYS_INLINE_LAMBDA
    {
      return VTK_LIKELY(sideDim >= 0 && sideDim < sides.GetDimension(cellType))
        ? sides.GetSideType(cellType, sideDim, sideId)
        : VTK_EMPTY_CELL;
    });
}

//------------------------------------------------------------------------------
// The number of points of the side sideId of dimension sideDim of the cell cellId, of type
// cellType, 0 <= sideId < its number of sides of this dimension, or 0 if the cell has no side of
// this dimension: the number of points of GetCellSidePoints(), without reading them, e.g., from the
// static tables of the cells of a fixed size. The scratch cell `cell` builds the cells whose sides
// are not read from static tables.
template <class DataSetT>
int GetCellSideSize(DataSetT* input, vtkIdType cellId, unsigned char cellType, int sideDim,
  int sideId, vtkGenericCell* cell)
{
  return DispatchCellSides(cellType,
    [&](auto sides) VTK_ALWAYS_INLINE_LAMBDA
    {
      return VTK_LIKELY(sideDim >= 0 && sideDim < sides.GetDimension(cellType))
        ? sides.GetSideSize(input, cellId, cellType, sideDim, sideId, cell)
        : 0;
    });
}

//------------------------------------------------------------------------------
// The points of the side sideId of dimension sideDim (0 for the vertices, 1 for the edges, 2 for
// the faces) of the cell cellId, of type cellType and whose npts points are `pts`, into sidePtIds,
// none if it is out of range: see vtkUnstructuredGrid::GetCellEdgePoints() and
// vtkUnstructuredGrid::GetCellFacePoints(), whose scratch cell is `cell`. The dataset is, e.g., a
// vtkUnstructuredGrid or a vtkPolyData.
template <class DataSetT>
VTK_ALWAYS_INLINE void GetCellSidePoints(DataSetT* input, vtkIdType cellId, unsigned char cellType,
  int sideDim, int sideId, vtkIdType npts, const vtkIdType* pts, vtkIdList* sidePtIds,
  vtkGenericCell* cell)
{
  DispatchCellSides(cellType,
    [&](auto sides) VTK_ALWAYS_INLINE_LAMBDA
    {
      if (VTK_UNLIKELY(sideDim < 0 || sideDim >= sides.GetDimension(cellType) || sideId < 0 ||
            sideId >= sides.GetNumberOfSides(input, cellId, cellType, sideDim, npts, cell)))
      {
        sidePtIds->Reset();
        return;
      }
      const vtkIdType* ids = nullptr;
      vtkIdType numCorners = 0;
      if constexpr (std::is_same_v<decltype(sides), PolyhedronCellSides>)
      {
        // A polyhedron may read its sides into sidePtIds, so the others are written into a
        // buffer, or point to its faces, then copied into sidePtIds.
        vtkIdType ptIds[MaximumCellSidePoints];
        const vtkIdType numSidePts = sides.GetSidePoints(input, cellId, cellType, sideDim, sideId,
          npts, pts, ptIds, sidePtIds, ids, numCorners, cell);
        if (ids != sidePtIds->GetPointer(0))
        {
          sidePtIds->SetNumberOfIds(numSidePts);
          std::copy_n(ids, numSidePts, sidePtIds->GetPointer(0));
        }
      }
      else
      {
        // The side is written into sidePtIds itself, with room for the largest side, or copied
        // into it by a built cell.
        sidePtIds->SetNumberOfIds(MaximumCellSidePoints);
        const vtkIdType numSidePts = sides.GetSidePoints(input, cellId, cellType, sideDim, sideId,
          npts, pts, sidePtIds->GetPointer(0), sidePtIds, ids, numCorners, cell);
        sidePtIds->SetNumberOfIds(numSidePts);
      }
    });
}

VTK_ABI_NAMESPACE_END
} // namespace vtkCellTypeSides
#endif // vtkCellTypeSides_h
// VTK-HeaderTest-Exclude: vtkCellTypeSides.h
