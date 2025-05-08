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
#ifndef viskores_rendering_raytracing_RayTracingTypeDefs_h
#define viskores_rendering_raytracing_RayTracingTypeDefs_h

#include <type_traits>
#include <viskores/List.h>
#include <viskores/Math.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleCompositeVector.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/DeviceAdapterList.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/TryExecute.h>
#include <viskores/cont/UncertainArrayHandle.h>

namespace viskores
{
namespace rendering
{
// A more useful  bounds check that tells you where it happened
#ifndef NDEBUG
#define BOUNDS_CHECK(HANDLE, INDEX) BoundsCheck((HANDLE), (INDEX), __FILE__, __LINE__)
#else
#define BOUNDS_CHECK(HANDLE, INDEX)
#endif

template <typename ArrayHandleType>
VISKORES_EXEC inline void BoundsCheck(const ArrayHandleType& handle,
                                      const viskores::Id& index,
                                      const char* file,
                                      int line)
{
  if (index < 0 || index >= handle.GetNumberOfValues())
    printf("Bad Index %d  at file %s line %d\n", (int)index, file, line);
}

namespace raytracing
{
template <typename T>
VISKORES_EXEC_CONT inline void GetInfinity(T& viskoresNotUsed(infinity));

template <>
VISKORES_EXEC_CONT inline void GetInfinity<viskores::Float32>(viskores::Float32& infinity)
{
  infinity = viskores::Infinity32();
}

template <>
VISKORES_EXEC_CONT inline void GetInfinity<viskores::Float64>(viskores::Float64& infinity)
{
  infinity = viskores::Infinity64();
}

template <typename Device>
inline std::string GetDeviceString(Device);

template <>
inline std::string GetDeviceString<viskores::cont::DeviceAdapterTagSerial>(
  viskores::cont::DeviceAdapterTagSerial)
{
  return "serial";
}

template <>
inline std::string GetDeviceString<viskores::cont::DeviceAdapterTagTBB>(
  viskores::cont::DeviceAdapterTagTBB)
{
  return "tbb";
}

template <>
inline std::string GetDeviceString<viskores::cont::DeviceAdapterTagOpenMP>(
  viskores::cont::DeviceAdapterTagOpenMP)
{
  return "openmp";
}

template <>
inline std::string GetDeviceString<viskores::cont::DeviceAdapterTagCuda>(
  viskores::cont::DeviceAdapterTagCuda)
{
  return "cuda";
}

template <>
inline std::string GetDeviceString<viskores::cont::DeviceAdapterTagKokkos>(
  viskores::cont::DeviceAdapterTagKokkos)
{
  return "kokkos";
}

struct DeviceStringFunctor
{
  std::string result;
  DeviceStringFunctor()
    : result("")
  {
  }

  template <typename Device>
  VISKORES_CONT bool operator()(Device)
  {
    VISKORES_IS_DEVICE_ADAPTER_TAG(Device);
    result = GetDeviceString(Device());
    return true;
  }
};

inline std::string GetDeviceString()
{
  DeviceStringFunctor functor;
  viskores::cont::TryExecute(functor);
  return functor.result;
}

using ColorBuffer4f = viskores::cont::ArrayHandle<viskores::Vec4f_32>;
using ColorBuffer4b = viskores::cont::ArrayHandle<viskores::Vec4ui_8>;

//Defining types supported by the rendering

//vec3s
using Vec3F = viskores::Vec3f_32;
using Vec3D = viskores::Vec3f_64;
using Vec3RenderingTypes = viskores::List<Vec3F, Vec3D>;

// Scalars Types
using ScalarF = viskores::Float32;
using ScalarD = viskores::Float64;

using RayStatusType = viskores::List<viskores::UInt8>;

using ScalarRenderingTypes = viskores::List<ScalarF, ScalarD>;

VISKORES_CONT inline viskores::cont::UncertainArrayHandle<ScalarRenderingTypes,
                                                          VISKORES_DEFAULT_STORAGE_LIST>
GetScalarFieldArray(const viskores::cont::Field& field)
{
  return field.GetData().ResetTypes(ScalarRenderingTypes{}, VISKORES_DEFAULT_STORAGE_LIST{});
}

VISKORES_CONT inline viskores::cont::UncertainArrayHandle<Vec3RenderingTypes,
                                                          VISKORES_DEFAULT_STORAGE_LIST>
GetVec3FieldArray(const viskores::cont::Field& field)
{
  return field.GetData().ResetTypes(Vec3RenderingTypes{}, VISKORES_DEFAULT_STORAGE_LIST{});
}
}
}
} //namespace viskores::rendering::raytracing
#endif //viskores_rendering_raytracing_RayTracingTypeDefs_h
