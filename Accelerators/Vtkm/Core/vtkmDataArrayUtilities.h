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
};

VTK_ABI_NAMESPACE_END
#endif
