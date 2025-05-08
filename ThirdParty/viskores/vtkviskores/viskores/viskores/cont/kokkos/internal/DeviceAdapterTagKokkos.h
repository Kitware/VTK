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
#ifndef viskores_cont_kokkos_internal_DeviceAdapterTagKokkos_h
#define viskores_cont_kokkos_internal_DeviceAdapterTagKokkos_h

#include <viskores/cont/DeviceAdapterTag.h>

/// @struct viskores::cont::DeviceAdapterTagKokkos
/// @brief Tag for a device adapter that uses the Kokkos library to run algorithms in parallel.
///
/// For this device to work, Viskores must be configured to use Kokkos and the executable
/// must be linked to the Kokkos libraries. Viskores will use the default execution space
/// of the provided kokkos library build. This tag is defined in
/// `viskores/cont/kokkos/DeviceAdapterKokkos.h`.

//We always create the kokkos tag when included, but we only mark it as
//a valid tag when VISKORES_ENABLE_KOKKOS is true. This is for easier development
//of multi-backend systems
#if defined(VISKORES_ENABLE_KOKKOS) &&                           \
  ((!defined(VISKORES_KOKKOS_CUDA) || defined(VISKORES_CUDA)) || \
   !defined(VISKORES_NO_ERROR_ON_MIXED_CUDA_CXX_TAG))
VISKORES_VALID_DEVICE_ADAPTER(Kokkos, VISKORES_DEVICE_ADAPTER_KOKKOS);
#else
VISKORES_INVALID_DEVICE_ADAPTER(Kokkos, VISKORES_DEVICE_ADAPTER_KOKKOS);
#endif

#endif // viskores_cont_kokkos_internal_DeviceAdapterTagKokkos_h
