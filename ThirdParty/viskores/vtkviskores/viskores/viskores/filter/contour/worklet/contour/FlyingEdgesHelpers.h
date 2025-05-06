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


#ifndef viskores_worklet_contour_flyingedges_helpers_h
#define viskores_worklet_contour_flyingedges_helpers_h

#include <viskores/Types.h>

#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DeviceAdapterTag.h>

#include <viskores/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <viskores/cont/kokkos/internal/DeviceAdapterTagKokkos.h>

namespace viskores
{
namespace worklet
{
namespace flying_edges
{

struct FlyingEdges3D
{
public:
  // Edge case table values.
  enum EdgeClass
  {
    Below = 0,      // below isovalue
    Above = 1,      // above isovalue
    LeftAbove = 1,  // left vertex is above isovalue
    RightAbove = 2, // right vertex is above isovalue
    BothAbove = 3   // entire edge is above isovalue
  };
  enum CellClass
  {
    Interior = 0,
    MinBoundary = 1,
    MaxBoundary = 2
  };
};

struct SumXAxis
{
  static constexpr viskores::Id xindex = 0;
  static constexpr viskores::Id yindex = 1;
  static constexpr viskores::Id zindex = 2;

  static constexpr viskores::IdComponent windingIndex0 = 0;
  static constexpr viskores::IdComponent windingIndex1 = 2;
  static constexpr viskores::IdComponent windingIndex2 = 1;
};
struct SumYAxis
{
  static constexpr viskores::Id xindex = 1;
  static constexpr viskores::Id yindex = 0;
  static constexpr viskores::Id zindex = 2;

  static constexpr viskores::IdComponent windingIndex0 = 0;
  static constexpr viskores::IdComponent windingIndex1 = 1;
  static constexpr viskores::IdComponent windingIndex2 = 2;
};

template <typename Device>
struct select_AxisToSum
{
  using type = SumXAxis;
};

template <>
struct select_AxisToSum<viskores::cont::DeviceAdapterTagCuda>
{
  using type = SumYAxis;
};
template <>
struct select_AxisToSum<viskores::cont::DeviceAdapterTagKokkos>
{
  using type = SumYAxis;
};

inline viskores::cont::CellSetStructured<2> make_metaDataMesh2D(SumXAxis,
                                                                const viskores::Id3& pdims)
{
  viskores::cont::CellSetStructured<2> metaDataMesh;
  metaDataMesh.SetPointDimensions(viskores::Id2{ pdims[1], pdims[2] });
  return metaDataMesh;
}

inline viskores::cont::CellSetStructured<2> make_metaDataMesh2D(SumYAxis,
                                                                const viskores::Id3& pdims)
{
  viskores::cont::CellSetStructured<2> metaDataMesh;
  metaDataMesh.SetPointDimensions(viskores::Id2{ pdims[0], pdims[2] });
  return metaDataMesh;
}


VISKORES_EXEC inline viskores::Id3 compute_ijk(SumXAxis, const viskores::Id3& executionSpaceIJK)
{
  return viskores::Id3{ 0, executionSpaceIJK[0], executionSpaceIJK[1] };
}
VISKORES_EXEC inline viskores::Id3 compute_ijk(SumYAxis, const viskores::Id3& executionSpaceIJK)
{
  return viskores::Id3{ executionSpaceIJK[0], 0, executionSpaceIJK[1] };
}


VISKORES_EXEC inline viskores::Id3 compute_cdims(SumXAxis,
                                                 const viskores::Id3& executionSpacePDims,
                                                 viskores::Id numOfXPoints)
{
  return viskores::Id3{ numOfXPoints - 1, executionSpacePDims[0] - 1, executionSpacePDims[1] - 1 };
}
VISKORES_EXEC inline viskores::Id3 compute_cdims(SumYAxis,
                                                 const viskores::Id3& executionSpacePDims,
                                                 viskores::Id numOfYPoints)
{
  return viskores::Id3{ executionSpacePDims[0] - 1, numOfYPoints - 1, executionSpacePDims[1] - 1 };
}
VISKORES_EXEC inline viskores::Id3 compute_pdims(SumXAxis,
                                                 const viskores::Id3& executionSpacePDims,
                                                 viskores::Id numOfXPoints)
{
  return viskores::Id3{ numOfXPoints, executionSpacePDims[0], executionSpacePDims[1] };
}
VISKORES_EXEC inline viskores::Id3 compute_pdims(SumYAxis,
                                                 const viskores::Id3& executionSpacePDims,
                                                 viskores::Id numOfYPoints)
{
  return viskores::Id3{ executionSpacePDims[0], numOfYPoints, executionSpacePDims[1] };
}

VISKORES_EXEC inline viskores::Id compute_start(SumXAxis,
                                                const viskores::Id3& ijk,
                                                const viskores::Id3& dims)
{
  return (dims[0] * ijk[1]) + ((dims[0] * dims[1]) * ijk[2]);
}
VISKORES_EXEC inline viskores::Id compute_start(SumYAxis,
                                                const viskores::Id3& ijk,
                                                const viskores::Id3& dims)
{
  return ijk[0] + ((dims[0] * dims[1]) * ijk[2]);
}

VISKORES_EXEC inline viskores::Id4 compute_neighbor_starts(SumXAxis,
                                                           const viskores::Id3& ijk,
                                                           const viskores::Id3& pdims)
{
  //Optimized form of
  // return viskores::Id4 { compute_start(sx, ijk, pdims),
  //                  compute_start(sx, ijk + viskores::Id3{ 0, 1, 0 }, pdims),
  //                  compute_start(sx, ijk + viskores::Id3{ 0, 0, 1 }, pdims),
  //                  compute_start(sx, ijk + viskores::Id3{ 0, 1, 1 }, pdims) };
  const auto sliceSize = (pdims[0] * pdims[1]);
  const auto rowPos = (pdims[0] * ijk[1]);
  return viskores::Id4{ rowPos + (sliceSize * ijk[2]),
                        rowPos + pdims[0] + (sliceSize * ijk[2]),
                        rowPos + (sliceSize * (ijk[2] + 1)),
                        rowPos + pdims[0] + (sliceSize * (ijk[2] + 1)) };
}
VISKORES_EXEC inline viskores::Id4 compute_neighbor_starts(SumYAxis,
                                                           const viskores::Id3& ijk,
                                                           const viskores::Id3& pdims)
{
  //Optimized form of
  // return viskores::Id4{ compute_start(sy, ijk, pdims),
  //                   compute_start(sy, ijk + viskores::Id3{ 1, 0, 0 }, pdims),
  //                   compute_start(sy, ijk + viskores::Id3{ 0, 0, 1 }, pdims),
  //                   compute_start(sy, ijk + viskores::Id3{ 1, 0, 1 }, pdims) };
  const auto sliceSize = (pdims[0] * pdims[1]);
  return viskores::Id4{ ijk[0] + (sliceSize * ijk[2]),
                        ijk[0] + 1 + (sliceSize * ijk[2]),
                        ijk[0] + (sliceSize * (ijk[2] + 1)),
                        ijk[0] + 1 + (sliceSize * (ijk[2] + 1)) };
}



VISKORES_EXEC inline viskores::Id compute_inc(SumXAxis, const viskores::Id3&)
{
  return 1;
}
VISKORES_EXEC inline viskores::Id compute_inc(SumYAxis, const viskores::Id3& dims)
{
  return dims[0];
}

//----------------------------------------------------------------------------
template <typename WholeEdgeField>
VISKORES_EXEC inline viskores::UInt8 getEdgeCase(const WholeEdgeField& edges,
                                                 const viskores::Id4& startPos,
                                                 viskores::Id inc)
{
  viskores::UInt8 e0 = edges.Get(startPos[0] + inc);
  viskores::UInt8 e1 = edges.Get(startPos[1] + inc);
  viskores::UInt8 e2 = edges.Get(startPos[2] + inc);
  viskores::UInt8 e3 = edges.Get(startPos[3] + inc);
  return static_cast<viskores::UInt8>(e0 | (e1 << 2) | (e2 << 4) | (e3 << 6));
}


//----------------------------------------------------------------------------

template <typename WholeEdgeField, typename FieldInPointId>
VISKORES_EXEC inline bool computeTrimBounds(viskores::Id rightMax,
                                            const WholeEdgeField& edges,
                                            const FieldInPointId& axis_mins,
                                            const FieldInPointId& axis_maxs,
                                            const viskores::Id4& startPos,
                                            viskores::Id inc,
                                            viskores::Id& left,
                                            viskores::Id& right)
{
  // find adjusted trim values.
  left = viskores::Min(axis_mins[0], axis_mins[1]);
  left = viskores::Min(left, axis_mins[2]);
  left = viskores::Min(left, axis_mins[3]);

  right = viskores::Max(axis_maxs[0], axis_maxs[1]);
  right = viskores::Max(right, axis_maxs[2]);
  right = viskores::Max(right, axis_maxs[3]);

  // The trim edges may need adjustment if the contour travels between rows
  // of edges (without intersecting these edges). This means checking
  // whether the trim faces at (left,right) made up of the edges intersect
  // the contour.
  if (left > rightMax && right == 0)
  {
    //verify that we have nothing to generate and early terminate.
    bool mins_same = (axis_mins[0] == axis_mins[1] && axis_mins[0] == axis_mins[2] &&
                      axis_mins[0] == axis_mins[3]);
    bool maxs_same = (axis_maxs[0] == axis_maxs[1] && axis_maxs[0] == axis_maxs[2] &&
                      axis_maxs[0] == axis_maxs[3]);

    left = 0;
    right = rightMax;
    if (mins_same && maxs_same)
    {
      viskores::UInt8 e0 = edges.Get(startPos[0]);
      viskores::UInt8 e1 = edges.Get(startPos[1]);
      viskores::UInt8 e2 = edges.Get(startPos[2]);
      viskores::UInt8 e3 = edges.Get(startPos[3]);
      if (e0 == e1 && e1 == e2 && e2 == e3)
      {
        //We have nothing to process in this row
        return false;
      }
    }
  }
  else
  {

    viskores::UInt8 e0 = edges.Get(startPos[0] + (left * inc));
    viskores::UInt8 e1 = edges.Get(startPos[1] + (left * inc));
    viskores::UInt8 e2 = edges.Get(startPos[2] + (left * inc));
    viskores::UInt8 e3 = edges.Get(startPos[3] + (left * inc));
    if ((e0 & 0x1) != (e1 & 0x1) || (e1 & 0x1) != (e2 & 0x1) || (e2 & 0x1) != (e3 & 0x1))
    {
      left = 0;
    }

    e0 = edges.Get(startPos[0] + (right * inc));
    e1 = edges.Get(startPos[1] + (right * inc));
    e2 = edges.Get(startPos[2] + (right * inc));
    e3 = edges.Get(startPos[3] + (right * inc));
    if ((e0 & 0x2) != (e1 & 0x2) || (e1 & 0x2) != (e2 & 0x2) || (e2 & 0x2) != (e3 & 0x2))
    {
      right = rightMax;
    }
  }

  return true;
}
}
}
}
#endif
