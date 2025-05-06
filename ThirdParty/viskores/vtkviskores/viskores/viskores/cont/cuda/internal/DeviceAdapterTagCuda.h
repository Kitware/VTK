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
#ifndef viskores_cont_cuda_internal_DeviceAdapterTagCuda_h
#define viskores_cont_cuda_internal_DeviceAdapterTagCuda_h

#include <viskores/cont/DeviceAdapterTag.h>

/// @struct viskores::cont::DeviceAdapterTagCuda
/// @brief Tag for a device adapter that uses a CUDA capable GPU device.
///
/// For this device to work, Viskores must be configured to use CUDA and the code must
/// be compiled by the CUDA `nvcc` compiler. This tag is defined in
/// `viskores/cont/cuda/DeviceAdapterCuda.h`.

// We always create the cuda tag when included, but we only mark it as a valid tag when
// VISKORES_ENABLE_CUDA is true. This is for easier development of multi-backend systems.
//
// We usually mark the Cuda device as valid if VISKORES_ENABLE_CUDA even if not compiling with Cuda.
// This is because you can still call a method in a different translation unit that is compiled
// with Cuda. However, if VISKORES_NO_ERROR_ON_MIXED_CUDA_CXX_TAG is set and we are not compiling with
// Cuda, then the device is marked invalid. This is so you can specifically compile CPU stuff even
// if other units are using Cuda.
#if defined(VISKORES_ENABLE_CUDA) && !defined(VISKORES_NO_ERROR_ON_MIXED_CUDA_CXX_TAG)
VISKORES_VALID_DEVICE_ADAPTER(Cuda, VISKORES_DEVICE_ADAPTER_CUDA);
#else
VISKORES_INVALID_DEVICE_ADAPTER(Cuda, VISKORES_DEVICE_ADAPTER_CUDA);
#endif

#endif //viskores_cont_cuda_internal_DeviceAdapterTagCuda_h
