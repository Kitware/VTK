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
#ifndef viskores_cont_cuda_internal_DeviceAdapterTimerImplementationCuda_h
#define viskores_cont_cuda_internal_DeviceAdapterTimerImplementationCuda_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/Types.h>

#include <viskores/cont/DeviceAdapterAlgorithm.h>
#include <viskores/cont/cuda/internal/DeviceAdapterTagCuda.h>

#include <cuda.h>

namespace viskores
{
namespace cont
{

///
/// Specialization of DeviceAdapterTimerImplementation for CUDA
/// CUDA contains its own high resolution timer that are able
/// to track how long it takes to execute async kernels.
/// If we simply measured time on the CPU it would incorrectly
/// just capture how long it takes to launch a kernel.
template <>
class VISKORES_CONT_EXPORT DeviceAdapterTimerImplementation<viskores::cont::DeviceAdapterTagCuda>
{
public:
  VISKORES_CONT DeviceAdapterTimerImplementation();

  VISKORES_CONT ~DeviceAdapterTimerImplementation();

  VISKORES_CONT void Reset();

  VISKORES_CONT void Start();

  VISKORES_CONT void Stop();

  VISKORES_CONT bool Started() const;

  VISKORES_CONT bool Stopped() const;

  VISKORES_CONT bool Ready() const;

  VISKORES_CONT viskores::Float64 GetElapsedTime() const;

private:
  // Copying CUDA events is problematic.
  DeviceAdapterTimerImplementation(
    const DeviceAdapterTimerImplementation<viskores::cont::DeviceAdapterTagCuda>&) = delete;
  void operator=(const DeviceAdapterTimerImplementation<viskores::cont::DeviceAdapterTagCuda>&) =
    delete;

  bool StartReady;
  bool StopReady;
  cudaEvent_t StartEvent;
  cudaEvent_t StopEvent;
};
}
} // namespace viskores::cont


#endif
