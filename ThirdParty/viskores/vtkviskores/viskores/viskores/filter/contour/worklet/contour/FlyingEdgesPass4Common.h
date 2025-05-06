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


#ifndef viskores_worklet_contour_flyingedges_pass4_common_h
#define viskores_worklet_contour_flyingedges_pass4_common_h

#include <viskores/filter/contour/worklet/contour/FlyingEdgesHelpers.h>
#include <viskores/filter/contour/worklet/contour/FlyingEdgesTables.h>

namespace viskores
{
namespace worklet
{
namespace flying_edges
{

VISKORES_EXEC inline viskores::Id3 compute_incs3d(const viskores::Id3& dims)
{
  return viskores::Id3{ 1, dims[0], (dims[0] * dims[1]) };
}

VISKORES_EXEC inline constexpr viskores::Id increment_cellId(SumXAxis,
                                                             viskores::Id cellId,
                                                             viskores::Id,
                                                             viskores::Id numToIncrement = 1)
{
  return cellId + numToIncrement;
}
VISKORES_EXEC inline constexpr viskores::Id increment_cellId(SumYAxis,
                                                             viskores::Id cellId,
                                                             viskores::Id y_point_axis_inc,
                                                             viskores::Id numToIncrement = 1)
{
  return cellId + ((y_point_axis_inc - 1) * numToIncrement);
}

VISKORES_EXEC inline bool case_includes_axes(viskores::UInt8 const* const edgeUses)
{
  return (edgeUses[0] != 0 || edgeUses[4] != 0 || edgeUses[8] != 0);
}

template <typename AxisToSum, typename WholeConnField, typename WholeCellIdField>
VISKORES_EXEC inline void generate_tris(viskores::Id inputCellId,
                                        viskores::UInt8 edgeCase,
                                        viskores::UInt8 numTris,
                                        viskores::Id* edgeIds,
                                        viskores::Id& triId,
                                        const WholeConnField& conn,
                                        const WholeCellIdField& cellIds)
{
  auto* edges = data::GetTriEdgeCases(edgeCase);
  viskores::Id edgeIndex = 1;
  viskores::Id index = static_cast<viskores::Id>(triId) * 3;
  for (viskores::UInt8 i = 0; i < numTris; ++i)
  {
    cellIds.Set(triId + i, inputCellId);

    //This keeps the same winding for the triangles that marching cells
    //produced. By keeping the winding the same we make sure
    //that 'fast' normals are consistent with the marching
    //cells version
    conn.Set(index, edgeIds[edges[edgeIndex + AxisToSum::windingIndex0]]);
    conn.Set(index + 1, edgeIds[edges[edgeIndex + AxisToSum::windingIndex1]]);
    conn.Set(index + 2, edgeIds[edges[edgeIndex + AxisToSum::windingIndex2]]);
    index += 3;
    edgeIndex += 3;
  }
  triId += numTris;
}


// Helper function to set up the point ids on voxel edges.
//----------------------------------------------------------------------------
template <typename AxisToSum, typename FieldInPointId3>
VISKORES_EXEC inline void init_voxelIds(AxisToSum,
                                        viskores::Id writeOffset,
                                        viskores::UInt8 edgeCase,
                                        const FieldInPointId3& axis_sums,
                                        viskores::Id* edgeIds)
{
  auto* edgeUses = data::GetEdgeUses(edgeCase);
  edgeIds[0] = writeOffset + axis_sums[0][AxisToSum::xindex]; // x-edges
  edgeIds[1] = writeOffset + axis_sums[1][AxisToSum::xindex];
  edgeIds[2] = writeOffset + axis_sums[3][AxisToSum::xindex];
  edgeIds[3] = writeOffset + axis_sums[2][AxisToSum::xindex];
  edgeIds[4] = writeOffset + axis_sums[0][AxisToSum::yindex]; // y-edges
  edgeIds[5] = edgeIds[4] + edgeUses[4];
  edgeIds[6] = writeOffset + axis_sums[3][AxisToSum::yindex];
  edgeIds[7] = edgeIds[6] + edgeUses[6];
  edgeIds[8] = writeOffset + axis_sums[0][AxisToSum::zindex]; // z-edges
  edgeIds[9] = edgeIds[8] + edgeUses[8];
  edgeIds[10] = writeOffset + axis_sums[1][AxisToSum::zindex];
  edgeIds[11] = edgeIds[10] + edgeUses[10];
}

// Helper function to advance the point ids along voxel rows.
//----------------------------------------------------------------------------
VISKORES_EXEC inline void advance_voxelIds(viskores::UInt8 const* const edgeUses,
                                           viskores::Id* edgeIds)
{
  edgeIds[0] += edgeUses[0]; // x-edges
  edgeIds[1] += edgeUses[1];
  edgeIds[2] += edgeUses[2];
  edgeIds[3] += edgeUses[3];
  edgeIds[4] += edgeUses[4]; // y-edges
  edgeIds[5] = edgeIds[4] + edgeUses[5];
  edgeIds[6] += edgeUses[6];
  edgeIds[7] = edgeIds[6] + edgeUses[7];
  edgeIds[8] += edgeUses[8]; // z-edges
  edgeIds[9] = edgeIds[8] + edgeUses[9];
  edgeIds[10] += edgeUses[10];
  edgeIds[11] = edgeIds[10] + edgeUses[11];
}

//----------------------------------------------------------------------------
struct Pass4TrimState
{
  viskores::Id left, right;
  viskores::Id3 ijk;
  viskores::Id4 startPos;
  viskores::Id cellId;
  viskores::Id axis_inc;
  viskores::Vec<viskores::UInt8, 3> boundaryStatus;
  bool hasWork = true;

  template <typename AxisToSum,
            typename ThreadIndices,
            typename WholeSumField,
            typename FieldInPointId,
            typename WholeEdgeField>
  VISKORES_EXEC Pass4TrimState(AxisToSum,
                               const viskores::Id3& pdims,
                               const ThreadIndices& threadIndices,
                               const WholeSumField& viskoresNotUsed(axis_sums),
                               const FieldInPointId& axis_mins,
                               const FieldInPointId& axis_maxs,
                               const WholeEdgeField& edges)
  {
    ijk = compute_ijk(AxisToSum{}, threadIndices.GetInputIndex3D());

    startPos = compute_neighbor_starts(AxisToSum{}, ijk, pdims);
    axis_inc = compute_inc(AxisToSum{}, pdims);

    // Compute the subset (start and end) of the row that we need
    // to iterate to generate triangles for the iso-surface
    hasWork = computeTrimBounds(
      pdims[AxisToSum::xindex] - 1, edges, axis_mins, axis_maxs, startPos, axis_inc, left, right);
    hasWork = hasWork && left != right;
    if (!hasWork)
    {
      return;
    }


    cellId = compute_start(AxisToSum{}, ijk, pdims - viskores::Id3{ 1, 1, 1 });

    //update our ijk
    cellId = increment_cellId(AxisToSum{}, cellId, axis_inc, left - ijk[AxisToSum::xindex]);
    ijk[AxisToSum::xindex] = left;

    boundaryStatus[0] = FlyingEdges3D::Interior;
    boundaryStatus[1] = FlyingEdges3D::Interior;
    boundaryStatus[2] = FlyingEdges3D::Interior;

    if (ijk[AxisToSum::xindex] < 1)
    {
      boundaryStatus[AxisToSum::xindex] += FlyingEdges3D::MinBoundary;
    }
    if (ijk[AxisToSum::xindex] >= (pdims[AxisToSum::xindex] - 2))
    {
      boundaryStatus[AxisToSum::xindex] += FlyingEdges3D::MaxBoundary;
    }
    if (ijk[AxisToSum::yindex] < 1)
    {
      boundaryStatus[AxisToSum::yindex] += FlyingEdges3D::MinBoundary;
    }
    if (ijk[AxisToSum::yindex] >= (pdims[AxisToSum::yindex] - 2))
    {
      boundaryStatus[AxisToSum::yindex] += FlyingEdges3D::MaxBoundary;
    }
    if (ijk[AxisToSum::zindex] < 1)
    {
      boundaryStatus[AxisToSum::zindex] += FlyingEdges3D::MinBoundary;
    }
    if (ijk[AxisToSum::zindex] >= (pdims[AxisToSum::zindex] - 2))
    {
      boundaryStatus[AxisToSum::zindex] += FlyingEdges3D::MaxBoundary;
    }
  }

  template <typename AxisToSum>
  VISKORES_EXEC inline void increment(AxisToSum, const viskores::Id3& pdims)
  {
    //compute what the current cellId is
    cellId = increment_cellId(AxisToSum{}, cellId, axis_inc);

    //compute what the current ijk is
    ijk[AxisToSum::xindex]++;

    // compute what the current boundary state is
    // can never be on the MinBoundary after we increment
    if (ijk[AxisToSum::xindex] >= (pdims[AxisToSum::xindex] - 2))
    {
      boundaryStatus[AxisToSum::xindex] = FlyingEdges3D::MaxBoundary;
    }
    else
    {
      boundaryStatus[AxisToSum::xindex] = FlyingEdges3D::Interior;
    }
  }
};

// Helper function to state if a boundary object refers to a location that
// is fully inside ( not on a boundary )
//----------------------------------------------------------------------------
VISKORES_EXEC inline bool fully_interior(const viskores::Vec<viskores::UInt8, 3>& boundaryStatus)
{
  return boundaryStatus[0] == FlyingEdges3D::Interior &&
    boundaryStatus[1] == FlyingEdges3D::Interior && boundaryStatus[2] == FlyingEdges3D::Interior;
}
}
}
}
#endif
