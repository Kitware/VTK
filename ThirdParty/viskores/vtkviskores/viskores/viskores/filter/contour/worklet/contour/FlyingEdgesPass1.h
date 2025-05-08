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


#ifndef viskores_worklet_contour_flyingedges_pass1_h
#define viskores_worklet_contour_flyingedges_pass1_h

#include <viskores/filter/contour/worklet/contour/FlyingEdgesHelpers.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{
namespace worklet
{
namespace flying_edges
{

/*
* Understanding Pass1 in general
*
* PASS 1: Process all of the voxel edges that compose each row. Determine the
* edges case classification, count the number of edge intersections, and
* figure out where intersections along the row begins and ends
* (i.e., gather information for computational trimming).
*
* So in general the algorithm selects a primary axis to stride ( X or Y).
* It does this by forming a plane along the other two axes and marching
* over the sum/primary axis.
*
* So for SumXAxis, this means that we form a YZ plane and march the
* X axis along each point. As we march we are looking at the X axis edge
* that is formed by the current and next point.
*
* So for SumYAxis, this means that we form a XZ plane and march the
* Y axis along each point. As we march we are looking at the Y axis edge
* that is formed by the current and next point.
*
*/

template <typename WholeEdgeField>
inline VISKORES_EXEC void write_edge(SumXAxis,
                                     viskores::Id write_index,
                                     WholeEdgeField& edges,
                                     viskores::UInt8 edgeCase)
{
  edges.Set(write_index, edgeCase);
}

template <typename WholeEdgeField>
inline VISKORES_EXEC void write_edge(SumYAxis,
                                     viskores::Id write_index,
                                     WholeEdgeField& edges,
                                     viskores::UInt8 edgeCase)
{
  if (edgeCase != FlyingEdges3D::Below)
  {
    edges.Set(write_index, edgeCase);
  }
}

template <typename T>
struct ComputePass1 : public viskores::worklet::WorkletVisitPointsWithCells
{
  viskores::Id3 PointDims;
  T IsoValue;

  ComputePass1() {}
  ComputePass1(T value, const viskores::Id3& pdims)
    : PointDims(pdims)
    , IsoValue(value)
  {
  }

  using ControlSignature = void(CellSetIn,
                                FieldOut axis_sum,
                                FieldOut axis_min,
                                FieldOut axis_max,
                                WholeArrayInOut edgeData,
                                WholeArrayIn data);
  using ExecutionSignature = void(ThreadIndices, _2, _3, _4, _5, _6, Device);
  using InputDomain = _1;

  template <typename ThreadIndices,
            typename WholeEdgeField,
            typename WholeDataField,
            typename Device>
  VISKORES_EXEC void operator()(const ThreadIndices& threadIndices,
                                viskores::Id3& axis_sum,
                                viskores::Id& axis_min,
                                viskores::Id& axis_max,
                                WholeEdgeField& edges,
                                const WholeDataField& field,
                                Device) const
  {
    using AxisToSum = typename select_AxisToSum<Device>::type;

    const viskores::Id3 ijk = compute_ijk(AxisToSum{}, threadIndices.GetInputIndex3D());
    const viskores::Id3 dims = this->PointDims;
    const viskores::Id startPos = compute_start(AxisToSum{}, ijk, dims);
    const viskores::Id offset = compute_inc(AxisToSum{}, dims);

    const T value = this->IsoValue;
    axis_min = this->PointDims[AxisToSum::xindex];
    axis_max = 0;
    T s1 = field.Get(startPos);
    T s0 = s1;
    axis_sum = { 0, 0, 0 };
    const viskores::Id end = this->PointDims[AxisToSum::xindex] - 1;
    for (viskores::Id i = 0; i < end; ++i)
    {
      s0 = s1;
      s1 = field.Get(startPos + (offset * (i + 1)));

      viskores::UInt8 edgeCase = FlyingEdges3D::Below;
      if (s0 >= value)
      {
        edgeCase = FlyingEdges3D::LeftAbove;
      }
      if (s1 >= value)
      {
        edgeCase |= FlyingEdges3D::RightAbove;
      }

      write_edge(AxisToSum{}, startPos + (offset * i), edges, edgeCase);

      if (edgeCase == FlyingEdges3D::LeftAbove || edgeCase == FlyingEdges3D::RightAbove)
      {
        axis_sum[AxisToSum::xindex] += 1; // increment number of intersections along axis
        axis_max = i + 1;
        if (axis_min == (end + 1))
        {
          axis_min = i;
        }
      }
    }
    write_edge(AxisToSum{}, startPos + (offset * end), edges, FlyingEdges3D::Below);
  }
};

struct launchComputePass1
{
  void FillEdgeCases(viskores::cont::ArrayHandle<viskores::UInt8>&, SumXAxis) const
  {
    // Do nothing
  }
  void FillEdgeCases(viskores::cont::ArrayHandle<viskores::UInt8>& edgeCases, SumYAxis) const
  {
    edgeCases.Fill(static_cast<viskores::UInt8>(FlyingEdges3D::Below));
  }

  template <typename DeviceAdapterTag,
            typename IVType,
            typename T,
            typename StorageTagField,
            typename... Args>
  VISKORES_CONT bool operator()(DeviceAdapterTag device,
                                const ComputePass1<IVType>& worklet,
                                const viskores::cont::ArrayHandle<T, StorageTagField>& inputField,
                                viskores::cont::ArrayHandle<viskores::UInt8>& edgeCases,
                                viskores::cont::CellSetStructured<2>& metaDataMesh2D,
                                Args&&... args) const
  {
    using AxisToSum = typename select_AxisToSum<DeviceAdapterTag>::type;

    viskores::cont::Invoker invoke(device);
    metaDataMesh2D = make_metaDataMesh2D(AxisToSum{}, worklet.PointDims);

    this->FillEdgeCases(edgeCases, AxisToSum{});
    invoke(worklet, metaDataMesh2D, std::forward<Args>(args)..., edgeCases, inputField);
    return true;
  }
};
}
}
}


#endif
