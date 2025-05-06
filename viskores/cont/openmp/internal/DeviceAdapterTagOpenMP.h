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

#ifndef viskores_cont_openmp_internal_DeviceAdapterTagOpenMP_h
#define viskores_cont_openmp_internal_DeviceAdapterTagOpenMP_h

#include <viskores/cont/DeviceAdapterTag.h>

/// @struct viskores::cont::DeviceAdapterTagOpenMP
/// @brief Tag for a device adapter that uses OpenMP compiler extensions to
/// run algorithms on multiple threads.
///
/// For this device to work, Viskores must be configured to use OpenMP and the code
/// must be compiled with a compiler that supports OpenMP pragmas. This tag is
/// defined in `viskores/cont/openmp/DeviceAdapterOpenMP.h`.

#ifdef VISKORES_ENABLE_OPENMP
VISKORES_VALID_DEVICE_ADAPTER(OpenMP, VISKORES_DEVICE_ADAPTER_OPENMP)
#else
VISKORES_INVALID_DEVICE_ADAPTER(OpenMP, VISKORES_DEVICE_ADAPTER_OPENMP)
#endif

#endif // viskores_cont_openmp_internal_DeviceAdapterTagOpenMP_h
