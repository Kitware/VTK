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
#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/MergePartitionedDataSet.h>
#include <viskores/filter/contour/Slice.h>
#include <viskores/filter/contour/SliceMultiple.h>
#include <viskores/worklet/WorkletMapField.h>
namespace viskores
{
namespace filter
{
namespace contour
{
class OffsetWorklet : public viskores::worklet::WorkletMapField
{
protected:
  viskores::Id OffsetValue;

public:
  VISKORES_CONT
  OffsetWorklet(const viskores::Id offset)
    : OffsetValue(offset)
  {
  }
  typedef void ControlSignature(FieldInOut);
  typedef void ExecutionSignature(_1);
  VISKORES_EXEC void operator()(viskores::Id& value) const { value += this->OffsetValue; }
};

viskores::cont::DataSet SliceMultiple::DoExecute(const viskores::cont::DataSet& input)
{
  viskores::cont::PartitionedDataSet slices;
  //Executing Slice filter several times and merge results together
  for (viskores::IdComponent i = 0;
       i < static_cast<viskores::IdComponent>(this->FunctionList.size());
       i++)
  {
    viskores::filter::contour::Slice slice;
    slice.SetImplicitFunction(this->GetImplicitFunction(i));
    slice.SetFieldsToPass(this->GetFieldsToPass());
    auto result = slice.Execute(input);
    slices.AppendPartition(result);
  }
  if (slices.GetNumberOfPartitions() > 1)
  {
    //Since the slice filter have already selected fields
    //the mergeCountours will copy all existing fields
    viskores::cont::DataSet mergedResults =
      viskores::cont::MergePartitionedDataSet(slices, viskores::Float64(0));
    return mergedResults;
  }
  return slices.GetPartition(0);
}
} // namespace contour
} // namespace filter
} // namespace viskores
