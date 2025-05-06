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

#include <viskores/filter/flow/PathParticle.h>

namespace viskores
{
namespace filter
{
namespace flow
{

VISKORES_CONT PathParticle::FieldType PathParticle::GetField(
  const viskores::cont::DataSet& dataset) const
{
  const auto& fieldNm = this->GetActiveFieldName();
  if (!dataset.HasPointField(fieldNm) && !dataset.HasCellField(fieldNm))
    throw viskores::cont::ErrorFilterExecution("Unsupported field assocation");
  auto assoc = dataset.GetField(fieldNm).GetAssociation();
  ArrayType arr;
  viskores::cont::ArrayCopyShallowIfPossible(dataset.GetField(fieldNm).GetData(), arr);
  return PathParticle::FieldType(arr, assoc);
}

VISKORES_CONT PathParticle::TerminationType PathParticle::GetTermination(
  const viskores::cont::DataSet& dataset) const
{
  // dataset not used
  (void)dataset;
  return PathParticle::TerminationType(this->NumberOfSteps);
}

VISKORES_CONT PathParticle::AnalysisType PathParticle::GetAnalysis(
  const viskores::cont::DataSet& dataset) const
{
  // dataset not used
  (void)dataset;
  return PathParticle::AnalysisType();
}

//VISKORES_CONT viskores::filter::flow::FlowResultType PathParticle::GetResultType() const
//{
//  return viskores::filter::flow::FlowResultType::PARTICLE_ADVECT_TYPE;
//}

}
}
} // namespace viskores::filter::flow
