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

#ifndef viskores_filter_flow_Streamline_h
#define viskores_filter_flow_Streamline_h

#include <viskores/filter/flow/FilterParticleAdvectionSteadyState.h>
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

class Streamline;

template <>
struct FlowTraits<Streamline>
{
  using ParticleType = viskores::Particle;
  using TerminationType = viskores::worklet::flow::NormalTermination;
  using AnalysisType = viskores::worklet::flow::StreamlineAnalysis<ParticleType>;
  using ArrayType = viskores::cont::ArrayHandle<viskores::Vec3f>;
  using FieldType = viskores::worklet::flow::VelocityField<ArrayType>;
};

/// \brief Advect particles in a vector field and display the path they take.
///
/// This filter takes as input a velocity vector field and seed locations. It then traces the
/// path each seed point would take if moving at the velocity specified by the field.
/// Mathematically, this is the curve that is tangent to the velocity field everywhere.
///
/// The output of this filter is a `viskores::cont::DataSet` containing a collection of poly-lines
/// representing the paths the seed particles take.
class VISKORES_FILTER_FLOW_EXPORT Streamline
  : public viskores::filter::flow::FilterParticleAdvectionSteadyState<Streamline>
{
public:
  using ParticleType = typename FlowTraits<Streamline>::ParticleType;
  using TerminationType = typename FlowTraits<Streamline>::TerminationType;
  using AnalysisType = typename FlowTraits<Streamline>::AnalysisType;
  using ArrayType = typename FlowTraits<Streamline>::ArrayType;
  using FieldType = typename FlowTraits<Streamline>::FieldType;

  VISKORES_CONT FieldType GetField(const viskores::cont::DataSet& data) const;

  VISKORES_CONT TerminationType GetTermination(const viskores::cont::DataSet& data) const;

  VISKORES_CONT AnalysisType GetAnalysis(const viskores::cont::DataSet& data) const;
};

}
}
} // namespace viskores::filter::flow

#endif // viskores_filter_flow_Streamline_h
