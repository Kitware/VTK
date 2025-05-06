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
#ifndef viskores_filter_multi_block_MergeDataSets_h
#define viskores_filter_multi_block_MergeDataSets_h

#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/Filter.h>
#include <viskores/filter/multi_block/viskores_filter_multi_block_export.h>

namespace viskores
{
namespace filter
{
namespace multi_block
{
/// @brief Merging multiple data sets into one data set.
///
/// This filter merges multiple data sets into one data set. We assume that the input data sets
/// have the same coordinate system. If there are missing fields in a specific data set,
/// the filter uses the InvalidValue specified by the user to fill in the associated position
/// of the field array.
///
/// `MergeDataSets` is used by passing a `viskores::cont::PartitionedDataSet` to its `Execute()`
/// method. The `Execute()` will return a `viskores::cont::PartitionedDataSet` because that is
/// the common interface for all filters. However, the `viskores::cont::PartitionedDataSet` will
/// have one partition that is all the blocks merged together.
class VISKORES_FILTER_MULTI_BLOCK_EXPORT MergeDataSets : public viskores::filter::Filter
{
public:
  /// @brief Specify the value to use where field values are missing.
  ///
  /// One issue when merging blocks in a paritioned dataset is that the blocks/partitions
  /// may have different fields. That is, one partition might not have all the fields of
  /// another partition. When these partitions are merged together, the values for this
  /// missing field must be set to something. They will be set to this value, which defaults
  /// to NaN.
  void SetInvalidValue(viskores::Float64 invalidValue) { this->InvalidValue = invalidValue; };
  /// @copydoc SetInvalidValue
  viskores::Float64 GetInvalidValue() { return this->InvalidValue; }

private:
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& inputDataSet) override;

  viskores::cont::PartitionedDataSet DoExecutePartitions(
    const viskores::cont::PartitionedDataSet& input) override;

  viskores::Float64 InvalidValue = viskores::Nan64();
};
} // namespace multi_block
} // namesapce filter
} // namespace viskores

#endif //viskores_filter_multi_block_MergeDataSets_h
