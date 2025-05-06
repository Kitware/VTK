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
#ifndef viskores_cont_cuda_internal_ScopedCudaStackSize_h
#define viskores_cont_cuda_internal_ScopedCudaStackSize_h

namespace viskores
{
namespace cont
{
namespace cuda
{
namespace internal
{

/// \brief RAII helper for temporarily changing CUDA stack size in an
/// exception-safe way.
struct ScopedCudaStackSize
{
  ScopedCudaStackSize(std::size_t newStackSize)
  {
    cudaDeviceGetLimit(&this->OldStackSize, cudaLimitStackSize);
    VISKORES_LOG_S(
      viskores::cont::LogLevel::Info,
      "Temporarily changing Cuda stack size from "
        << viskores::cont::GetHumanReadableSize(static_cast<viskores::UInt64>(this->OldStackSize))
        << " to "
        << viskores::cont::GetHumanReadableSize(static_cast<viskores::UInt64>(newStackSize)));
    cudaDeviceSetLimit(cudaLimitStackSize, newStackSize);
  }

  ~ScopedCudaStackSize()
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   "Restoring Cuda stack size to " << viskores::cont::GetHumanReadableSize(
                     static_cast<viskores::UInt64>(this->OldStackSize)));
    cudaDeviceSetLimit(cudaLimitStackSize, this->OldStackSize);
  }

  // Disable copy
  ScopedCudaStackSize(const ScopedCudaStackSize&) = delete;
  ScopedCudaStackSize& operator=(const ScopedCudaStackSize&) = delete;

private:
  std::size_t OldStackSize;
};
}
}
}
} // viskores::cont::cuda::internal

#endif // viskores_cont_cuda_internal_ScopedCudaStackSize_h
