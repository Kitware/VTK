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
#ifndef viskores_internal_ArrayPortalHelpers_h
#define viskores_internal_ArrayPortalHelpers_h


#include <viskores/VecTraits.h>
#include <viskores/internal/ExportMacros.h>

namespace viskores
{
namespace internal
{

namespace detail
{

template <typename PortalType>
struct PortalSupportsGetsImpl
{
  template <typename U, typename S = decltype(std::declval<U>().Get(viskores::Id{}))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<PortalType>(0));
};

template <typename PortalType>
struct PortalSupportsGets3DImpl
{
  template <typename U, typename S = decltype(std::declval<U>().Get(viskores::Id3{}))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<PortalType>(0));
};

template <typename PortalType>
struct PortalSupportsSetsImpl
{
  template <typename U,
            typename S = decltype(std::declval<U>().Set(viskores::Id{},
                                                        std::declval<typename U::ValueType>()))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<PortalType>(0));
};

template <typename PortalType>
struct PortalSupportsSets3DImpl
{
  template <typename U,
            typename S = decltype(std::declval<U>().Set(viskores::Id3{},
                                                        std::declval<typename U::ValueType>()))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<PortalType>(0));
};

template <typename PortalType>
struct PortalSupportsIteratorsImpl
{
  template <typename U, typename S = decltype(std::declval<U>().GetIteratorBegin())>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<PortalType>(0));
};

} // namespace detail

template <typename PortalType>
using PortalSupportsGets =
  typename detail::PortalSupportsGetsImpl<typename std::decay<PortalType>::type>::type;

template <typename PortalType>
using PortalSupportsGets3D =
  typename detail::PortalSupportsGets3DImpl<typename std::decay<PortalType>::type>::type;

template <typename PortalType>
using PortalSupportsSets =
  typename detail::PortalSupportsSetsImpl<typename std::decay<PortalType>::type>::type;

template <typename PortalType>
using PortalSupportsSets3D =
  typename detail::PortalSupportsSets3DImpl<typename std::decay<PortalType>::type>::type;

template <typename PortalType>
using PortalSupportsIterators =
  typename detail::PortalSupportsIteratorsImpl<typename std::decay<PortalType>::type>::type;
}
} // namespace viskores::internal

#endif //viskores_internal_ArrayPortalHelpers_h
