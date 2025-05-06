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
#ifndef viskores_cont_cuda_internal_MakeThrustIterator_h
#define viskores_cont_cuda_internal_MakeThrustIterator_h

#include <viskores/exec/cuda/internal/ArrayPortalFromThrust.h>
#include <viskores/exec/cuda/internal/IteratorFromArrayPortal.h>

#include <viskores/internal/ArrayPortalBasic.h>

namespace viskores
{
namespace cont
{
namespace cuda
{
namespace internal
{
template <typename PortalType>
inline viskores::exec::cuda::internal::IteratorFromArrayPortal<PortalType> IteratorBegin(
  const PortalType& portal)
{
  viskores::exec::cuda::internal::IteratorFromArrayPortal<PortalType> iterator(portal);
  return iterator;
}

template <typename PortalType>
inline viskores::exec::cuda::internal::IteratorFromArrayPortal<PortalType> IteratorEnd(
  const PortalType& portal)
{
  viskores::exec::cuda::internal::IteratorFromArrayPortal<PortalType> iterator(portal);
  iterator += static_cast<std::ptrdiff_t>(portal.GetNumberOfValues());
  return iterator;
}

template <typename T>
inline T* IteratorBegin(const viskores::exec::cuda::internal::ArrayPortalFromThrust<T>& portal)
{
  return portal.GetIteratorBegin();
}

template <typename T>
inline T* IteratorEnd(const viskores::exec::cuda::internal::ArrayPortalFromThrust<T>& portal)
{
  return portal.GetIteratorEnd();
}

template <typename T>
inline const T* IteratorBegin(
  const viskores::exec::cuda::internal::ConstArrayPortalFromThrust<T>& portal)
{
  return portal.GetIteratorBegin();
}

template <typename T>
inline const T* IteratorEnd(
  const viskores::exec::cuda::internal::ConstArrayPortalFromThrust<T>& portal)
{
  return portal.GetIteratorEnd();
}

template <typename T>
inline T* IteratorBegin(const viskores::internal::ArrayPortalBasicWrite<T>& portal)
{
  return portal.GetIteratorBegin();
}

template <typename T>
inline T* IteratorEnd(const viskores::internal::ArrayPortalBasicWrite<T>& portal)
{
  return portal.GetIteratorEnd();
}

template <typename T>
inline const T* IteratorBegin(const viskores::internal::ArrayPortalBasicRead<T>& portal)
{
  return portal.GetIteratorBegin();
}

template <typename T>
inline const T* IteratorEnd(const viskores::internal::ArrayPortalBasicRead<T>& portal)
{
  return portal.GetIteratorEnd();
}
}
}
}

} //namespace viskores::cont::cuda::internal

#endif
