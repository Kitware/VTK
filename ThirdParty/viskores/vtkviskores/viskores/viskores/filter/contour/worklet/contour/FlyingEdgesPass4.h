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


#ifndef viskores_worklet_contour_flyingedges_pass4_h
#define viskores_worklet_contour_flyingedges_pass4_h

#include <viskores/filter/contour/worklet/contour/FlyingEdgesPass4Common.h>
#include <viskores/filter/contour/worklet/contour/FlyingEdgesPass4X.h>
#include <viskores/filter/contour/worklet/contour/FlyingEdgesPass4XWithNormals.h>
#include <viskores/filter/contour/worklet/contour/FlyingEdgesPass4Y.h>

namespace viskores
{
namespace worklet
{
namespace flying_edges
{

struct launchComputePass4
{
  viskores::Id3 PointDims;

  viskores::Id CellWriteOffset;
  viskores::Id PointWriteOffset;

  launchComputePass4(const viskores::Id3& pdims,
                     viskores::Id multiContourCellOffset,
                     viskores::Id multiContourPointOffset)
    : PointDims(pdims)
    , CellWriteOffset(multiContourCellOffset)
    , PointWriteOffset(multiContourPointOffset)
  {
  }

  template <typename DeviceAdapterTag,
            typename IVType,
            typename T,
            typename CoordsType,
            typename StorageTagField,
            typename MeshSums,
            typename PointType,
            typename NormalType>
  VISKORES_CONT bool LaunchXAxis(
    DeviceAdapterTag device,
    viskores::Id viskoresNotUsed(newPointSize),
    IVType isoval,
    CoordsType coordinateSystem,
    const viskores::cont::ArrayHandle<T, StorageTagField>& inputField,
    viskores::cont::ArrayHandle<viskores::UInt8> edgeCases,
    viskores::cont::CellSetStructured<2>& metaDataMesh2D,
    const MeshSums& metaDataSums,
    const viskores::cont::ArrayHandle<viskores::Id>& metaDataMin,
    const viskores::cont::ArrayHandle<viskores::Id>& metaDataMax,
    const viskores::cont::ArrayHandle<viskores::Int32>& metaDataNumTris,
    viskores::worklet::contour::CommonState& sharedState,
    viskores::cont::ArrayHandle<viskores::Id>& triangle_topology,
    PointType& points,
    NormalType& normals) const
  {
    viskores::cont::Invoker invoke(device);
    if (sharedState.GenerateNormals)
    {
      ComputePass4XWithNormals<IVType> worklet4(
        isoval, this->PointDims, this->CellWriteOffset, this->PointWriteOffset);
      invoke(worklet4,
             metaDataMesh2D,
             metaDataSums,
             metaDataMin,
             metaDataMax,
             metaDataNumTris,
             edgeCases,
             coordinateSystem,
             inputField,
             triangle_topology,
             sharedState.InterpolationEdgeIds,
             sharedState.InterpolationWeights,
             sharedState.CellIdMap,
             points,
             normals);
    }
    else
    {
      ComputePass4X<IVType> worklet4(
        isoval, this->PointDims, this->CellWriteOffset, this->PointWriteOffset);
      invoke(worklet4,
             metaDataMesh2D,
             metaDataSums,
             metaDataMin,
             metaDataMax,
             metaDataNumTris,
             edgeCases,
             coordinateSystem,
             inputField,
             triangle_topology,
             sharedState.InterpolationEdgeIds,
             sharedState.InterpolationWeights,
             sharedState.CellIdMap,
             points);
    }

    return true;
  }

  template <typename DeviceAdapterTag,
            typename IVType,
            typename T,
            typename CoordsType,
            typename StorageTagField,
            typename MeshSums,
            typename PointType,
            typename NormalType>
  VISKORES_CONT bool LaunchYAxis(
    DeviceAdapterTag device,
    viskores::Id newPointSize,
    IVType isoval,
    CoordsType coordinateSystem,
    const viskores::cont::ArrayHandle<T, StorageTagField>& inputField,
    viskores::cont::ArrayHandle<viskores::UInt8> edgeCases,
    viskores::cont::CellSetStructured<2>& metaDataMesh2D,
    const MeshSums& metaDataSums,
    const viskores::cont::ArrayHandle<viskores::Id>& metaDataMin,
    const viskores::cont::ArrayHandle<viskores::Id>& metaDataMax,
    const viskores::cont::ArrayHandle<viskores::Int32>& metaDataNumTris,
    viskores::worklet::contour::CommonState& sharedState,
    viskores::cont::ArrayHandle<viskores::Id>& triangle_topology,
    PointType& points,
    NormalType& normals) const
  {
    viskores::cont::Invoker invoke(device);

    ComputePass4Y<IVType> worklet4(
      isoval, this->PointDims, this->CellWriteOffset, this->PointWriteOffset);
    invoke(worklet4,
           metaDataMesh2D,
           metaDataSums,
           metaDataMin,
           metaDataMax,
           metaDataNumTris,
           edgeCases,
           inputField,
           triangle_topology,
           sharedState.InterpolationEdgeIds,
           sharedState.InterpolationWeights,
           sharedState.CellIdMap);

    //This needs to be done on array handle view ( start = this->PointWriteOffset, len = newPointSize)
    ComputePass5Y<IVType> worklet5(
      this->PointDims, this->PointWriteOffset, sharedState.GenerateNormals);

    invoke(worklet5,
           viskores::cont::make_ArrayHandleView(
             sharedState.InterpolationEdgeIds, this->PointWriteOffset, newPointSize),
           viskores::cont::make_ArrayHandleView(
             sharedState.InterpolationWeights, this->PointWriteOffset, newPointSize),
           viskores::cont::make_ArrayHandleView(points, this->PointWriteOffset, newPointSize),
           inputField,
           coordinateSystem,
           normals);

    return true;
  }

  template <typename DeviceAdapterTag, typename... Args>
  VISKORES_CONT bool Launch(SumXAxis, DeviceAdapterTag device, Args&&... args) const
  {
    return this->LaunchXAxis(device, std::forward<Args>(args)...);
  }

  template <typename DeviceAdapterTag, typename... Args>
  VISKORES_CONT bool Launch(SumYAxis, DeviceAdapterTag device, Args&&... args) const
  {
    return this->LaunchYAxis(device, std::forward<Args>(args)...);
  }

  template <typename DeviceAdapterTag, typename... Args>
  VISKORES_CONT bool operator()(DeviceAdapterTag device, Args&&... args) const
  {
    return this->Launch(
      typename select_AxisToSum<DeviceAdapterTag>::type{}, device, std::forward<Args>(args)...);
  }
};
}
}
}
#endif
