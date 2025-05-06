//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef lcl_internal_Config_h
#define lcl_internal_Config_h

#include <cstdint>
#include <type_traits>

#if defined(__CUDACC__)
# define LCL_EXEC __device__ __host__
#elif defined(__HIP__)
#include "hip/hip_runtime.h" //required for __device__ __host__
# define LCL_EXEC __device__ __host__
#else
# define LCL_EXEC
#endif

namespace lcl
{

namespace internal
{
template <typename T>
using ClosestFloatType =
  typename std::enable_if<std::is_arithmetic<T>::value,
                          typename std::conditional<sizeof(T) <= 4, float, double>::type>::type;
}

using IdShape = std::int8_t;
using IdComponent = std::int32_t;

} // namespace lcl

#endif // lcl_internal_Config_h
