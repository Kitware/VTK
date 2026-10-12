// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2019 Sandia Corporation.
// SPDX-FileCopyrightText: Copyright 2019 UT-Battelle, LLC.
// SPDX-FileCopyrightText: Copyright 2019 Los Alamos National Security.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-LANL-USGov

#include "vtkmDataArrayUtilities.h"
#include "vtkmConfigCore.h"

#include "viskores/cont/DeviceAdapterTag.h"
#include "viskores/cont/RuntimeDeviceTracker.h"

#ifdef VISKORES_ENABLE_CUDA
#include <cuda_runtime_api.h>
#endif // VISKORES_ENABLE_CUDA

#ifdef VISKORES_ENABLE_KOKKOS
#include <Kokkos_Core.hpp>
#ifdef KOKKOS_ENABLE_SYCL
#include "vtkmlib/vtkmInitializer.h"
#include <sycl/ext/oneapi/backend/level_zero.hpp>
#include <viskores/cont/kokkos/DeviceAdapterKokkos.h>
#endif
#endif // VISKORES_ENABLE_KOKKOS

VTK_ABI_NAMESPACE_BEGIN
#if defined(VISKORES_ENABLE_KOKKOS) && defined(KOKKOS_ENABLE_SYCL)
namespace
{
// The queue Viskores' Kokkos device runs on, or nullptr when it is not a
// Level Zero queue. Kokkos is brought up first if nothing has done so yet;
// asking the tracker is what makes Viskores initialize its devices.
sycl::queue* LevelZeroQueue()
{
  vtkmInitializer initialize;
  (void)initialize;
  if (!viskores::cont::GetRuntimeDeviceTracker().CanRunOn(
        viskores::cont::DeviceAdapterTagKokkos{}) ||
    !Kokkos::is_initialized())
  {
    return nullptr;
  }
  sycl::queue& queue = Kokkos::SYCL().sycl_queue();
  if (queue.get_backend() != sycl::backend::ext_oneapi_level_zero)
  {
    return nullptr;
  }
  return &queue;
}
}
#endif

//-----------------------------------------------------------------------------
bool vtkmDataArrayUtilities::IsDeviceAdapterAvailable(int8_t deviceAdapterId)
{
  auto deviceAdapter = viskores::cont::make_DeviceAdapterId(deviceAdapterId);
  viskores::cont::RuntimeDeviceTracker& runtimeTracker = viskores::cont::GetRuntimeDeviceTracker();
  return runtimeTracker.CanRunOn(deviceAdapter);
}

//-----------------------------------------------------------------------------
bool vtkmDataArrayUtilities::IsDevicePointer(const void* ptr, int8_t& id)
{
#ifdef VISKORES_ENABLE_CUDA
  id = VISKORES_DEVICE_ADAPTER_CUDA;
  return vtkmDataArrayUtilities::IsCudaDevicePointer(ptr);
#elif defined(VISKORES_ENABLE_KOKKOS)
  id = VISKORES_DEVICE_ADAPTER_KOKKOS;
#ifdef VISKORES_KOKKOS_CUDA
  return vtkmDataArrayUtilities::IsCudaDevicePointer(ptr);
#elif defined(VISKORES_KOKKOS_HIP)
  return vtkmDataArrayUtilities::IsHipDevicePointer(ptr);
#elif defined(KOKKOS_ENABLE_SYCL)
  return vtkmDataArrayUtilities::IsLevelZeroDevicePointer(ptr);
#else
#warning "Device pointers are not correctly detected"
  (void)ptr;
  return false;
#endif
#else
  (void)ptr;
  id = VISKORES_DEVICE_ADAPTER_SERIAL;
  return false;
#endif
}

//-----------------------------------------------------------------------------
bool vtkmDataArrayUtilities::IsCudaDevicePointer(const void* ptr)
{
#if defined(VISKORES_ENABLE_CUDA) || defined(VISKORES_KOKKOS_CUDA)
  cudaPointerAttributes atts;
  const cudaError_t perr = cudaPointerGetAttributes(&atts, ptr);
  // clear last error so other error checking does not pick it up
  cudaError_t error = cudaGetLastError();
  (void)error;
  bool isCudaDevice = perr == cudaSuccess &&
    (atts.type == cudaMemoryTypeDevice || atts.type == cudaMemoryTypeManaged);
  return isCudaDevice;
#else
  (void)ptr;
  return false;
#endif
}

//----------------------------------------------------------------------------
bool vtkmDataArrayUtilities::IsHipDevicePointer(const void* ptr)
{
#ifdef VISKORES_KOKKOS_HIP
  hipPointerAttribute_t atts;
  const hipError_t perr = hipPointerGetAttributes(&atts, ptr);
  // clear last error so other error checking does not pick it up
  hipError_t error = hipGetLastError();
  (void)error;
  return (perr == hipSuccess) &&
    ((atts.type == hipMemoryTypeDevice) || (atts.type == hipMemoryTypeManaged) ||
      (atts.type == hipMemoryTypeUnified));
#else
  (void)ptr;
  return false;
#endif
}

//----------------------------------------------------------------------------
bool vtkmDataArrayUtilities::IsLevelZeroDevicePointer(const void* ptr)
{
#if defined(VISKORES_ENABLE_KOKKOS) && defined(KOKKOS_ENABLE_SYCL)
  sycl::queue* queue = LevelZeroQueue();
  if (!queue || !ptr)
  {
    return false;
  }
  const sycl::usm::alloc kind = sycl::get_pointer_type(ptr, queue->get_context());
  return kind == sycl::usm::alloc::device || kind == sycl::usm::alloc::shared;
#else
  (void)ptr;
  return false;
#endif
}

//----------------------------------------------------------------------------
bool vtkmDataArrayUtilities::GetLevelZeroHandles(void*& driver, void*& device, void*& context)
{
  driver = nullptr;
  device = nullptr;
  context = nullptr;
#if defined(VISKORES_ENABLE_KOKKOS) && defined(KOKKOS_ENABLE_SYCL)
  sycl::queue* queue = LevelZeroQueue();
  if (!queue)
  {
    return false;
  }
  const sycl::device syclDevice = queue->get_device();
  driver = sycl::get_native<sycl::backend::ext_oneapi_level_zero>(syclDevice.get_platform());
  device = sycl::get_native<sycl::backend::ext_oneapi_level_zero>(syclDevice);
  context = sycl::get_native<sycl::backend::ext_oneapi_level_zero>(queue->get_context());
  return driver && device && context;
#else
  return false;
#endif
}

//----------------------------------------------------------------------------
const char* vtkmDataArrayUtilities::GetDeviceMemorySpaceName()
{
#if defined(VISKORES_ENABLE_CUDA) || defined(VISKORES_KOKKOS_CUDA)
  return "cuda";
#elif defined(VISKORES_KOKKOS_HIP)
  return "hip";
#elif defined(VISKORES_ENABLE_KOKKOS) && defined(KOKKOS_ENABLE_SYCL)
  return LevelZeroQueue() ? "level_zero" : nullptr;
#else
  return nullptr;
#endif
}

VTK_ABI_NAMESPACE_END
