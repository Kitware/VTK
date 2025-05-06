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

#include <viskores/filter/flow/WarpXStreamline.h>

namespace viskores
{
namespace filter
{
namespace flow
{

VISKORES_CONT WarpXStreamline::FieldType WarpXStreamline::GetField(
  const viskores::cont::DataSet& dataset) const
{
  const auto& electric = this->GetEField();
  const auto& magnetic = this->GetBField();
  if (!dataset.HasPointField(electric) && !dataset.HasCellField(electric))
    throw viskores::cont::ErrorFilterExecution("Unsupported field assocation");
  if (!dataset.HasPointField(magnetic) && !dataset.HasCellField(magnetic))
    throw viskores::cont::ErrorFilterExecution("Unsupported field assocation");
  auto eAssoc = dataset.GetField(electric).GetAssociation();
  auto bAssoc = dataset.GetField(magnetic).GetAssociation();
  if (eAssoc != bAssoc)
  {
    throw viskores::cont::ErrorFilterExecution("E and B field need to have same association");
  }
  ArrayType eField, bField;
  viskores::cont::ArrayCopyShallowIfPossible(dataset.GetField(electric).GetData(), eField);
  viskores::cont::ArrayCopyShallowIfPossible(dataset.GetField(magnetic).GetData(), bField);
  return viskores::worklet::flow::ElectroMagneticField<ArrayType>(eField, bField, eAssoc);
}

VISKORES_CONT WarpXStreamline::TerminationType WarpXStreamline::GetTermination(
  const viskores::cont::DataSet& dataset) const
{
  // dataset not used
  (void)dataset;
  return WarpXStreamline::TerminationType(this->NumberOfSteps);
}

VISKORES_CONT WarpXStreamline::AnalysisType WarpXStreamline::GetAnalysis(
  const viskores::cont::DataSet& dataset) const
{
  // dataset not used
  (void)dataset;
  return WarpXStreamline::AnalysisType(this->NumberOfSteps);
}

}
}
} // namespace viskores::filter::flow
