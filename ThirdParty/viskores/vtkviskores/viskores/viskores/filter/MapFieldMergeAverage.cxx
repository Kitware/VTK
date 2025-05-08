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

#include <viskores/cont/Logging.h>
#include <viskores/filter/MapFieldMergeAverage.h>
#include <viskores/worklet/AverageByKey.h>

namespace
{

struct DoMapFieldMerge
{
  template <typename InputArrayType>
  void operator()(const InputArrayType& input,
                  const viskores::worklet::internal::KeysBase& keys,
                  viskores::cont::UnknownArrayHandle& output) const
  {
    using BaseComponentType = typename InputArrayType::ValueType::ComponentType;

    viskores::worklet::AverageByKey::Run(
      keys, input, output.ExtractArrayFromComponents<BaseComponentType>(viskores::CopyFlag::Off));
  }
};

} // anonymous namespace

bool viskores::filter::MapFieldMergeAverage(const viskores::cont::Field& inputField,
                                            const viskores::worklet::internal::KeysBase& keys,
                                            viskores::cont::Field& outputField)
{
  VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

  viskores::cont::UnknownArrayHandle outputArray = inputField.GetData().NewInstanceBasic();
  outputArray.Allocate(keys.GetInputRange());

  try
  {
    inputField.GetData().CastAndCallWithExtractedArray(DoMapFieldMerge{}, keys, outputArray);
    outputField =
      viskores::cont::Field(inputField.GetName(), inputField.GetAssociation(), outputArray);
    return true;
  }
  catch (...)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Warn, "Faild to map field " << inputField.GetName());
    return false;
  }
}

bool viskores::filter::MapFieldMergeAverage(const viskores::cont::Field& inputField,
                                            const viskores::worklet::internal::KeysBase& keys,
                                            viskores::cont::DataSet& outputData)
{
  VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

  viskores::cont::Field outputField;
  bool success = viskores::filter::MapFieldMergeAverage(inputField, keys, outputField);
  if (success)
  {
    outputData.AddField(outputField);
  }
  return success;
}
