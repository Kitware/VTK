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
#ifndef viskores_rendering_raytracing_Ray_Operations_h
#define viskores_rendering_raytracing_Ray_Operations_h

#include <viskores/Matrix.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/raytracing/ChannelBufferOperations.h>
#include <viskores/rendering/raytracing/Ray.h>
#include <viskores/rendering/raytracing/Worklets.h>
#include <viskores/rendering/viskores_rendering_export.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

class RayStatusFilter : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldInOut);
  using ExecutionSignature = void(_1, _2);
  VISKORES_EXEC
  void operator()(const viskores::Id& hitIndex, viskores::UInt8& rayStatus) const
  {
    if (hitIndex == -1)
      rayStatus = RAY_EXITED_DOMAIN;
    else if (rayStatus != RAY_EXITED_DOMAIN && rayStatus != RAY_TERMINATED)
      rayStatus = RAY_ACTIVE;
    //else printf("Bad status state %d \n",(int)rayStatus);
  }
}; //class RayStatusFileter

class RayMapCanvas : public viskores::worklet::WorkletMapField
{
protected:
  viskores::Matrix<viskores::Float32, 4, 4> InverseProjView;
  viskores::Id Width;
  viskores::Float32 DoubleInvHeight;
  viskores::Float32 DoubleInvWidth;
  viskores::Vec3f_32 Origin;

public:
  VISKORES_CONT
  RayMapCanvas(const viskores::Matrix<viskores::Float32, 4, 4>& inverseProjView,
               const viskores::Id width,
               const viskores::Id height,
               const viskores::Vec3f_32& origin)
    : InverseProjView(inverseProjView)
    , Width(width)
    , Origin(origin)
  {
    VISKORES_ASSERT(width > 0);
    VISKORES_ASSERT(height > 0);
    DoubleInvHeight = 2.f / static_cast<viskores::Float32>(height);
    DoubleInvWidth = 2.f / static_cast<viskores::Float32>(width);
  }

  using ControlSignature = void(FieldIn, FieldInOut, FieldIn, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename Precision, typename DepthPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& pixelId,
                                Precision& maxDistance,
                                const Vec<Precision, 3>& origin,
                                const DepthPortalType& depths) const
  {
    viskores::Vec4f_32 position;
    position[0] = static_cast<viskores::Float32>(pixelId % Width);
    position[1] = static_cast<viskores::Float32>(pixelId / Width);
    position[2] = static_cast<viskores::Float32>(depths.Get(pixelId));
    position[3] = 1;
    // transform into normalized device coordinates (-1,1)
    position[0] = position[0] * DoubleInvWidth - 1.f;
    position[1] = position[1] * DoubleInvHeight - 1.f;
    position[2] = 2.f * position[2] - 1.f;
    // offset so we don't go all the way to the same point
    position[2] -= 0.00001f;
    position = viskores::MatrixMultiply(InverseProjView, position);
    viskores::Vec3f_32 p;
    p[0] = position[0] / position[3];
    p[1] = position[1] / position[3];
    p[2] = position[2] / position[3];
    p = p - origin;


    maxDistance = viskores::Magnitude(p);
  }
}; //class RayMapMinDistances

} // namespace detail
class RayOperations
{
public:
  template <typename T>
  static void ResetStatus(Ray<T>& rays, viskores::UInt8 status)
  {
    viskores::cont::ArrayHandleConstant<viskores::UInt8> statusHandle(status, rays.NumRays);
    viskores::cont::Algorithm::Copy(statusHandle, rays.Status);
  }

  //
  // Some worklets like triangle intersection do not set the
  // ray status, so this operation sets the status based on
  // the ray hit index
  //
  template <typename Device, typename T>
  static void UpdateRayStatus(Ray<T>& rays, Device)
  {
    viskores::worklet::DispatcherMapField<detail::RayStatusFilter> dispatcher{ (
      detail::RayStatusFilter{}) };
    dispatcher.SetDevice(Device());
    dispatcher.Invoke(rays.HitIdx, rays.Status);
  }

  template <typename T>
  static void UpdateRayStatus(Ray<T>& rays)
  {
    viskores::worklet::DispatcherMapField<detail::RayStatusFilter> dispatcher{ (
      detail::RayStatusFilter{}) };
    dispatcher.Invoke(rays.HitIdx, rays.Status);
  }

  VISKORES_RENDERING_EXPORT static void MapCanvasToRays(
    Ray<viskores::Float32>& rays,
    const viskores::rendering::Camera& camera,
    const viskores::rendering::CanvasRayTracer& canvas);

  template <typename T>
  static viskores::Id RaysInMesh(Ray<T>& rays)
  {
    viskores::Vec<UInt8, 2> maskValues;
    maskValues[0] = RAY_ACTIVE;
    maskValues[1] = RAY_LOST;

    viskores::cont::ArrayHandle<viskores::UInt8> masks;

    viskores::worklet::DispatcherMapField<ManyMask<viskores::UInt8, 2>> dispatcher{ (
      ManyMask<viskores::UInt8, 2>{ maskValues }) };
    dispatcher.Invoke(rays.Status, masks);
    viskores::cont::ArrayHandleCast<viskores::Id, viskores::cont::ArrayHandle<viskores::UInt8>>
      castedMasks(masks);
    const viskores::Id initVal = 0;
    viskores::Id count = viskores::cont::Algorithm::Reduce(castedMasks, initVal);

    return count;
  }

  template <typename T>
  static viskores::Id GetStatusCount(Ray<T>& rays, viskores::Id status)
  {
    viskores::UInt8 statusUInt8;
    if (status < 0 || status > 255)
    {
      throw viskores::cont::ErrorBadValue("Rays GetStatusCound: invalid status");
    }

    statusUInt8 = static_cast<viskores::UInt8>(status);
    viskores::cont::ArrayHandle<viskores::UInt8> masks;

    viskores::worklet::DispatcherMapField<Mask<viskores::UInt8>> dispatcher{ (
      Mask<viskores::UInt8>{ statusUInt8 }) };
    dispatcher.Invoke(rays.Status, masks);
    viskores::cont::ArrayHandleCast<viskores::Id, viskores::cont::ArrayHandle<viskores::UInt8>>
      castedMasks(masks);
    const viskores::Id initVal = 0;
    viskores::Id count = viskores::cont::Algorithm::Reduce(castedMasks, initVal);

    return count;
  }

  template <typename T>
  static viskores::Id RaysProcessed(Ray<T>& rays)
  {
    viskores::Vec<UInt8, 3> maskValues;
    maskValues[0] = RAY_TERMINATED;
    maskValues[1] = RAY_EXITED_DOMAIN;
    maskValues[2] = RAY_ABANDONED;

    viskores::cont::ArrayHandle<viskores::UInt8> masks;

    viskores::worklet::DispatcherMapField<ManyMask<viskores::UInt8, 3>> dispatcher{ (
      ManyMask<viskores::UInt8, 3>{ maskValues }) };
    dispatcher.Invoke(rays.Status, masks);
    viskores::cont::ArrayHandleCast<viskores::Id, viskores::cont::ArrayHandle<viskores::UInt8>>
      castedMasks(masks);
    const viskores::Id initVal = 0;
    viskores::Id count = viskores::cont::Algorithm::Reduce(castedMasks, initVal);

    return count;
  }

  template <typename T>
  static viskores::cont::ArrayHandle<viskores::UInt8> CompactActiveRays(Ray<T>& rays)
  {
    viskores::Vec<UInt8, 1> maskValues;
    maskValues[0] = RAY_ACTIVE;
    auto statusUInt8 = static_cast<viskores::UInt8>(RAY_ACTIVE);
    viskores::cont::ArrayHandle<viskores::UInt8> masks;

    viskores::worklet::DispatcherMapField<Mask<viskores::UInt8>> dispatcher{ (
      Mask<viskores::UInt8>{ statusUInt8 }) };
    dispatcher.Invoke(rays.Status, masks);

    viskores::cont::ArrayHandle<T> emptyHandle;

    rays.Normal =
      viskores::cont::make_ArrayHandleCompositeVector(emptyHandle, emptyHandle, emptyHandle);
    rays.Origin =
      viskores::cont::make_ArrayHandleCompositeVector(emptyHandle, emptyHandle, emptyHandle);
    rays.Dir =
      viskores::cont::make_ArrayHandleCompositeVector(emptyHandle, emptyHandle, emptyHandle);

    const viskores::Int32 numFloatArrays = 18;
    viskores::cont::ArrayHandle<T>* floatArrayPointers[numFloatArrays];
    floatArrayPointers[0] = &rays.OriginX;
    floatArrayPointers[1] = &rays.OriginY;
    floatArrayPointers[2] = &rays.OriginZ;
    floatArrayPointers[3] = &rays.DirX;
    floatArrayPointers[4] = &rays.DirY;
    floatArrayPointers[5] = &rays.DirZ;
    floatArrayPointers[6] = &rays.Distance;
    floatArrayPointers[7] = &rays.MinDistance;
    floatArrayPointers[8] = &rays.MaxDistance;

    floatArrayPointers[9] = &rays.Scalar;
    floatArrayPointers[10] = &rays.IntersectionX;
    floatArrayPointers[11] = &rays.IntersectionY;
    floatArrayPointers[12] = &rays.IntersectionZ;
    floatArrayPointers[13] = &rays.U;
    floatArrayPointers[14] = &rays.V;
    floatArrayPointers[15] = &rays.NormalX;
    floatArrayPointers[16] = &rays.NormalY;
    floatArrayPointers[17] = &rays.NormalZ;

    const int breakPoint = rays.IntersectionDataEnabled ? -1 : 9;
    for (int i = 0; i < numFloatArrays; ++i)
    {
      if (i == breakPoint)
      {
        break;
      }
      viskores::cont::ArrayHandle<T> compacted;
      viskores::cont::Algorithm::CopyIf(*floatArrayPointers[i], masks, compacted);
      *floatArrayPointers[i] = compacted;
    }

    //
    // restore the composite vectors
    //
    rays.Normal =
      viskores::cont::make_ArrayHandleCompositeVector(rays.NormalX, rays.NormalY, rays.NormalZ);
    rays.Origin =
      viskores::cont::make_ArrayHandleCompositeVector(rays.OriginX, rays.OriginY, rays.OriginZ);
    rays.Dir = viskores::cont::make_ArrayHandleCompositeVector(rays.DirX, rays.DirY, rays.DirZ);

    viskores::cont::ArrayHandle<viskores::Id> compactedHits;
    viskores::cont::Algorithm::CopyIf(rays.HitIdx, masks, compactedHits);
    rays.HitIdx = compactedHits;

    viskores::cont::ArrayHandle<viskores::Id> compactedPixels;
    viskores::cont::Algorithm::CopyIf(rays.PixelIdx, masks, compactedPixels);
    rays.PixelIdx = compactedPixels;

    viskores::cont::ArrayHandle<viskores::UInt8> compactedStatus;
    viskores::cont::Algorithm::CopyIf(rays.Status, masks, compactedStatus);
    rays.Status = compactedStatus;

    rays.NumRays = rays.Status.ReadPortal().GetNumberOfValues();

    const auto bufferCount = static_cast<size_t>(rays.Buffers.size());
    for (size_t i = 0; i < bufferCount; ++i)
    {
      ChannelBufferOperations::Compact(rays.Buffers[i], masks, rays.NumRays);
    }
    return masks;
  }

  template <typename T>
  static void Resize(Ray<T>& rays, const viskores::Int32 newSize)
  {
    if (newSize == rays.NumRays)
      return; //nothing to do

    rays.NumRays = newSize;

    if (rays.IntersectionDataEnabled)
    {
      rays.IntersectionX.Allocate(rays.NumRays);
      rays.IntersectionY.Allocate(rays.NumRays);
      rays.IntersectionZ.Allocate(rays.NumRays);

      rays.U.Allocate(rays.NumRays);
      rays.V.Allocate(rays.NumRays);
      rays.Scalar.Allocate(rays.NumRays);

      rays.NormalX.Allocate(rays.NumRays);
      rays.NormalY.Allocate(rays.NumRays);
      rays.NormalZ.Allocate(rays.NumRays);
    }

    rays.OriginX.Allocate(rays.NumRays);
    rays.OriginY.Allocate(rays.NumRays);
    rays.OriginZ.Allocate(rays.NumRays);

    rays.DirX.Allocate(rays.NumRays);
    rays.DirY.Allocate(rays.NumRays);
    rays.DirZ.Allocate(rays.NumRays);

    rays.Distance.Allocate(rays.NumRays);
    rays.MinDistance.Allocate(rays.NumRays);
    rays.MaxDistance.Allocate(rays.NumRays);
    rays.Status.Allocate(rays.NumRays);
    rays.HitIdx.Allocate(rays.NumRays);
    rays.PixelIdx.Allocate(rays.NumRays);

    for (auto&& buffer : rays.Buffers)
    {
      buffer.Resize(rays.NumRays);
    }
  }

  template <typename T>
  static void CopyDistancesToMin(Ray<T> rays, const T offset = 0.f)
  {
    viskores::worklet::DispatcherMapField<CopyAndOffsetMask<T>> dispatcher{ (
      CopyAndOffsetMask<T>{ offset, RAY_EXITED_MESH }) };
    dispatcher.Invoke(rays.Distance, rays.MinDistance, rays.Status);
  }
};
}
}
} // namespace vktm::rendering::raytracing
#endif
