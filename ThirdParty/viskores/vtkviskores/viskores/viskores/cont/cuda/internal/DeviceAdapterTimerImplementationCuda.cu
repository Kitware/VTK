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
#include <viskores/cont/cuda/internal/DeviceAdapterTimerImplementationCuda.h>

#include <viskores/Types.h>
#include <viskores/cont/cuda/ErrorCuda.h>

#include <cuda.h>

namespace viskores
{
namespace cont
{

DeviceAdapterTimerImplementation<
  viskores::cont::DeviceAdapterTagCuda>::DeviceAdapterTimerImplementation()
{
  VISKORES_CUDA_CALL(cudaEventCreate(&this->StartEvent));
  VISKORES_CUDA_CALL(cudaEventCreate(&this->StopEvent));
  this->Reset();
}

DeviceAdapterTimerImplementation<
  viskores::cont::DeviceAdapterTagCuda>::~DeviceAdapterTimerImplementation()
{
  // These aren't wrapped in VISKORES_CUDA_CALL because we can't throw errors
  // from destructors. We're relying on cudaGetLastError in the
  // VISKORES_CUDA_CHECK_ASYNCHRONOUS_ERROR catching any issues from these calls
  // later.
  cudaEventDestroy(this->StartEvent);
  cudaEventDestroy(this->StopEvent);
}

void DeviceAdapterTimerImplementation<viskores::cont::DeviceAdapterTagCuda>::Reset()
{
  this->StartReady = false;
  this->StopReady = false;
}

void DeviceAdapterTimerImplementation<viskores::cont::DeviceAdapterTagCuda>::Start()
{
  VISKORES_CUDA_CALL(cudaEventRecord(this->StartEvent, cudaStreamPerThread));
  this->StartReady = true;
}

void DeviceAdapterTimerImplementation<viskores::cont::DeviceAdapterTagCuda>::Stop()
{
  VISKORES_CUDA_CALL(cudaEventRecord(this->StopEvent, cudaStreamPerThread));
  VISKORES_CUDA_CALL(cudaEventSynchronize(this->StopEvent));
  this->StopReady = true;
}

bool DeviceAdapterTimerImplementation<viskores::cont::DeviceAdapterTagCuda>::Started() const
{
  return this->StartReady;
}

bool DeviceAdapterTimerImplementation<viskores::cont::DeviceAdapterTagCuda>::Stopped() const
{
  return this->StopReady;
}

// Callbacks without a mandated order(in independent streams) execute in undefined
// order and maybe serialized. So Instead CudaEventQuery is used here.
// Ref link: https://docs.nvidia.com/cuda/cuda-driver-api/group__CUDA__STREAM.html
bool DeviceAdapterTimerImplementation<viskores::cont::DeviceAdapterTagCuda>::Ready() const
{
  if (cudaEventQuery(this->StopEvent) == cudaSuccess)
  {
    return true;
  }
  return false;
}


viskores::Float64
DeviceAdapterTimerImplementation<viskores::cont::DeviceAdapterTagCuda>::GetElapsedTime() const
{
  assert(this->StartReady);
  if (!this->StartReady)
  {
    VISKORES_LOG_F(viskores::cont::LogLevel::Error,
                   "Start() function should be called first then trying to call GetElapsedTime().");
    return 0;
  }
  if (!this->StopReady)
  {
    // Stop was not called, so we have to insert a new event into the stream
    VISKORES_CUDA_CALL(cudaEventRecord(this->StopEvent, cudaStreamPerThread));
    VISKORES_CUDA_CALL(cudaEventSynchronize(this->StopEvent));
  }

  float elapsedTimeMilliseconds;
  VISKORES_CUDA_CALL(
    cudaEventElapsedTime(&elapsedTimeMilliseconds, this->StartEvent, this->StopEvent));
  // Reset Stop flag to its original state
  return static_cast<viskores::Float64>(0.001f * elapsedTimeMilliseconds);
}
}
} // namespace viskores::cont
