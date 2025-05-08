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
#ifndef viskores_worklet_BoundingVolumeHierachy_h
#define viskores_worklet_BoundingVolumeHierachy_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DataSet.h>
#include <viskores/rendering/viskores_rendering_export.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

struct AABBs
{
  viskores::cont::ArrayHandle<viskores::Float32> xmins;
  viskores::cont::ArrayHandle<viskores::Float32> ymins;
  viskores::cont::ArrayHandle<viskores::Float32> zmins;
  viskores::cont::ArrayHandle<viskores::Float32> xmaxs;
  viskores::cont::ArrayHandle<viskores::Float32> ymaxs;
  viskores::cont::ArrayHandle<viskores::Float32> zmaxs;
};

//
// This is the data structure that is passed to the ray tracer.
//
class VISKORES_RENDERING_EXPORT LinearBVH
{
public:
  using InnerNodesHandle = viskores::cont::ArrayHandle<viskores::Vec4f_32>;
  using LeafNodesHandle = viskores::cont::ArrayHandle<Id>;
  AABBs AABB;
  InnerNodesHandle FlatBVH;
  LeafNodesHandle Leafs;
  viskores::Bounds TotalBounds;
  viskores::Id LeafCount;

protected:
  bool IsConstructed;
  bool CanConstruct;

public:
  LinearBVH();

  VISKORES_CONT
  LinearBVH(AABBs& aabbs);

  VISKORES_CONT
  LinearBVH(const LinearBVH& other);

  VISKORES_CONT void Allocate(const viskores::Id& leafCount);

  VISKORES_CONT
  void Construct();

  VISKORES_CONT
  void SetData(AABBs& aabbs);

  VISKORES_CONT
  AABBs& GetAABBs();

  VISKORES_CONT
  bool GetIsConstructed() const;

  viskores::Id GetNumberOfAABBs() const;
}; // class LinearBVH
}
}
} // namespace viskores::rendering::raytracing
#endif //viskores_worklet_BoundingVolumeHierachy_h
