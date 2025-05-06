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
#ifndef viskores_cont_kokkos_internal_KokkosTypes_h
#define viskores_cont_kokkos_internal_KokkosTypes_h

#include <viskores/cont/viskores_cont_export.h>
#include <viskores/internal/Configure.h>

VISKORES_THIRDPARTY_PRE_INCLUDE
#include <Kokkos_Core.hpp>
VISKORES_THIRDPARTY_POST_INCLUDE

namespace viskores
{
namespace cont
{
namespace kokkos
{
namespace internal
{

using ExecutionSpace = Kokkos::DefaultExecutionSpace;

VISKORES_CONT_EXPORT const ExecutionSpace& GetExecutionSpaceInstance();

template <typename ValueType>
using KokkosViewCont = Kokkos::
  View<ValueType*, Kokkos::LayoutRight, Kokkos::HostSpace, Kokkos::MemoryTraits<Kokkos::Unmanaged>>;

template <typename ValueType>
using KokkosViewExec =
  decltype(Kokkos::create_mirror(ExecutionSpace{}, KokkosViewCont<ValueType>{}));

template <typename ValueType>
using KokkosViewConstCont = typename KokkosViewCont<ValueType>::const_type;

template <typename ValueType>
using KokkosViewConstExec = typename KokkosViewExec<ValueType>::const_type;
}
}
}
} // viskores::cont::kokkos::internal

#endif // viskores_cont_kokkos_internal_KokkosTypes_h
