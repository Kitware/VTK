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

#ifndef viskores_filter_flow_FilterParticleAdvectionUnsteadyState_h
#define viskores_filter_flow_FilterParticleAdvectionUnsteadyState_h

#include <viskores/filter/flow/FilterParticleAdvection.h>
#include <viskores/filter/flow/viskores_filter_flow_export.h>

namespace viskores
{
namespace filter
{
namespace flow
{

template <typename Derived>
struct FlowTraits;

/// @brief Superclass for filters that operate on flow that changes over time.
///
template <typename Derived>
class VISKORES_FILTER_FLOW_EXPORT FilterParticleAdvectionUnsteadyState
  : public FilterParticleAdvection
{
public:
  using ParticleType = typename FlowTraits<Derived>::ParticleType;
  using FieldType = typename FlowTraits<Derived>::FieldType;
  using TerminationType = typename FlowTraits<Derived>::TerminationType;
  using AnalysisType = typename FlowTraits<Derived>::AnalysisType;

  /// @brief Specifies time value for the input data set.
  ///
  /// This is the data set that passed into the `Execute()` method.
  VISKORES_CONT void SetPreviousTime(viskores::FloatDefault t1) { this->Time1 = t1; }

  /// @brief Specifies time value for the next data set.
  ///
  /// This is the data set passed into `SetNextDataSet()` @e before `Execute()` is called.
  VISKORES_CONT void SetNextTime(viskores::FloatDefault t2) { this->Time2 = t2; }

  /// @brief Specifies the data for the later time step.
  VISKORES_CONT void SetNextDataSet(const viskores::cont::DataSet& ds) { this->Input2 = { ds }; }

  /// @brief Specifies the data for the later time step.
  VISKORES_CONT void SetNextDataSet(const viskores::cont::PartitionedDataSet& pds)
  {
    this->Input2 = pds;
  }

private:
  VISKORES_CONT FieldType GetField(const viskores::cont::DataSet& data) const;

  VISKORES_CONT TerminationType GetTermination(const viskores::cont::DataSet& data) const;

  VISKORES_CONT AnalysisType GetAnalysis(const viskores::cont::DataSet& data) const;

  VISKORES_CONT viskores::cont::PartitionedDataSet DoExecutePartitions(
    const viskores::cont::PartitionedDataSet& input);

  viskores::cont::PartitionedDataSet Input2;
  viskores::FloatDefault Time1 = -1;
  viskores::FloatDefault Time2 = -1;
};

}
}
} // namespace viskores::filter::flow

#endif // viskores_filter_flow_FilterParticleAdvectionUnsteadyState_h
