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

#ifndef viskores_filter_flow_worklet_RK4Integrator_h
#define viskores_filter_flow_worklet_RK4Integrator_h

#include <viskores/filter/flow/worklet/GridEvaluatorStatus.h>
#include <viskores/filter/flow/worklet/IntegratorStatus.h>

namespace viskores
{
namespace worklet
{
namespace flow
{

template <typename ExecEvaluatorType>
class ExecRK4Integrator
{
public:
  VISKORES_EXEC_CONT
  ExecRK4Integrator(const ExecEvaluatorType& evaluator)
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
    viskores::FloatDefault boundary =
      this->Evaluator.GetTemporalBoundary(static_cast<viskores::Id>(1));
    if ((time + stepLength + viskores::Epsilon<viskores::FloatDefault>() - boundary) > 0.0)
      stepLength = boundary - time;

    //k1 = F(p,t)
    //k2 = F(p+hk1/2, t+h/2
    //k3 = F(p+hk2/2, t+h/2
    //k4 = F(p+hk3, t+h)
    //Yn+1 = Yn + 1/6 h (k1+2k2+2k3+k4)

    viskores::FloatDefault var1 = (stepLength / static_cast<viskores::FloatDefault>(2));
    viskores::FloatDefault var2 = time + var1;
    viskores::FloatDefault var3 = time + stepLength;

    viskores::Vec3f v1 = viskores::TypeTraits<viskores::Vec3f>::ZeroInitialization();
    viskores::Vec3f v2 = v1, v3 = v1, v4 = v1;
    viskores::VecVariable<viskores::Vec3f, 2> k1, k2, k3, k4;

    GridEvaluatorStatus evalStatus;

    evalStatus = this->Evaluator.Evaluate(inpos, time, k1);
    if (evalStatus.CheckFail())
      return IntegratorStatus(evalStatus, false);
    v1 = particle.Velocity(k1, stepLength);

    evalStatus = this->Evaluator.Evaluate(inpos + var1 * v1, var2, k2);
    if (evalStatus.CheckFail())
      return IntegratorStatus(evalStatus, false);
    v2 = particle.Velocity(k2, stepLength);

    evalStatus = this->Evaluator.Evaluate(inpos + var1 * v2, var2, k3);
    if (evalStatus.CheckFail())
      return IntegratorStatus(evalStatus, false);
    v3 = particle.Velocity(k3, stepLength);

    evalStatus = this->Evaluator.Evaluate(inpos + stepLength * v3, var3, k4);
    if (evalStatus.CheckFail())
      return IntegratorStatus(evalStatus, false);
    v4 = particle.Velocity(k4, stepLength);

    velocity = (v1 + 2 * v2 + 2 * v3 + v4) / static_cast<viskores::FloatDefault>(6);

    return IntegratorStatus(evalStatus,
                            viskores::MagnitudeSquared(velocity) <=
                              viskores::Epsilon<viskores::FloatDefault>());
  }

private:
  ExecEvaluatorType Evaluator;
};

template <typename EvaluatorType>
class RK4Integrator
{
private:
  EvaluatorType Evaluator;

public:
  VISKORES_CONT
  RK4Integrator() = default;

  VISKORES_CONT
  RK4Integrator(const EvaluatorType& evaluator)
    : Evaluator(evaluator)
  {
  }

  VISKORES_CONT auto PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                         viskores::cont::Token& token) const
    -> ExecRK4Integrator<decltype(this->Evaluator.PrepareForExecution(device, token))>
  {
    auto evaluator = this->Evaluator.PrepareForExecution(device, token);
    using ExecEvaluatorType = decltype(evaluator);
    return ExecRK4Integrator<ExecEvaluatorType>(evaluator);
  }
};

}
}
} //viskores::worklet::flow

#endif // viskores_filter_flow_worklet_RK4Integrator_h
