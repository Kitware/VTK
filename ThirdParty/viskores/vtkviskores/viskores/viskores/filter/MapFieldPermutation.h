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

#ifndef viskores_filter_MapFieldPermutation_h
#define viskores_filter_MapFieldPermutation_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Field.h>

#include <viskores/filter/viskores_filter_core_export.h>

namespace viskores
{
namespace filter
{

/// \brief Maps a field by permuting it by a given index array.
///
/// This method will create a new field containing the data from the provided `inputField` but
/// reorded by the given `permutation` index array. The value in the resulting field for index _i_
/// will be be a value from `inputField`, but comes from the index that comes from `permutation` at
/// position _i_. The result is placed in `outputField`.
///
/// The intention of this method is to implement the mapping of fields from the input to the output
/// in filters (many of which require this permutation of a field), but can be used in other places
/// as well.
///
/// `outputField` is set to have the same metadata as the input. If the metadata needs to change
/// (such as the name or the association) that should be done after the function returns.
///
/// This function returns whether the field was successfully permuted. If the returned result is
/// `true`, then the results in `outputField` are valid. If it is `false`, then `outputField`
/// should not be used.
///
/// If an invalid index is given in the permutation array (i.e. less than 0 or greater than the
/// size of the array), then the resulting outputField will be given `invalidValue` (converted as
/// best as possible to the correct data type).
///
VISKORES_FILTER_CORE_EXPORT VISKORES_CONT bool MapFieldPermutation(
  const viskores::cont::Field& inputField,
  const viskores::cont::ArrayHandle<viskores::Id>& permutation,
  viskores::cont::Field& outputField,
  viskores::Float64 invalidValue = viskores::Nan<viskores::Float64>());

VISKORES_FILTER_CORE_EXPORT VISKORES_CONT bool MapFieldPermutation(
  const viskores::cont::CoordinateSystem& inputCoords,
  const viskores::cont::ArrayHandle<viskores::Id>& permutation,
  viskores::cont::CoordinateSystem& outputCoords,
  viskores::Float64 invalidValue = viskores::Nan<viskores::Float64>());

/// \brief Maps a field by permuting it by a given index array.
///
/// This method will create a new field containing the data from the provided `inputField` but
/// reorded by the given `permutation` index array. The value in the resulting field for index _i_
/// will be be a value from `inputField`, but comes from the index that comes from `permutation` at
/// position _i_.
///
/// The intention of this method is to implement the `MapFieldOntoOutput` methods in filters (many
/// of which require this permutation of a field), but can be used in other places as well. The
/// resulting field is put in the given `DataSet`.
///
/// The returned `Field` has the same metadata as the input. If the metadata needs to change (such
/// as the name or the association), then a different form of `MapFieldPermutation` should be used.
///
/// This function returns whether the field was successfully permuted. If the returned result is
/// `true`, then `outputData` has the permuted field. If it is `false`, then the field is not
/// placed in `outputData`.
///
/// If an invalid index is given in the permutation array (i.e. less than 0 or greater than the
/// size of the array), then the resulting outputField will be given `invalidValue` (converted as
/// best as possible to the correct data type).
///
VISKORES_FILTER_CORE_EXPORT VISKORES_CONT bool MapFieldPermutation(
  const viskores::cont::Field& inputField,
  const viskores::cont::ArrayHandle<viskores::Id>& permutation,
  viskores::cont::DataSet& outputData,
  viskores::Float64 invalidValue = viskores::Nan<viskores::Float64>());
}
} // namespace viskores::filter

#endif //viskores_filter_MapFieldPermutation_h
