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

#ifndef viskores_filter_flow_WarpXStreamline_h
#define viskores_filter_flow_WarpXStreamline_h

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

class WarpXStreamline;

template <>
struct FlowTraits<WarpXStreamline>
{
  using ParticleType = viskores::ChargedParticle;
  using TerminationType = viskores::worklet::flow::NormalTermination;
  using AnalysisType = viskores::worklet::flow::StreamlineAnalysis<ParticleType>;
  using ArrayType = viskores::cont::ArrayHandle<viskores::Vec3f>;
  using FieldType = viskores::worklet::flow::ElectroMagneticField<ArrayType>;
};

/// \brief Advect particles in a vector field.

/// Takes as input a vector field and seed locations and generates the
/// end points for each seed through the vector field.
class VISKORES_FILTER_FLOW_EXPORT WarpXStreamline
  : public viskores::filter::flow::FilterParticleAdvectionSteadyState<WarpXStreamline>
{
public:
  using ParticleType = typename FlowTraits<WarpXStreamline>::ParticleType;
  using TerminationType = typename FlowTraits<WarpXStreamline>::TerminationType;
  using AnalysisType = typename FlowTraits<WarpXStreamline>::AnalysisType;
  using ArrayType = typename FlowTraits<WarpXStreamline>::ArrayType;
  using FieldType = typename FlowTraits<WarpXStreamline>::FieldType;

  VISKORES_CONT WarpXStreamline() { this->SetSolverEuler(); }

  VISKORES_CONT FieldType GetField(const viskores::cont::DataSet& data) const;

  VISKORES_CONT TerminationType GetTermination(const viskores::cont::DataSet& data) const;

  VISKORES_CONT AnalysisType GetAnalysis(const viskores::cont::DataSet& data) const;

  VISKORES_CONT void SetEField(const std::string& name) { this->SetActiveField(0, name); }

  VISKORES_CONT void SetBField(const std::string& name) { this->SetActiveField(1, name); }

  VISKORES_CONT std::string GetEField() const { return this->GetActiveFieldName(0); }

  VISKORES_CONT std::string GetBField() const { return this->GetActiveFieldName(1); }
};

}
}
} // namespace viskores::filter::flow

#endif // viskores_filter_flow_WarpXStreamline_h
