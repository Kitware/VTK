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
#include <viskores/cont/CellSetExtrude.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/UncertainCellSet.h>
#include <viskores/cont/UnknownCellSet.h>
#include <viskores/filter/field_conversion/PointAverage.h>
#include <viskores/filter/field_conversion/worklet/PointAverage.h>

namespace viskores
{
namespace filter
{
namespace field_conversion
{
viskores::cont::DataSet PointAverage::DoExecute(const viskores::cont::DataSet& input)
{
  const auto& field = GetFieldFromDataSet(input);
  if (!field.IsCellField())
  {
    throw viskores::cont::ErrorFilterExecution("Cell field expected.");
  }

  viskores::cont::UnknownCellSet cellSet = input.GetCellSet();
  viskores::cont::UnknownArrayHandle inArray = field.GetData();
  viskores::cont::UnknownArrayHandle outArray = inArray.NewInstanceBasic();

  auto resolveType = [&](const auto& concrete)
  {
    using T = typename std::decay_t<decltype(concrete)>::ValueType::ComponentType;
    auto result = outArray.ExtractArrayFromComponents<T>();
    using SupportedCellSets = viskores::ListAppend<viskores::List<viskores::cont::CellSetExtrude>,
                                                   VISKORES_DEFAULT_CELL_SET_LIST>;

    this->Invoke(viskores::worklet::PointAverage{},
                 cellSet.ResetCellSetList<SupportedCellSets>(),
                 concrete,
                 result);
  };
  // TODO: Do we need to deal with XCG storage type (viskores::cont::ArrayHandleXGCCoordinates)
  // explicitly? Extracting from that is slow.
  inArray.CastAndCallWithExtractedArray(resolveType);

  std::string outputName = this->GetOutputFieldName();
  if (outputName.empty())
  {
    // Default name is name of input.
    outputName = field.GetName();
  }
  return this->CreateResultFieldPoint(input, outputName, outArray);
}
}
}
}
