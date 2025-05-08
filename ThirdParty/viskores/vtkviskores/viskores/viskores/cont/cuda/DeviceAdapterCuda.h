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
#ifndef viskores_cont_cuda_DeviceAdapterCuda_h
#define viskores_cont_cuda_DeviceAdapterCuda_h

#include <viskores/cont/cuda/internal/DeviceAdapterTagCuda.h>

#ifdef VISKORES_ENABLE_CUDA

#ifdef VISKORES_CUDA

//This is required to be first so that we get patches for thrust included
//in the correct order
#include <viskores/exec/cuda/internal/ThrustPatches.h>

#include <viskores/cont/cuda/internal/DeviceAdapterAlgorithmCuda.h>
#include <viskores/cont/cuda/internal/DeviceAdapterMemoryManagerCuda.h>
#include <viskores/cont/cuda/internal/RuntimeDeviceConfigurationCuda.h>

#else // !VISKORES_CUDA

#ifndef VISKORES_NO_ERROR_ON_MIXED_CUDA_CXX_TAG
#error When Viskores is built with CUDA enabled all compilation units that include DeviceAdapterTagCuda must use the cuda compiler
#endif //!VISKORES_NO_ERROR_ON_MIXED_CUDA_CXX_TAG

#endif // !VISKORES_CUDA

#endif // VISKORES_ENABLE_CUDA

#endif //viskores_cont_cuda_DeviceAdapterCuda_h
