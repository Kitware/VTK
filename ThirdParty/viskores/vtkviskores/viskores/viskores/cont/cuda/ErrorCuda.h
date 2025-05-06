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
#ifndef viskores_cont_cuda_ErrorCuda_h
#define viskores_cont_cuda_ErrorCuda_h

#include <viskores/Types.h>
#include <viskores/cont/Error.h>

#include <cuda.h>

#include <sstream>

/// A macro that can be used to check to see if there are any unchecked
/// CUDA errors. Will throw an ErrorCuda if there are.
///
#define VISKORES_CUDA_CHECK_ASYNCHRONOUS_ERROR()                                              \
  VISKORES_SWALLOW_SEMICOLON_PRE_BLOCK                                                        \
  {                                                                                           \
    const cudaError_t viskores_cuda_check_async_error = cudaGetLastError();                   \
    if (viskores_cuda_check_async_error != cudaSuccess)                                       \
    {                                                                                         \
      throw ::viskores::cont::cuda::ErrorCuda(                                                \
        viskores_cuda_check_async_error, __FILE__, __LINE__, "Unchecked asynchronous error"); \
    }                                                                                         \
  }                                                                                           \
  VISKORES_SWALLOW_SEMICOLON_POST_BLOCK

/// A macro that can be wrapped around a CUDA command and will throw an
/// ErrorCuda exception if the CUDA command fails.
///
#define VISKORES_CUDA_CALL(command)                              \
  VISKORES_CUDA_CHECK_ASYNCHRONOUS_ERROR();                      \
  VISKORES_SWALLOW_SEMICOLON_PRE_BLOCK                           \
  {                                                              \
    const cudaError_t viskores_cuda_call_error = command;        \
    if (viskores_cuda_call_error != cudaSuccess)                 \
    {                                                            \
      throw ::viskores::cont::cuda::ErrorCuda(                   \
        viskores_cuda_call_error, __FILE__, __LINE__, #command); \
    }                                                            \
  }                                                              \
  VISKORES_SWALLOW_SEMICOLON_POST_BLOCK

namespace viskores
{
namespace cont
{
namespace cuda
{

VISKORES_SILENCE_WEAK_VTABLE_WARNING_START

/// This error is thrown whenever an unidentified CUDA runtime error is
/// encountered.
///
class VISKORES_ALWAYS_EXPORT ErrorCuda : public viskores::cont::Error
{
public:
  ErrorCuda(cudaError_t error)
  {
    std::stringstream message;
    message << "CUDA Error: " << cudaGetErrorString(error);
    this->SetMessage(message.str());
  }

  ErrorCuda(cudaError_t error,
            const std::string& file,
            viskores::Id line,
            const std::string& description)
  {
    std::stringstream message;
    message << "CUDA Error: " << cudaGetErrorString(error) << std::endl
            << description << " @ " << file << ":" << line;
    this->SetMessage(message.str());
  }
};

VISKORES_SILENCE_WEAK_VTABLE_WARNING_END
}
}
} // namespace viskores::cont:cuda

#endif //viskores_cont_cuda_ErrorCuda_h
