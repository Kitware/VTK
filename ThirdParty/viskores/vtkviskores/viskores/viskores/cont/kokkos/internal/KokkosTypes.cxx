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
#include <viskores/cont/kokkos/internal/KokkosTypes.h>

namespace viskores
{
namespace cont
{
namespace kokkos
{
namespace internal
{

const ExecutionSpace& GetExecutionSpaceInstance()
{
  // We use per-thread execution spaces so that the threads can execute independently without
  // requiring global synchronizations.
#if defined(VISKORES_KOKKOS_CUDA)
  static thread_local ExecutionSpace space(cudaStreamPerThread);
#else
  static thread_local ExecutionSpace space;
#endif
  return space;
}

}
}
}
} // viskores::cont::kokkos::internal
