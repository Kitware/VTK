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
#ifndef viskores_cont_internal_MapArrayPermutation_h
#define viskores_cont_internal_MapArrayPermutation_h

#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/cont/viskores_cont_export.h>

namespace viskores
{
namespace cont
{
namespace internal
{

/// Used to map a permutation like that found in an ArrayHandlePermutation.
///
VISKORES_CONT_EXPORT viskores::cont::UnknownArrayHandle MapArrayPermutation(
  const viskores::cont::UnknownArrayHandle& inputArray,
  const viskores::cont::UnknownArrayHandle& permutation,
  viskores::Float64 invalidValue = viskores::Nan64());

/// Used to map a permutation array.
///
template <typename T, typename S>
viskores::cont::UnknownArrayHandle MapArrayPermutation(
  const viskores::cont::ArrayHandle<
    T,
    viskores::cont::StorageTagPermutation<viskores::cont::StorageTagBasic, S>>& inputArray,
  viskores::Float64 invalidValue = viskores::Nan64())
{
  viskores::cont::ArrayHandlePermutation<viskores::cont::ArrayHandle<viskores::Id>,
                                         viskores::cont::ArrayHandle<T, S>>
    input = inputArray;
  return MapArrayPermutation(input.GetValueArray(), input.GetIndexArray(), invalidValue);
}

}
}
} // namespace viskores::cont::internal

#endif //viskores_cont_internal_MapArrayPermutation_h
