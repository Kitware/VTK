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
#ifndef viskores_exec_cuda_internal_ArrayPortalBasicCuda_h
#define viskores_exec_cuda_internal_ArrayPortalBasicCuda_h

// This file provides specializations of ArrayPortalBasic that use texture loading
// intrinsics to load data from arrays faster in read-only arrays. These intrinsics
// are only available with compute capabilities >= 3.5, so only compile this code if
// we are compiling for that.
#if __CUDA_ARCH__ >= 350

#include <viskores/Types.h>

namespace viskores
{
namespace internal
{
namespace detail
{

// Forward declaration (declared in viskores/internal/ArrayPortalBasic.h)
template <typename T>
VISKORES_EXEC_CONT static inline T ArrayPortalBasicReadGet(const T* const data);

// Use the __ldg intrinsic to load read-only arrays through texture reads.
// Currently CUDA doesn't support texture loading of signed char's so that is why
// You don't see viskores::Int8 in any of the lists.

VISKORES_EXEC_CONT static inline viskores::UInt8 ArrayPortalBasicReadGet(
  const viskores::UInt8* const data)
{
  return __ldg(data);
}
VISKORES_EXEC_CONT static inline viskores::Int16 ArrayPortalBasicReadGet(
  const viskores::Int16* const data)
{
  return __ldg(data);
}
VISKORES_EXEC_CONT static inline viskores::UInt16 ArrayPortalBasicReadGet(
  const viskores::UInt16* const data)
{
  return __ldg(data);
}
VISKORES_EXEC_CONT static inline viskores::Int32 ArrayPortalBasicReadGet(
  const viskores::Int32* const data)
{
  return __ldg(data);
}
VISKORES_EXEC_CONT static inline viskores::UInt32 ArrayPortalBasicReadGet(
  const viskores::UInt32* const data)
{
  return __ldg(data);
}
VISKORES_EXEC_CONT static inline viskores::Float32 ArrayPortalBasicReadGet(
  const viskores::Float32* const data)
{
  return __ldg(data);
}
VISKORES_EXEC_CONT static inline viskores::Float64 ArrayPortalBasicReadGet(
  const viskores::Float64* const data)
{
  return __ldg(data);
}

// CUDA can do some vector texture loads, but only for its own types, so we have to convert
// to the CUDA type first.

VISKORES_EXEC_CONT static inline viskores::Vec2i_32 ArrayPortalBasicReadGet(
  const viskores::Vec2i_32* const data)
{
  const int2 temp = __ldg(reinterpret_cast<const int2*>(data));
  return viskores::Vec2i_32(temp.x, temp.y);
}
VISKORES_EXEC_CONT static inline viskores::Vec2ui_32 ArrayPortalBasicReadGet(
  const viskores::Vec2ui_32* const data)
{
  const uint2 temp = __ldg(reinterpret_cast<const uint2*>(data));
  return viskores::Vec2ui_32(temp.x, temp.y);
}
VISKORES_EXEC_CONT static inline viskores::Vec2f_32 ArrayPortalBasicReadGet(
  const viskores::Vec2f_32* const data)
{
  const float2 temp = __ldg(reinterpret_cast<const float2*>(data));
  return viskores::Vec2f_32(temp.x, temp.y);
}
VISKORES_EXEC_CONT static inline viskores::Vec2f_64 ArrayPortalBasicReadGet(
  const viskores::Vec2f_64* const data)
{
  const double2 temp = __ldg(reinterpret_cast<const double2*>(data));
  return viskores::Vec2f_64(temp.x, temp.y);
}

VISKORES_EXEC_CONT static inline viskores::Vec4i_32 ArrayPortalBasicReadGet(
  const viskores::Vec4i_32* const data)
{
  const int4 temp = __ldg(reinterpret_cast<const int4*>(data));
  return viskores::Vec4i_32(temp.x, temp.y, temp.z, temp.w);
}
VISKORES_EXEC_CONT static inline viskores::Vec4ui_32 ArrayPortalBasicReadGet(
  const viskores::Vec4ui_32* const data)
{
  const uint4 temp = __ldg(reinterpret_cast<const uint4*>(data));
  return viskores::Vec4ui_32(temp.x, temp.y, temp.z, temp.w);
}
VISKORES_EXEC_CONT static inline viskores::Vec4f_32 ArrayPortalBasicReadGet(
  const viskores::Vec4f_32* const data)
{
  const float4 temp = __ldg(reinterpret_cast<const float4*>(data));
  return viskores::Vec4f_32(temp.x, temp.y, temp.z, temp.w);
}

// CUDA does not support loading many of the vector types we use including 3-wide vectors.
// Support these using multiple scalar loads.

template <typename T, viskores::IdComponent N>
VISKORES_EXEC_CONT static inline viskores::Vec<T, N> ArrayPortalBasicReadGet(
  const viskores::Vec<T, N>* const data)
{
  const T* recastedData = reinterpret_cast<const T*>(data);
  viskores::Vec<T, N> result;
#pragma unroll
  for (viskores::IdComponent i = 0; i < N; ++i)
  {
    result[i] = ArrayPortalBasicReadGet(recastedData + i);
  }
  return result;
}
}
}
} // namespace viskores::internal::detail

#endif // __CUDA_ARCH__ >= 350

#endif //viskores_exec_cuda_internal_ArrayPortalBasicCuda_h
