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
#ifndef viskores_cont_kokkos_DeviceAdapterKokkos_h
#define viskores_cont_kokkos_DeviceAdapterKokkos_h

#include <viskores/cont/kokkos/internal/DeviceAdapterTagKokkos.h>

#if defined(VISKORES_ENABLE_KOKKOS)

#if !defined(VISKORES_KOKKOS_CUDA) || defined(VISKORES_CUDA)

#include <viskores/cont/kokkos/internal/DeviceAdapterAlgorithmKokkos.h>
#include <viskores/cont/kokkos/internal/DeviceAdapterMemoryManagerKokkos.h>
#include <viskores/cont/kokkos/internal/DeviceAdapterRuntimeDetectorKokkos.h>
#include <viskores/cont/kokkos/internal/RuntimeDeviceConfigurationKokkos.h>

#else // !defined(VISKORES_KOKKOS_CUDA) || defined(VISKORES_CUDA)

#if !defined(VISKORES_NO_ERROR_ON_MIXED_CUDA_CXX_TAG)
#error When Viskores is built with Kokkoas with CUDA enabled, all compilation units that include DeviceAdapterTagKokkos must use the cuda compiler
#endif // !defined(VISKORES_NO_ERROR_ON_MIXED_CUDA_CXX_TAG)

#endif // !defined(VISKORES_KOKKOS_CUDA) || defined(VISKORES_CUDA)

#endif // defined(VISKORES_ENABLE_KOKKOS)

#endif //viskores_cont_kokkos_DeviceAdapterKokkos_h
