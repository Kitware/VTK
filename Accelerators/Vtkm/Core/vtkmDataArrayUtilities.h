// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2019 Sandia Corporation.
// SPDX-FileCopyrightText: Copyright 2019 UT-Battelle, LLC.
// SPDX-FileCopyrightText: Copyright 2019 Los Alamos National Security.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-LANL-USGov
/**
 * @class   vtkmDataArrayUtilities
 * @brief   Utility functions for working with Viskores device arrays.
 *
 * This class provides utility functions to check for device adapter availability
 * and to check if a pointer is a device pointer for supported device adapters.
 */

#ifndef vtkmDataArrayUtilities_h
#define vtkmDataArrayUtilities_h

#include "vtkAcceleratorsVTKmCoreModule.h"
#include <cstdint> // for int8_t

VTK_ABI_NAMESPACE_BEGIN
class VTKACCELERATORSVTKMCORE_EXPORT vtkmDataArrayUtilities
{
public:
  /**
   * Check if a Viskores device adapter is available to use.
   */
  static bool IsDeviceAdapterAvailable(int8_t deviceAdapterId);

  /**
   * Get if the pointer is a device pointer and the device adapter id, which can one of the
   * following values:
   *
   * VISKORES_DEVICE_ADAPTER_SERIAL = 1
   * VISKORES_DEVICE_ADAPTER_CUDA = 2
   * VISKORES_DEVICE_ADAPTER_KOKKOS = 5
   */
  static bool IsDevicePointer(const void* ptr, int8_t& deviceAdapterId);

  /**
   * Check if the pointer is a CUDA device pointer.
   */
  static bool IsCudaDevicePointer(const void* ptr);

  /**
   * Check if the pointer is a HIP device pointer.
   */
  static bool IsHipDevicePointer(const void* ptr);

  /**
   * Check if the pointer is USM device or shared memory known to the SYCL
   * context Viskores runs in, on a Level Zero device.
   *
   * This distinguishes device memory from host memory. It does not prove the
   * pointer was allocated in that context: drivers are not required to tell
   * contexts apart, and Intel's does not.
   */
  static bool IsLevelZeroDevicePointer(const void* ptr);

  /**
   * The Level Zero handles behind the SYCL queue Viskores runs on: the
   * `ze_driver_handle_t`, `ze_device_handle_t` and `ze_context_handle_t`.
   *
   * A USM pointer is only meaningful inside the context it was allocated
   * from, and nothing in DLPack or vtkMemoryDescriptor carries a context. So
   * a producer that wants VTK to read its Level Zero memory allocates in
   * *this* context, obtained here once, rather than sending a context along
   * with every buffer.
   *
   * Returns false, with all three null, unless Viskores was built on Kokkos
   * with its SYCL backend and that backend is running on Level Zero. The
   * handles belong to the SYCL runtime: do not destroy them.
   */
  static bool GetLevelZeroHandles(void*& driver, void*& device, void*& context);

  /**
   * The vtkMemoryDescriptor memory space of the Viskores device that holds
   * device data in this build: "cuda", "hip" or "level_zero". Returns
   * nullptr when there is none.
   */
  static const char* GetDeviceMemorySpaceName();
};

VTK_ABI_NAMESPACE_END
#endif
