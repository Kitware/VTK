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
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/UnknownCellSet.h>
#include <viskores/filter/field_conversion/CellAverage.h>
#include <viskores/filter/field_conversion/worklet/CellAverage.h>

namespace viskores
{
namespace filter
{
namespace field_conversion
{
//-----------------------------------------------------------------------------
viskores::cont::DataSet CellAverage::DoExecute(const viskores::cont::DataSet& input)
{
  const auto& field = GetFieldFromDataSet(input);
  if (!field.IsPointField())
  {
    throw viskores::cont::ErrorFilterExecution("Point field expected.");
  }

  viskores::cont::UnknownCellSet inputCellSet = input.GetCellSet();
  viskores::cont::UnknownArrayHandle inArray = field.GetData();
  viskores::cont::UnknownArrayHandle outArray = inArray.NewInstanceBasic();

  auto resolveType = [&](const auto& concrete)
  {
    using T = typename std::decay_t<decltype(concrete)>::ValueType::ComponentType;
    auto result = outArray.ExtractArrayFromComponents<T>();
    this->Invoke(viskores::worklet::CellAverage{}, inputCellSet, concrete, result);
  };
  inArray.CastAndCallWithExtractedArray(resolveType);

  std::string outputName = this->GetOutputFieldName();
  if (outputName.empty())
  {
    // Default name is name of input.
    outputName = field.GetName();
  }
  return this->CreateResultFieldCell(input, outputName, outArray);
}
} // namespace field_conversion
} // namespace filter
} // namespace viskores
