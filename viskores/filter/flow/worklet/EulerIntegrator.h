//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#ifndef viskores_filter_flow_worklet_EulerIntegrator_h
#define viskores_filter_flow_worklet_EulerIntegrator_h

#include <viskores/filter/flow/worklet/GridEvaluatorStatus.h>
#include <viskores/filter/flow/worklet/IntegratorStatus.h>

namespace viskores
{
namespace worklet
{
namespace flow
{

template <typename EvaluatorType>
class ExecEulerIntegrator
{
public:
  VISKORES_EXEC_CONT
  ExecEulerIntegrator(const EvaluatorType& evaluator)
    : Evaluator(evaluator)
  {
  }

  template <typename Particle>
  VISKORES_EXEC IntegratorStatus CheckStep(const Particle& particle,
                                           viskores::FloatDefault stepLength,
                                           viskores::Vec3f& velocity) const
  {
    auto time = particle.GetTime();
    auto inpos = particle.GetEvaluationPosition(stepLength);
    viskores::VecVariable<viskores::Vec3f, 2> vectors;
    GridEvaluatorStatus evalStatus = this->Evaluator.Evaluate(inpos, time, vectors);
    if (evalStatus.CheckFail())
      return IntegratorStatus(evalStatus, false);

    velocity = particle.Velocity(vectors, stepLength);

    return IntegratorStatus(evalStatus,
                            viskores::MagnitudeSquared(velocity) <=
                              viskores::Epsilon<viskores::FloatDefault>());
  }

private:
  EvaluatorType Evaluator;
};

template <typename EvaluatorType>
class EulerIntegrator
{
private:
  EvaluatorType Evaluator;

public:
  EulerIntegrator() = default;

  VISKORES_CONT
  EulerIntegrator(const EvaluatorType& evaluator)
    : Evaluator(evaluator)
  {
  }

  VISKORES_CONT auto PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                         viskores::cont::Token& token) const
    -> ExecEulerIntegrator<decltype(this->Evaluator.PrepareForExecution(device, token))>
  {
    auto evaluator = this->Evaluator.PrepareForExecution(device, token);
    using ExecEvaluatorType = decltype(evaluator);
    return ExecEulerIntegrator<ExecEvaluatorType>(evaluator);
  }
}; //EulerIntegrator

}
}
} //viskores::worklet::flow

#endif // viskores_filter_flow_worklet_EulerIntegrator_h
