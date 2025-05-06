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
#ifndef viskores_cont_kokkos_internal_KokkosAlloc_h
#define viskores_cont_kokkos_internal_KokkosAlloc_h

#include <viskores/cont/viskores_cont_export.h>

#include <cstddef>

namespace viskores
{
namespace cont
{
namespace kokkos
{
namespace internal
{

VISKORES_CONT_EXPORT void* Allocate(std::size_t size);
VISKORES_CONT_EXPORT void Free(void* ptr);
VISKORES_CONT_EXPORT void* Reallocate(void* ptr, std::size_t newSize);

}
}
}
} // viskores::cont::kokkos::internal

#endif // viskores_cont_kokkos_internal_KokkosAlloc_h
