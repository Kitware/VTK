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
#ifndef viskores_cont_cuda_interal_ThrustExecptionHandler_h
#define viskores_cont_cuda_interal_ThrustExecptionHandler_h

#include <viskores/cont/ErrorBadAllocation.h>
#include <viskores/cont/ErrorExecution.h>
#include <viskores/internal/ExportMacros.h>

#include <viskores/exec/cuda/internal/ThrustPatches.h>
VISKORES_THIRDPARTY_PRE_INCLUDE
#include <thrust/system_error.h>
VISKORES_THIRDPARTY_POST_INCLUDE

namespace viskores
{
namespace cont
{
namespace cuda
{
namespace internal
{

static inline void throwAsViskoresException()
{
  try
  {
    //re-throw the last exception
    throw;
  }
  catch (std::bad_alloc& error)
  {
    throw viskores::cont::ErrorBadAllocation(error.what());
  }
  catch (thrust::system_error& error)
  {
    throw viskores::cont::ErrorExecution(error.what());
  }
}
}
}
}
}

#endif //viskores_cont_cuda_interal_ThrustExecptionHandler_h
