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
#include <viskores/filter/mesh_info/CellMeasures.h>
#include <viskores/filter/mesh_info/worklet/CellMeasure.h>

namespace viskores
{
namespace filter
{
namespace mesh_info
{

//-----------------------------------------------------------------------------
VISKORES_CONT CellMeasures::CellMeasures()
{
  this->SetUseCoordinateSystemAsField(true);
  this->SetCellMeasureName("measure");
}

//-----------------------------------------------------------------------------
VISKORES_CONT CellMeasures::CellMeasures(IntegrationType m)
  : Measure(m)
{
  this->SetUseCoordinateSystemAsField(true);
  this->SetCellMeasureName("measure");
}

//-----------------------------------------------------------------------------

VISKORES_CONT viskores::cont::DataSet CellMeasures::DoExecute(const viskores::cont::DataSet& input)
{
  const auto& field = this->GetFieldFromDataSet(input);
  if (!field.IsPointField())
  {
    throw viskores::cont::ErrorFilterExecution("CellMeasures expects point field input.");
  }

  const auto& cellset = input.GetCellSet();
  viskores::cont::ArrayHandle<viskores::FloatDefault> outArray;

  auto resolveType = [&](const auto& concrete)
  { this->Invoke(viskores::worklet::CellMeasure{ this->Measure }, cellset, concrete, outArray); };
  this->CastAndCallVecField<3>(field, resolveType);

  std::string outputName = this->GetCellMeasureName();
  if (outputName.empty())
  {
    // Default name is name of input.
    outputName = "measure";
  }
  return this->CreateResultFieldCell(input, outputName, outArray);
}
} // namespace mesh_info
} // namespace filter
} // namespace viskores
