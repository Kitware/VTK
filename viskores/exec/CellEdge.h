//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_exec_CellEdge_h
#define viskores_exec_CellEdge_h

#include <viskores/CellShape.h>
#include <viskores/CellTraits.h>
#include <viskores/ErrorCode.h>
#include <viskores/Types.h>
#include <viskores/exec/FunctorBase.h>

namespace viskores
{
namespace exec
{

namespace detail
{

class CellEdgeTables
{
public:
  static constexpr viskores::Int32 MAX_NUM_EDGES = 12;

public:
  VISKORES_EXEC viskores::Int32 NumEdges(viskores::Int32 cellShapeId) const
  {
    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Int32 numEdges[viskores::NUMBER_OF_CELL_SHAPES] = {
      // NumEdges
      0,  //  0: CELL_SHAPE_EMPTY
      0,  //  1: CELL_SHAPE_VERTEX
      0,  //  2: Unused
      0,  //  3: CELL_SHAPE_LINE
      0,  //  4: CELL_SHAPE_POLY_LINE
      3,  //  5: CELL_SHAPE_TRIANGLE
      0,  //  6: Unused
      -1, //  7: CELL_SHAPE_POLYGON  ---special case---
      0,  //  8: Unused
      4,  //  9: CELL_SHAPE_QUAD
      6,  // 10: CELL_SHAPE_TETRA
      0,  // 11: Unused
      12, // 12: CELL_SHAPE_HEXAHEDRON
      9,  // 13: CELL_SHAPE_WEDGE
      8   // 14: CELL_SHAPE_PYRAMID
    };
    return numEdges[cellShapeId];
  }

  VISKORES_EXEC viskores::Int32 PointsInEdge(viskores::Int32 cellShapeId,
                                             viskores::Int32 edgeIndex,
                                             viskores::Int32 localPointIndex) const
  {
    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Int32
      pointsInEdge[viskores::NUMBER_OF_CELL_SHAPES][MAX_NUM_EDGES][2] = {
        // clang-format off
        //  0: CELL_SHAPE_EMPTY
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  1: CELL_SHAPE_VERTEX
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  2: Unused
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  3: CELL_SHAPE_LINE
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  4: CELL_SHAPE_POLY_LINE
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  5: CELL_SHAPE_TRIANGLE
        { { 0, 1 },   { 1, 2 },   { 2, 0 },   { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  6: Unused
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  7: CELL_SHAPE_POLYGON  --- special case ---
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  8: Unused
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        //  9: CELL_SHAPE_QUAD
        { { 0, 1 },   { 1, 2 },   { 2, 3 },   { 3, 0 },   { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        // 10: CELL_SHAPE_TETRA
        { { 0, 1 },   { 1, 2 },   { 2, 0 },   { 0, 3 },   { 1, 3 },   { 2, 3 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        // 11: Unused
        { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 },
          { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        // 12: CELL_SHAPE_HEXAHEDRON
        { { 0, 1 }, { 1, 2 }, { 3, 2 }, { 0, 3 }, { 4, 5 }, { 5, 6 },
          { 7, 6 }, { 4, 7 }, { 0, 4 }, { 1, 5 }, { 3, 7 }, { 2, 6 } },
        // 13: CELL_SHAPE_WEDGE
        { { 0, 1 }, { 1, 2 }, { 2, 0 }, { 3, 4 },   { 4, 5 },   { 5, 3 },
          { 0, 3 }, { 1, 4 }, { 2, 5 }, { -1, -1 }, { -1, -1 }, { -1, -1 } },
        // 14: CELL_SHAPE_PYRAMID
        { { 0, 1 }, { 1, 2 }, { 2, 3 },   { 3, 0 },   { 0, 4 },   { 1, 4 },
          { 2, 4 }, { 3, 4 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } }
        // clang-format on
      };

    return pointsInEdge[cellShapeId][edgeIndex][localPointIndex];
  }
};

} // namespace detail

template <typename CellShapeTag>
static inline VISKORES_EXEC viskores::ErrorCode CellEdgeNumberOfEdges(
  viskores::IdComponent numPoints,
  CellShapeTag,
  viskores::IdComponent& numEdges)
{
  if (numPoints != viskores::CellTraits<CellShapeTag>::NUM_POINTS)
  {
    numEdges = -1;
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }
  numEdges = detail::CellEdgeTables{}.NumEdges(CellShapeTag::Id);
  return viskores::ErrorCode::Success;
}

static inline VISKORES_EXEC viskores::ErrorCode CellEdgeNumberOfEdges(
  viskores::IdComponent numPoints,
  viskores::CellShapeTagPolygon,
  viskores::IdComponent& numEdges)
{
  if (numPoints <= 0)
  {
    numEdges = -1;
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }
  numEdges = numPoints;
  return viskores::ErrorCode::Success;
}

static inline VISKORES_EXEC viskores::ErrorCode CellEdgeNumberOfEdges(
  viskores::IdComponent numPoints,
  viskores::CellShapeTagPolyLine,
  viskores::IdComponent& numEdges)
{
  if (numPoints <= 0)
  {
    numEdges = -1;
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }
  numEdges = detail::CellEdgeTables{}.NumEdges(viskores::CELL_SHAPE_POLY_LINE);
  return viskores::ErrorCode::Success;
}

/// @brief Get the number of edges in a cell.
///
/// @param[in]  numPoints The number of points in the cell.
/// @param[in]  shape A tag of type `CellShapeTag*` to identify the shape of the cell.
///     This method is overloaded for different shape types.
/// @param[out] numEdges A reference to return the number of edges.
static inline VISKORES_EXEC viskores::ErrorCode CellEdgeNumberOfEdges(
  viskores::IdComponent numPoints,
  viskores::CellShapeTagGeneric shape,
  viskores::IdComponent& numEdges)
{
  if (shape.Id == viskores::CELL_SHAPE_POLYGON)
  {
    return CellEdgeNumberOfEdges(numPoints, viskores::CellShapeTagPolygon(), numEdges);
  }
  else if (shape.Id == viskores::CELL_SHAPE_POLY_LINE)
  {
    return CellEdgeNumberOfEdges(numPoints, viskores::CellShapeTagPolyLine(), numEdges);
  }
  else
  {
    numEdges = detail::CellEdgeTables{}.NumEdges(shape.Id);
    return viskores::ErrorCode::Success;
  }
}

template <typename CellShapeTag>
static inline VISKORES_EXEC viskores::ErrorCode CellEdgeLocalIndex(viskores::IdComponent numPoints,
                                                                   viskores::IdComponent pointIndex,
                                                                   viskores::IdComponent edgeIndex,
                                                                   CellShapeTag shape,
                                                                   viskores::IdComponent& result)
{
  if ((pointIndex < 0) || (pointIndex > 1))
  {
    result = -1;
    return viskores::ErrorCode::InvalidPointId;
  }
  if ((edgeIndex < 0) || (edgeIndex >= detail::CellEdgeTables::MAX_NUM_EDGES))
  {
    result = -1;
    return viskores::ErrorCode::InvalidEdgeId;
  }

  viskores::IdComponent numEdges;
  VISKORES_RETURN_ON_ERROR(viskores::exec::CellEdgeNumberOfEdges(numPoints, shape, numEdges));
  if (edgeIndex >= numEdges)
  {
    result = -1;
    return viskores::ErrorCode::InvalidEdgeId;
  }

  detail::CellEdgeTables table;
  result = table.PointsInEdge(CellShapeTag::Id, edgeIndex, pointIndex);
  return viskores::ErrorCode::Success;
}

static inline VISKORES_EXEC viskores::ErrorCode CellEdgeLocalIndex(viskores::IdComponent numPoints,
                                                                   viskores::IdComponent pointIndex,
                                                                   viskores::IdComponent edgeIndex,
                                                                   viskores::CellShapeTagPolygon,
                                                                   viskores::IdComponent& result)
{
  if (numPoints < 3)
  {
    result = -1;
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }
  if ((pointIndex < 0) || (pointIndex > 1))
  {
    result = -1;
    return viskores::ErrorCode::InvalidPointId;
  }
  if ((edgeIndex < 0) || (edgeIndex >= numPoints))
  {
    result = -1;
    return viskores::ErrorCode::InvalidEdgeId;
  }

  if (edgeIndex + pointIndex < numPoints)
  {
    result = edgeIndex + pointIndex;
  }
  else
  {
    result = 0;
  }
  return viskores::ErrorCode::Success;
}

/// Given the index for an edge of a cell and one of the points on that edge, this
/// function returns the point index for the cell.
/// To get the point indices relative to the data set, the returned index should be used
/// to reference a `PointIndices` list.
///
/// @param[in]  numPoints The number of points in the cell.
/// @param[in]  pointIndex The index of the edge within the cell.
/// @param[in]  edgeIndex The index of the point on the edge (either 0 or 1).
/// @param[in]  shape A tag of type `CellShapeTag*` to identify the shape of the cell.
///     This method is overloaded for different shape types.
/// @param[out] result Reference to put the index of the point relative to the cell
///     (between 0 and the number of points in the cell).
static inline VISKORES_EXEC viskores::ErrorCode CellEdgeLocalIndex(
  viskores::IdComponent numPoints,
  viskores::IdComponent pointIndex,
  viskores::IdComponent edgeIndex,
  viskores::CellShapeTagGeneric shape,
  viskores::IdComponent& result)
{
  if ((pointIndex < 0) || (pointIndex > 1))
  {
    result = -1;
    return viskores::ErrorCode::InvalidPointId;
  }
  if ((edgeIndex < 0) || (edgeIndex >= detail::CellEdgeTables::MAX_NUM_EDGES))
  {
    result = -1;
    return viskores::ErrorCode::InvalidEdgeId;
  }

  if (shape.Id == viskores::CELL_SHAPE_POLYGON)
  {
    return CellEdgeLocalIndex(
      numPoints, pointIndex, edgeIndex, viskores::CellShapeTagPolygon(), result);
  }
  else
  {
    detail::CellEdgeTables table;
    if (edgeIndex >= table.NumEdges(shape.Id))
    {
      result = -1;
      return viskores::ErrorCode::InvalidEdgeId;
    }

    result = table.PointsInEdge(shape.Id, edgeIndex, pointIndex);
    return viskores::ErrorCode::Success;
  }
}

/// @brief Returns a canonical identifier for a cell edge
///
/// Given information about a cell edge and the global point indices for that cell, returns a
/// viskores::Id2 that contains values that are unique to that edge. The values for two edges will be
/// the same if and only if the edges contain the same points.
///
template <typename CellShapeTag, typename GlobalPointIndicesVecType>
static inline VISKORES_EXEC viskores::ErrorCode CellEdgeCanonicalId(
  viskores::IdComponent numPoints,
  viskores::IdComponent edgeIndex,
  CellShapeTag shape,
  const GlobalPointIndicesVecType& globalPointIndicesVec,
  viskores::Id2& result)
{
  result = { -1, -1 };

  viskores::IdComponent localIndex0;
  VISKORES_RETURN_ON_ERROR(
    viskores::exec::CellEdgeLocalIndex(numPoints, 0, edgeIndex, shape, localIndex0));
  viskores::Id pointIndex0 = globalPointIndicesVec[localIndex0];

  viskores::IdComponent localIndex1;
  VISKORES_RETURN_ON_ERROR(
    viskores::exec::CellEdgeLocalIndex(numPoints, 1, edgeIndex, shape, localIndex1));
  viskores::Id pointIndex1 = globalPointIndicesVec[localIndex1];

  if (pointIndex0 < pointIndex1)
  {
    result = viskores::Id2(pointIndex0, pointIndex1);
  }
  else
  {
    result = viskores::Id2(pointIndex1, pointIndex0);
  }

  return viskores::ErrorCode::Success;
}

}
} // namespace viskores::exec

#endif //viskores_exec_CellFaces_h
