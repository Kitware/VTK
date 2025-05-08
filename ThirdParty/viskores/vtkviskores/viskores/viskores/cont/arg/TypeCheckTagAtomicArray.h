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
#ifndef viskores_cont_arg_TypeCheckTagAtomicArray_h
#define viskores_cont_arg_TypeCheckTagAtomicArray_h

#include <viskores/cont/arg/TypeCheck.h>

#include <viskores/List.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/AtomicArray.h>

namespace viskores
{
namespace cont
{
namespace arg
{

/// The atomic array type check passes for an \c ArrayHandle of a structure
/// that is valid for atomic access. There are many restrictions on the
/// type of data that can be used for an atomic array.
///
struct TypeCheckTagAtomicArray;

template <typename ArrayType>
struct TypeCheck<TypeCheckTagAtomicArray, ArrayType>
{
  static constexpr bool value = false;
};

template <typename T>
struct TypeCheck<TypeCheckTagAtomicArray,
                 viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>>
{
  static constexpr bool value = viskores::ListHas<viskores::cont::AtomicArrayTypeList, T>::value;
};

}
}
} // namespace viskores::cont::arg

#endif //viskores_cont_arg_TypeCheckTagAtomicArray_h
