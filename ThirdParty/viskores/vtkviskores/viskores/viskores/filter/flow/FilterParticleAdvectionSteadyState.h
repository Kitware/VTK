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

#ifndef viskores_filter_flow_FilterParticleAdvectionSteadyState_h
#define viskores_filter_flow_FilterParticleAdvectionSteadyState_h

#include <viskores/filter/flow/FilterParticleAdvection.h>
#include <viskores/filter/flow/FlowTypes.h>
#include <viskores/filter/flow/viskores_filter_flow_export.h>

namespace viskores
{
namespace filter
{
namespace flow
{

template <typename Derived>
struct FlowTraits;

template <typename Derived>
class VISKORES_FILTER_FLOW_EXPORT FilterParticleAdvectionSteadyState
  : public FilterParticleAdvection
{
public:
  using ParticleType = typename FlowTraits<Derived>::ParticleType;
  using FieldType = typename FlowTraits<Derived>::FieldType;
  using TerminationType = typename FlowTraits<Derived>::TerminationType;
  using AnalysisType = typename FlowTraits<Derived>::AnalysisType;

private:
  VISKORES_CONT FieldType GetField(const viskores::cont::DataSet& data) const;

  VISKORES_CONT TerminationType GetTermination(const viskores::cont::DataSet& data) const;

  VISKORES_CONT AnalysisType GetAnalysis(const viskores::cont::DataSet& data) const;

  VISKORES_CONT viskores::cont::PartitionedDataSet DoExecutePartitions(
    const viskores::cont::PartitionedDataSet& input) override;
};
}
}
} // namespace viskores::filter::flow

#endif // viskores_filter_flow_FilterParticleAdvectionSteadyState_h
