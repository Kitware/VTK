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

#include <viskores/filter/flow/Pathline.h>

namespace viskores
{
namespace filter
{
namespace flow
{

VISKORES_CONT Pathline::FieldType Pathline::GetField(const viskores::cont::DataSet& dataset) const
{
  const auto& fieldNm = this->GetActiveFieldName();
  if (!dataset.HasPointField(fieldNm) && !dataset.HasCellField(fieldNm))
    throw viskores::cont::ErrorFilterExecution("Unsupported field assocation");
  auto assoc = dataset.GetField(fieldNm).GetAssociation();
  ArrayType arr;
  viskores::cont::ArrayCopyShallowIfPossible(dataset.GetField(fieldNm).GetData(), arr);
  return viskores::worklet::flow::VelocityField<ArrayType>(arr, assoc);
}

VISKORES_CONT Pathline::TerminationType Pathline::GetTermination(
  const viskores::cont::DataSet& dataset) const
{
  // dataset not used
  (void)dataset;
  return Pathline::TerminationType(this->NumberOfSteps);
}

VISKORES_CONT Pathline::AnalysisType Pathline::GetAnalysis(
  const viskores::cont::DataSet& dataset) const
{
  // dataset not used
  (void)dataset;
  return Pathline::AnalysisType(this->NumberOfSteps);
}
//VISKORES_CONT viskores::filter::flow::FlowResultType Pathline::GetResultType() const
//{
//  return viskores::filter::flow::FlowResultType::STREAMLINE_TYPE;
//}

}
}
} // namespace viskores::filter::flow
