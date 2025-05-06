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

#ifndef viskores_filter_flow_Pathline_h
#define viskores_filter_flow_Pathline_h

#include <viskores/filter/flow/FilterParticleAdvectionUnsteadyState.h>
#include <viskores/filter/flow/FlowTypes.h>
#include <viskores/filter/flow/viskores_filter_flow_export.h>

#include <viskores/filter/flow/worklet/Analysis.h>
#include <viskores/filter/flow/worklet/Field.h>
#include <viskores/filter/flow/worklet/Termination.h>

namespace viskores
{
namespace filter
{
namespace flow
{

class Pathline;

template <>
struct FlowTraits<Pathline>
{
  using ParticleType = viskores::Particle;
  using TerminationType = viskores::worklet::flow::NormalTermination;
  using AnalysisType = viskores::worklet::flow::StreamlineAnalysis<ParticleType>;
  using ArrayType = viskores::cont::ArrayHandle<viskores::Vec3f>;
  using FieldType = viskores::worklet::flow::VelocityField<ArrayType>;
};

/// \brief Advect particles in a time-varying vector field and display the path they take.
///
/// This filter takes as input a velocity vector field, changing between two time steps,
/// and seed locations. It then traces the path each seed point would take if moving at
/// the velocity specified by the field.
///
/// The output of this filter is a `viskores::cont::DataSet` containing a collection of poly-lines
/// representing the paths the seed particles take.
class VISKORES_FILTER_FLOW_EXPORT Pathline
  : public viskores::filter::flow::FilterParticleAdvectionUnsteadyState<Pathline>
{
public:
  using ParticleType = typename FlowTraits<Pathline>::ParticleType;
  using TerminationType = typename FlowTraits<Pathline>::TerminationType;
  using AnalysisType = typename FlowTraits<Pathline>::AnalysisType;
  using ArrayType = typename FlowTraits<Pathline>::ArrayType;
  using FieldType = typename FlowTraits<Pathline>::FieldType;

  VISKORES_CONT FieldType GetField(const viskores::cont::DataSet& data) const;

  VISKORES_CONT TerminationType GetTermination(const viskores::cont::DataSet& data) const;

  VISKORES_CONT AnalysisType GetAnalysis(const viskores::cont::DataSet& data) const;
};

}
}
} // namespace viskores::filter::flow

#endif // viskores_filter_flow_Pathline_h
