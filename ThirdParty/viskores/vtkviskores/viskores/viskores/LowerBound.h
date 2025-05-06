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

#ifndef viskores_LowerBound_h
#define viskores_LowerBound_h

#include <viskores/BinaryPredicates.h>
#include <viskores/cont/ArrayPortalToIterators.h>

#include <viskores/internal/Configure.h>

#include <algorithm>
#include <iterator>

namespace viskores
{

/// Implementation of std::lower_bound that is appropriate
/// for both control and execution environments.
/// The overloads that take portals return indices instead of iterators.
/// @{
template <typename IterT, typename T, typename Comp>
VISKORES_EXEC_CONT IterT LowerBound(IterT first, IterT last, const T& val, Comp comp)
{
#if defined(VISKORES_CUDA) || defined(VISKORES_HIP)
  auto len = last - first;
  while (len != 0)
  {
    const auto halfLen = len / 2;
    IterT mid = first + halfLen;
    if (comp(*mid, val))
    {
      first = mid + 1;
      len -= halfLen + 1;
    }
    else
    {
      len = halfLen;
    }
  }
  return first;
#else  // VISKORES_CUDA || VISKORES_HIP
  return std::lower_bound(first, last, val, std::move(comp));
#endif // VISKORES_CUDA || VISKORES_HIP
}

template <typename IterT, typename T>
VISKORES_EXEC_CONT IterT LowerBound(IterT first, IterT last, const T& val)
{
  return viskores::LowerBound(first, last, val, viskores::SortLess{});
}

template <typename PortalT, typename T, typename Comp>
VISKORES_EXEC_CONT viskores::Id LowerBound(const PortalT& portal, const T& val, Comp comp)
{
  auto first = viskores::cont::ArrayPortalToIteratorBegin(portal);
  auto last = viskores::cont::ArrayPortalToIteratorEnd(portal);
  auto result = viskores::LowerBound(first, last, val, comp);
  return static_cast<viskores::Id>(result - first);
}

template <typename PortalT, typename T>
VISKORES_EXEC_CONT viskores::Id LowerBound(const PortalT& portal, const T& val)
{
  auto first = viskores::cont::ArrayPortalToIteratorBegin(portal);
  auto last = viskores::cont::ArrayPortalToIteratorEnd(portal);
  auto result = viskores::LowerBound(first, last, val, viskores::SortLess{});
  return static_cast<viskores::Id>(result - first);
}
/// @}

} // end namespace viskores

#endif // viskores_LowerBound_h
