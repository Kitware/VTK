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

#include <viskores/filter/flow/FilterParticleAdvectionUnsteadyState.h>

#include <viskores/filter/flow/internal/BoundsMap.h>
#include <viskores/filter/flow/internal/DataSetIntegratorUnsteadyState.h>
#include <viskores/filter/flow/internal/ParticleAdvector.h>

namespace viskores
{
namespace filter
{
namespace flow
{

template <typename Derived>
VISKORES_CONT typename FilterParticleAdvectionUnsteadyState<Derived>::FieldType
FilterParticleAdvectionUnsteadyState<Derived>::GetField(const viskores::cont::DataSet& data) const
{
  const Derived* inst = static_cast<const Derived*>(this);
  return inst->GetField(data);
}

template <typename Derived>
VISKORES_CONT typename FilterParticleAdvectionUnsteadyState<Derived>::TerminationType
FilterParticleAdvectionUnsteadyState<Derived>::GetTermination(
  const viskores::cont::DataSet& data) const
{
  const Derived* inst = static_cast<const Derived*>(this);
  return inst->GetTermination(data);
}

template <typename Derived>
VISKORES_CONT typename FilterParticleAdvectionUnsteadyState<Derived>::AnalysisType
FilterParticleAdvectionUnsteadyState<Derived>::GetAnalysis(
  const viskores::cont::DataSet& data) const
{
  const Derived* inst = static_cast<const Derived*>(this);
  return inst->GetAnalysis(data);
}

template <typename Derived>
VISKORES_CONT viskores::cont::PartitionedDataSet
FilterParticleAdvectionUnsteadyState<Derived>::DoExecutePartitions(
  const viskores::cont::PartitionedDataSet& input)
{
  this->ValidateOptions();

  using DSIType = viskores::filter::flow::internal::
    DataSetIntegratorUnsteadyState<ParticleType, FieldType, TerminationType, AnalysisType>;

  if (this->BlockIdsSet)
    this->BoundsMap = viskores::filter::flow::internal::BoundsMap(input, this->BlockIds);
  else
    this->BoundsMap = viskores::filter::flow::internal::BoundsMap(input);

  std::vector<DSIType> dsi;
  for (viskores::Id i = 0; i < input.GetNumberOfPartitions(); i++)
  {
    viskores::Id blockId = this->BoundsMap.GetLocalBlockId(i);
    auto ds1 = input.GetPartition(i);
    auto ds2 = this->Input2.GetPartition(i);

    // Build the field for the current dataset
    FieldType field1 = this->GetField(ds1);
    FieldType field2 = this->GetField(ds2);

    // Build the termination for the current dataset
    TerminationType termination = this->GetTermination(ds1);

    AnalysisType analysis = this->GetAnalysis(ds1);

    dsi.emplace_back(blockId,
                     field1,
                     field2,
                     ds1,
                     ds2,
                     this->Time1,
                     this->Time2,
                     this->SolverType,
                     termination,
                     analysis);
  }
  viskores::filter::flow::internal::ParticleAdvector<DSIType> pav(
    this->BoundsMap, dsi, this->UseThreadedAlgorithm);

  viskores::cont::ArrayHandle<ParticleType> particles;
  this->Seeds.AsArrayHandle(particles);
  return pav.Execute(particles, this->StepSize);
}

}
}
} // namespace viskores::filter::flow

#include <viskores/filter/flow/PathParticle.h>
#include <viskores/filter/flow/Pathline.h>

namespace viskores
{
namespace filter
{
namespace flow
{

template class FilterParticleAdvectionUnsteadyState<viskores::filter::flow::PathParticle>;
template class FilterParticleAdvectionUnsteadyState<viskores::filter::flow::Pathline>;

} // namespace flow
} // namespace filter
} // namespace viskores
