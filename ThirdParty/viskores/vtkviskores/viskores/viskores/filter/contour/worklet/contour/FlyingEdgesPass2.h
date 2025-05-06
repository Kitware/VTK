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


#ifndef viskores_worklet_contour_flyingedges_pass2_h
#define viskores_worklet_contour_flyingedges_pass2_h

#include <viskores/filter/contour/worklet/contour/FlyingEdgesHelpers.h>
#include <viskores/filter/contour/worklet/contour/FlyingEdgesTables.h>

namespace viskores
{
namespace worklet
{
namespace flying_edges
{

struct ComputePass2 : public viskores::worklet::WorkletVisitCellsWithPoints
{
  viskores::Id3 PointDims;

  ComputePass2() {}
  explicit ComputePass2(const viskores::Id3& pdims)
    : PointDims(pdims)
  {
  }

  using ControlSignature = void(CellSetIn,
                                WholeArrayInOut axis_sums,
                                FieldInPoint axis_mins,
                                FieldInPoint axis_maxs,
                                FieldOutCell cell_tri_count,
                                WholeArrayIn edgeData);
  using ExecutionSignature = void(ThreadIndices, _2, _3, _4, _5, _6, Device);
  using InputDomain = _1;

  template <typename ThreadIndices,
            typename WholeSumField,
            typename FieldInPointId,
            typename WholeEdgeField,
            typename Device>
  VISKORES_EXEC void operator()(const ThreadIndices& threadIndices,
                                const WholeSumField& axis_sums,
                                const FieldInPointId& axis_mins,
                                const FieldInPointId& axis_maxs,
                                viskores::Int32& cell_tri_count,
                                const WholeEdgeField& edges,
                                Device) const
  {
    using AxisToSum = typename select_AxisToSum<Device>::type;

    // Pass 2. Traverse all cells in the meta data plane. This allows us to
    // easily grab the four edge cases bounding this voxel-row
    const viskores::Id3 ijk = compute_ijk(AxisToSum{}, threadIndices.GetInputIndex3D());
    const viskores::Id3 pdims = this->PointDims;

    const viskores::Id4 startPos = compute_neighbor_starts(AxisToSum{}, ijk, pdims);
    const viskores::Id axis_inc = compute_inc(AxisToSum{}, pdims);

    // Compute the subset (start and end) of the row that we need
    // to iterate to generate triangles for the iso-surface
    viskores::Id left, right;
    bool hasWork = computeTrimBounds(
      pdims[AxisToSum::xindex] - 1, edges, axis_mins, axis_maxs, startPos, axis_inc, left, right);
    if (!hasWork)
    {
      return;
    }

    viskores::Vec<bool, 3> onBoundary(false, false, false); //updated in for-loop
    onBoundary[AxisToSum::yindex] = (ijk[AxisToSum::yindex] >= (pdims[AxisToSum::yindex] - 2));
    onBoundary[AxisToSum::zindex] = (ijk[AxisToSum::zindex] >= (pdims[AxisToSum::zindex] - 2));

    cell_tri_count = 0;
    viskores::Id3 sums = axis_sums.Get(threadIndices.GetIndicesIncident()[0]);
    viskores::Id3 adj_row_sum(0, 0, 0);
    viskores::Id3 adj_col_sum(0, 0, 0);
    if (onBoundary[AxisToSum::yindex])
    {
      adj_row_sum = axis_sums.Get(threadIndices.GetIndicesIncident()[1]);
    }
    if (onBoundary[AxisToSum::zindex])
    {
      adj_col_sum = axis_sums.Get(threadIndices.GetIndicesIncident()[3]);
    }

    for (viskores::Id i = left; i < right; ++i) // run along the trimmed voxels
    {
      viskores::UInt8 edgeCase = getEdgeCase(edges, startPos, (axis_inc * i));
      viskores::UInt8 numTris = data::GetNumberOfPrimitives(edgeCase);
      if (numTris > 0)
      {
        cell_tri_count += numTris;

        // Count the number of y- and z-points to be generated. Pass# 1 counted
        // the number of x-intersections along the x-edges. Now we count all
        // intersections on the y- and z-voxel axes.
        auto* edgeUses = data::GetEdgeUses(edgeCase);

        onBoundary[AxisToSum::xindex] = (i >= (pdims[AxisToSum::xindex] - 2));

        // row axes edge always counted
        sums[AxisToSum::yindex] += edgeUses[4];
        // col axes edge always counted
        sums[AxisToSum::zindex] += edgeUses[8];

        // handle boundary
        this->CountBoundaryEdgeUses(
          AxisToSum{}, onBoundary, edgeUses, sums, adj_row_sum, adj_col_sum);
      }
    }

    axis_sums.Set(threadIndices.GetIndicesIncident()[0], sums);
    if (onBoundary[AxisToSum::yindex])
    {
      axis_sums.Set(threadIndices.GetIndicesIncident()[1], adj_row_sum);
    }
    if (onBoundary[AxisToSum::zindex])
    {
      axis_sums.Set(threadIndices.GetIndicesIncident()[3], adj_col_sum);
    }
  }

  //----------------------------------------------------------------------------
  // Count intersections along voxel axes. When traversing the volume across
  // edges, the voxel axes on the boundary may be undefined near boundaries
  // (because there are no fully-formed cells). Thus the voxel axes on the
  // boundary are treated specially.
  //
  // Only on these boundaries do we write to the metaData of our neighbor
  // as it is safe as those
  template <typename AxisToSum>
  VISKORES_EXEC inline void CountBoundaryEdgeUses(AxisToSum,
                                                  viskores::Vec<bool, 3> onBoundary,
                                                  viskores::UInt8 const* const edgeUses,
                                                  viskores::Id3& sums,
                                                  viskores::Id3& adj_row_sum,
                                                  viskores::Id3& adj_col_sum) const
  {
    if (onBoundary[AxisToSum::xindex]) //+x boundary
    {
      sums[AxisToSum::yindex] += edgeUses[5];
      sums[AxisToSum::zindex] += edgeUses[9];
      if (onBoundary[AxisToSum::yindex]) //+x +y
      {
        adj_row_sum[AxisToSum::zindex] += edgeUses[11];
      }
      if (onBoundary[AxisToSum::zindex]) //+x +z
      {
        adj_col_sum[AxisToSum::yindex] += edgeUses[7];
      }
    }
    if (onBoundary[AxisToSum::yindex]) //+y boundary
    {
      adj_row_sum[AxisToSum::zindex] += edgeUses[10];
    }
    if (onBoundary[AxisToSum::zindex]) //+z boundary
    {
      adj_col_sum[AxisToSum::yindex] += edgeUses[6];
    }
  }
};
}
}
}

#endif
