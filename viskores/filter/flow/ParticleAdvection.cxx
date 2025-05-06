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

#include <viskores/filter/flow/ParticleAdvection.h>

namespace viskores
{
namespace filter
{
namespace flow
{

// using ParticleType    = viskores::Particle;
// using TerminationType = viskores::worklet::flow::NormalTermination;
// using AnalysisType    = viskores::worklet::flow::Streamline;
// using ArrayType       = viskores::cont::ArrayHandle<viskores::Vec3f>;
// using FieldType       = viskores::worklet::flow::VelocityField<ArrayType>;

VISKORES_CONT ParticleAdvection::FieldType ParticleAdvection::GetField(
  const viskores::cont::DataSet& dataset) const
{
  const auto& fieldNm = this->GetActiveFieldName();
  if (!dataset.HasPointField(fieldNm) && !dataset.HasCellField(fieldNm))
    throw viskores::cont::ErrorFilterExecution("Unsupported field assocation");
  auto assoc = dataset.GetField(fieldNm).GetAssociation();
  ArrayType arr;
  viskores::cont::ArrayCopyShallowIfPossible(dataset.GetField(fieldNm).GetData(), arr);
  return ParticleAdvection::FieldType(arr, assoc);
}

VISKORES_CONT ParticleAdvection::TerminationType ParticleAdvection::GetTermination(
  const viskores::cont::DataSet& dataset) const
{
  // dataset not used
  (void)dataset;
  return ParticleAdvection::TerminationType(this->NumberOfSteps);
}

VISKORES_CONT ParticleAdvection::AnalysisType ParticleAdvection::GetAnalysis(
  const viskores::cont::DataSet& dataset) const
{
  // dataset not used
  (void)dataset;
  return ParticleAdvection::AnalysisType{};
}

}
}
} // namespace viskores::filter::flow
