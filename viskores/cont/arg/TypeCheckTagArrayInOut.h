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
#ifndef viskores_cont_arg_TypeCheckTagArrayInOut_h
#define viskores_cont_arg_TypeCheckTagArrayInOut_h

#include <viskores/cont/arg/TypeCheck.h>

#include <viskores/List.h>

#include <viskores/cont/ArrayHandle.h>

#include <viskores/internal/ArrayPortalHelpers.h>

namespace viskores
{
namespace cont
{
namespace arg
{

/// The Array type check passes for any object that behaves like an
/// `ArrayHandle` class and can be passed to the ArrayInOut transport.
///
struct TypeCheckTagArrayInOut
{
};

namespace detail
{

template <typename ArrayType,
          bool IsArrayHandle = viskores::cont::internal::ArrayHandleCheck<ArrayType>::type::value>
struct IsArrayHandleInOut;

template <typename ArrayType>
struct IsArrayHandleInOut<ArrayType, true>
{
  static constexpr bool value =
    (viskores::internal::PortalSupportsGets<typename ArrayType::ReadPortalType>::value &&
     viskores::internal::PortalSupportsSets<typename ArrayType::WritePortalType>::value);
};

template <typename ArrayType>
struct IsArrayHandleInOut<ArrayType, false>
{
  static constexpr bool value = false;
};

} // namespace detail

template <typename ArrayType>
struct TypeCheck<TypeCheckTagArrayInOut, ArrayType>
{
  static constexpr bool value = detail::IsArrayHandleInOut<ArrayType>::value;
};
}
}
} // namespace viskores::cont::arg

#endif //viskores_cont_arg_TypeCheckTagArray_h
