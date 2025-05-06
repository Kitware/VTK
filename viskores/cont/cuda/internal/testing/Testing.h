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
#ifndef viskores_cont_cuda_internal_Testing_h
#define viskores_cont_cuda_internal_Testing_h

#include <viskores/cont/testing/Testing.h>

#include <cuda.h>

namespace viskores
{
namespace cont
{
namespace cuda
{
namespace internal
{

struct Testing
{
public:
  static VISKORES_CONT int CheckCudaBeforeExit(int result)
  {
    cudaError_t cudaError = cudaPeekAtLastError();
    if (cudaError != cudaSuccess)
    {
      std::cout << "***** Unchecked Cuda error." << std::endl
                << cudaGetErrorString(cudaError) << std::endl;
      return 1;
    }
    else
    {
      std::cout << "No Cuda error detected." << std::endl;
    }
    return result;
  }

  template <class Func>
  static VISKORES_CONT int Run(Func function)
  {
    int result = viskores::cont::testing::Testing::Run(function);
    return CheckCudaBeforeExit(result);
  }
};
}
}
}
} // namespace viskores::cont::cuda::internal

#endif //viskores_cont_cuda_internal_Testing_h
