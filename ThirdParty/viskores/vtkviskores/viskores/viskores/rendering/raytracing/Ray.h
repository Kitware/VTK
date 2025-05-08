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
#ifndef viskores_rendering_raytracing_Ray_h
#define viskores_rendering_raytracing_Ray_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleCompositeVector.h>
#include <viskores/rendering/raytracing/ChannelBuffer.h>

#include <vector>

#define RAY_ACTIVE 0
#define RAY_COMPLETE 1
#define RAY_TERMINATED 2
#define RAY_EXITED_MESH 3
#define RAY_EXITED_DOMAIN 4
#define RAY_LOST 5
#define RAY_ABANDONED 6
#define RAY_TUG_EPSILON 0.001

namespace viskores
{
namespace rendering
{
namespace raytracing
{

template <typename Precision>
class Ray
{
protected:
  bool IntersectionDataEnabled;

public:
  // composite vectors to hold array handles
  typename //tell the compiler we have a dependent type
    viskores::cont::ArrayHandleCompositeVector<viskores::cont::ArrayHandle<Precision>,
                                               viskores::cont::ArrayHandle<Precision>,
                                               viskores::cont::ArrayHandle<Precision>>
      Intersection;

  typename //tell the compiler we have a dependent type
    viskores::cont::ArrayHandleCompositeVector<viskores::cont::ArrayHandle<Precision>,
                                               viskores::cont::ArrayHandle<Precision>,
                                               viskores::cont::ArrayHandle<Precision>>
      Normal;

  typename //tell the compiler we have a dependent type
    viskores::cont::ArrayHandleCompositeVector<viskores::cont::ArrayHandle<Precision>,
                                               viskores::cont::ArrayHandle<Precision>,
                                               viskores::cont::ArrayHandle<Precision>>
      Origin;

  typename //tell the compiler we have a dependent type
    viskores::cont::ArrayHandleCompositeVector<viskores::cont::ArrayHandle<Precision>,
                                               viskores::cont::ArrayHandle<Precision>,
                                               viskores::cont::ArrayHandle<Precision>>
      Dir;

  viskores::cont::ArrayHandle<Precision> IntersectionX; //ray Intersection
  viskores::cont::ArrayHandle<Precision> IntersectionY;
  viskores::cont::ArrayHandle<Precision> IntersectionZ;

  viskores::cont::ArrayHandle<Precision> OriginX; //ray Origin
  viskores::cont::ArrayHandle<Precision> OriginY;
  viskores::cont::ArrayHandle<Precision> OriginZ;

  viskores::cont::ArrayHandle<Precision> DirX; //ray Dir
  viskores::cont::ArrayHandle<Precision> DirY;
  viskores::cont::ArrayHandle<Precision> DirZ;

  viskores::cont::ArrayHandle<Precision> U; //barycentric coordinates
  viskores::cont::ArrayHandle<Precision> V;
  viskores::cont::ArrayHandle<Precision> NormalX; //ray Normal
  viskores::cont::ArrayHandle<Precision> NormalY;
  viskores::cont::ArrayHandle<Precision> NormalZ;
  viskores::cont::ArrayHandle<Precision> Scalar; //scalar

  viskores::cont::ArrayHandle<Precision> Distance; //distance to hit

  viskores::cont::ArrayHandle<viskores::Id> HitIdx;
  viskores::cont::ArrayHandle<viskores::Id> PixelIdx;

  viskores::cont::ArrayHandle<Precision> MinDistance;  // distance to hit
  viskores::cont::ArrayHandle<Precision> MaxDistance;  // distance to hit
  viskores::cont::ArrayHandle<viskores::UInt8> Status; // 0 = active 1 = miss 2 = lost

  std::vector<ChannelBuffer<Precision>> Buffers;
  viskores::Id DebugWidth;
  viskores::Id DebugHeight;
  viskores::Id NumRays;

  VISKORES_CONT
  Ray()
  {
    IntersectionDataEnabled = false;
    NumRays = 0;
    Intersection =
      viskores::cont::make_ArrayHandleCompositeVector(IntersectionX, IntersectionY, IntersectionZ);
    Normal = viskores::cont::make_ArrayHandleCompositeVector(NormalX, NormalY, NormalZ);
    Origin = viskores::cont::make_ArrayHandleCompositeVector(OriginX, OriginY, OriginZ);
    Dir = viskores::cont::make_ArrayHandleCompositeVector(DirX, DirY, DirZ);

    ChannelBuffer<Precision> buffer;
    buffer.Resize(NumRays);
    Buffers.push_back(buffer);
    DebugWidth = -1;
    DebugHeight = -1;
  }

  void EnableIntersectionData()
  {
    if (IntersectionDataEnabled)
    {
      return;
    }

    IntersectionDataEnabled = true;

    IntersectionX.Allocate(NumRays);
    IntersectionY.Allocate(NumRays);
    IntersectionZ.Allocate(NumRays);

    U.Allocate(NumRays);
    V.Allocate(NumRays);
    Scalar.Allocate(NumRays);

    NormalX.Allocate(NumRays);
    NormalY.Allocate(NumRays);
    NormalZ.Allocate(NumRays);
  }

  void DisableIntersectionData()
  {
    if (!IntersectionDataEnabled)
    {
      return;
    }

    IntersectionDataEnabled = false;
    IntersectionX.ReleaseResources();
    IntersectionY.ReleaseResources();
    IntersectionZ.ReleaseResources();
    U.ReleaseResources();
    V.ReleaseResources();
    Scalar.ReleaseResources();

    NormalX.ReleaseResources();
    NormalY.ReleaseResources();
    NormalZ.ReleaseResources();
  }

  VISKORES_CONT
  void AddBuffer(const viskores::Int32 numChannels, const std::string name)
  {
    ChannelBuffer<Precision> buffer(numChannels, this->NumRays);
    buffer.SetName(name);
    this->Buffers.push_back(buffer);
  }

  VISKORES_CONT
  bool HasBuffer(const std::string name)
  {
    for (const auto& buffer : this->Buffers)
    {
      if (buffer.GetName() == name)
        return true;
    }
    return false;
  }

  VISKORES_CONT
  ChannelBuffer<Precision>& GetBuffer(const std::string name)
  {
    for (auto&& buffer : this->Buffers)
    {
      if (buffer.GetName() == name)
        return buffer;
    }

    throw viskores::cont::ErrorBadValue("No channel buffer with requested name: " + name);
  }

  void PrintRay(viskores::Id pixelId)
  {
    for (viskores::Id i = 0; i < NumRays; ++i)
    {
      if (PixelIdx.WritePortal().Get(i) == pixelId)
      {
        std::cout << "Ray " << pixelId << "\n";
        std::cout << "Origin "
                  << "[" << OriginX.WritePortal().Get(i) << "," << OriginY.WritePortal().Get(i)
                  << "," << OriginZ.WritePortal().Get(i) << "]\n";
        std::cout << "Dir "
                  << "[" << DirX.WritePortal().Get(i) << "," << DirY.WritePortal().Get(i) << ","
                  << DirZ.WritePortal().Get(i) << "]\n";
      }
    }
  }

  friend class RayOperations;
}; // class ray
}
}
} //namespace viskores::rendering::raytracing
#endif //viskores_rendering_raytracing_Ray_h
