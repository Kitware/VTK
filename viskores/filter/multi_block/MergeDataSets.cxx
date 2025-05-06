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

#include <viskores/cont/MergePartitionedDataSet.h>
#include <viskores/filter/multi_block/MergeDataSets.h>
namespace viskores
{
namespace filter
{
namespace multi_block
{
viskores::cont::PartitionedDataSet MergeDataSets::DoExecutePartitions(
  const viskores::cont::PartitionedDataSet& input)
{
  viskores::cont::DataSet mergedResult =
    viskores::cont::MergePartitionedDataSet(input, this->GetInvalidValue());
  return viskores::cont::PartitionedDataSet(mergedResult);
}
viskores::cont::DataSet MergeDataSets::DoExecute(const viskores::cont::DataSet&)
{
  throw viskores::cont::ErrorFilterExecution("MergeDataSets only works for a PartitionedDataSet");
}
} // namespace multi_block
} // namespace filter
} // namespace viskores
