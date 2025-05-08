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

#ifndef viskores_filter_MapFieldMergeAverage_h
#define viskores_filter_MapFieldMergeAverage_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Field.h>

#include <viskores/worklet/Keys.h>

#include <viskores/filter/viskores_filter_core_export.h>

namespace viskores
{
namespace filter
{

/// \brief Maps a field by merging entries based on a keys object.
///
/// This method will create a new field containing the data from the provided `inputField` but but
/// with groups of entities merged together. The input `keys` object encapsulates which elements
/// should be merged together. A group of elements merged together will be averaged. The result is
/// placed in `outputField`.
///
/// The intention of this method is to implement the `MapFieldOntoOutput` methods in filters (many
/// of which require this merge of a field), but can be used in other places as well.
///
/// `outputField` is set to have the same metadata as the input. If the metadata needs to change
/// (such as the name or the association) that should be done after the function returns.
///
/// This function returns whether the field was successfully merged. If the returned result is
/// `true`, then the results in `outputField` are valid. If it is `false`, then `outputField`
/// should not be used.
///
VISKORES_FILTER_CORE_EXPORT VISKORES_CONT bool MapFieldMergeAverage(
  const viskores::cont::Field& inputField,
  const viskores::worklet::internal::KeysBase& keys,
  viskores::cont::Field& outputField);

/// \brief Maps a field by merging entries based on a keys object.
///
/// This method will create a new field containing the data from the provided `inputField` but but
/// with groups of entities merged together. The input `keys` object encapsulates which elements
/// should be merged together. A group of elements merged together will be averaged.
///
/// The intention of this method is to implement the `MapFieldOntoOutput` methods in filters (many
/// of which require this merge of a field), but can be used in other places as well. The
/// resulting field is put in the given `DataSet`.
///
/// The returned `Field` has the same metadata as the input. If the metadata needs to change (such
/// as the name or the association), then a different form of `MapFieldMergeAverage` should be used.
///
/// This function returns whether the field was successfully merged. If the returned result is
/// `true`, then `outputData` has the merged field. If it is `false`, then the field is not
/// placed in `outputData`.
///
VISKORES_FILTER_CORE_EXPORT VISKORES_CONT bool MapFieldMergeAverage(
  const viskores::cont::Field& inputField,
  const viskores::worklet::internal::KeysBase& keys,
  viskores::cont::DataSet& outputData);
}
} // namespace viskores::filter

#endif //viskores_filter_MapFieldMergeAverage_h
