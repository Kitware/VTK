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

#ifndef viskores_Swap_h
#define viskores_Swap_h

#include <viskores/internal/ExportMacros.h>

#include <algorithm>

// Below there is some code to deal with conflicts between Viskores's Swap and the Swap that comes
// with CUDA's CUB. We need to make sure that the Swap function gets included so that our
// defensive code works either way.
#if defined(VISKORES_CUDA) && defined(VISKORES_CUDA_DEVICE_PASS) && \
  defined(VISKORES_CUDA_VERSION_MAJOR) && (VISKORES_CUDA_VERSION_MAJOR >= 12)
#include <cub/thread/thread_sort.cuh>
#endif

namespace viskores
{

/// Performs a swap operation. Safe to call from cuda code.
#if defined(VISKORES_CUDA) && defined(VISKORES_CUDA_DEVICE_PASS)
// CUDA 12 adds a `cub::Swap` function that creates ambiguity with `viskores::Swap`.
// This happens when a function from the `cub` namespace is called with an object of a class
// defined in the `viskores` namespace as an argument. If that function has an unqualified call to
// `Swap`, it results in ADL being used, causing the templated functions `cub::Swap` and
// `viskores::Swap` to conflict.
#if defined(VISKORES_CUDA_VERSION_MAJOR) && (VISKORES_CUDA_VERSION_MAJOR >= 12)
using cub::Swap;
#else
template <typename T>
VISKORES_EXEC_CONT inline void Swap(T& a, T& b)
{
  T temp = a;
  a = b;
  b = temp;
}
#endif
#elif defined(VISKORES_HIP)
template <typename T>
__host__ inline void Swap(T& a, T& b)
{
  using std::swap;
  swap(a, b);
}
template <typename T>
__device__ inline void Swap(T& a, T& b)
{
  T temp = a;
  a = b;
  b = temp;
}
#else
template <typename T>
VISKORES_EXEC_CONT inline void Swap(T& a, T& b)
{
  using std::swap;
  swap(a, b);
}
#endif

} // end namespace viskores

#endif //viskores_Swap_h
