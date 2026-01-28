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

VTK_ABI_NAMESPACE_BEGIN
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

VTK_ABI_NAMESPACE_END
