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

#ifndef viskores_filter_flow_worklet_Stepper_h
#define viskores_filter_flow_worklet_Stepper_h

#include <viskores/filter/flow/worklet/GridEvaluators.h>
#include <viskores/filter/flow/worklet/IntegratorStatus.h>
#include <viskores/filter/flow/worklet/Particles.h>

#include <limits>

namespace viskores
{
namespace worklet
{
namespace flow
{

template <typename ExecIntegratorType, typename ExecEvaluatorType>
class StepperImpl
{
private:
  ExecIntegratorType Integrator;
  ExecEvaluatorType Evaluator;
  viskores::FloatDefault DeltaT;
  viskores::FloatDefault Tolerance;

public:
  VISKORES_EXEC_CONT
  StepperImpl(const ExecIntegratorType& integrator,
              const ExecEvaluatorType& evaluator,
              const viskores::FloatDefault deltaT,
              const viskores::FloatDefault tolerance)
    : Integrator(integrator)
    , Evaluator(evaluator)
    , DeltaT(deltaT)
    , Tolerance(tolerance)
  {
  }

  template <typename Particle>
  VISKORES_EXEC IntegratorStatus Step(Particle& particle,
                                      viskores::FloatDefault& time,
                                      viskores::Vec3f& outpos) const
  {
    viskores::Vec3f velocity(0, 0, 0);
    auto status = this->Integrator.CheckStep(particle, this->DeltaT, velocity);
    if (status.CheckOk())
    {
      outpos = particle.GetPosition() + this->DeltaT * velocity;
      time += this->DeltaT;
    }
    else
      outpos = particle.GetPosition();

    return status;
  }

  template <typename Particle>
  VISKORES_EXEC IntegratorStatus SmallStep(Particle& particle,
                                           viskores::FloatDefault& time,
                                           viskores::Vec3f& outpos) const
  {
    //Stepping by this->DeltaT goes beyond the bounds of the dataset.
    //We need to take an Euler step that goes outside of the dataset.
    //Use a binary search to find the largest step INSIDE the dataset.
    //Binary search uses a shrinking bracket of inside / outside, so when
    //we terminate, the outside value is the stepsize that will nudge
    //the particle outside the dataset.

    //The binary search will be between {0, this->DeltaT}
    viskores::FloatDefault stepRange[2] = { 0, this->DeltaT };

    viskores::Vec3f currPos(particle.GetEvaluationPosition(this->DeltaT));
    viskores::Vec3f currVelocity(0, 0, 0);
    viskores::VecVariable<viskores::Vec3f, 2> currValue, tmp;
    auto evalStatus = this->Evaluator.Evaluate(currPos, particle.GetTime(), currValue);
    if (evalStatus.CheckFail())
      return IntegratorStatus(evalStatus, false);

    const viskores::FloatDefault eps = viskores::Epsilon<viskores::FloatDefault>() * 10;
    viskores::FloatDefault div = 1;
    while ((stepRange[1] - stepRange[0]) > eps)
    {
      //Try a step midway between stepRange[0] and stepRange[1]
      div *= 2;
      viskores::FloatDefault currStep = stepRange[0] + (this->DeltaT / div);

      //See if we can step by currStep
      IntegratorStatus status = this->Integrator.CheckStep(particle, currStep, currVelocity);

      if (status.CheckOk()) //Integration step succedded.
      {
        //See if this point is in/out.
        auto newPos = particle.GetPosition() + currStep * currVelocity;
        evalStatus = this->Evaluator.Evaluate(newPos, particle.GetTime() + currStep, tmp);
        if (evalStatus.CheckOk())
        {
          //Point still in. Update currPos and set range to {currStep, stepRange[1]}
          currPos = newPos;
          stepRange[0] = currStep;
        }
        else
        {
          //The step succedded, but the next point is outside.
          //Step too long. Set range to: {stepRange[0], currStep} and continue.
          stepRange[1] = currStep;
        }
      }
      else
      {
        //Step too long. Set range to: {stepRange[0], stepCurr} and continue.
        stepRange[1] = currStep;
      }
    }

    evalStatus = this->Evaluator.Evaluate(currPos, particle.GetTime() + stepRange[0], currValue);
    // The eval at Time + stepRange[0] better be *inside*
    VISKORES_ASSERT(evalStatus.CheckOk() && !evalStatus.CheckSpatialBounds());
    if (evalStatus.CheckFail() || evalStatus.CheckSpatialBounds())
      return IntegratorStatus(evalStatus, false);

    // Update the position and time.
    auto velocity = particle.Velocity(currValue, stepRange[1]);
    outpos = currPos + stepRange[1] * velocity;
    time += stepRange[1];

    // Get the evaluation status for the point that is moved by the euler step.
    evalStatus = this->Evaluator.Evaluate(outpos, time, currValue);

    IntegratorStatus status(evalStatus,
                            viskores::MagnitudeSquared(velocity) <=
                              viskores::Epsilon<viskores::FloatDefault>());
    status.SetOk(); //status is ok.

    return status;
  }
};


template <typename IntegratorType, typename EvaluatorType>
class Stepper : public viskores::cont::ExecutionObjectBase
{
private:
  IntegratorType Integrator;
  EvaluatorType Evaluator;
  viskores::FloatDefault DeltaT;
  viskores::FloatDefault Tolerance = std::numeric_limits<viskores::FloatDefault>::epsilon() *
    static_cast<viskores::FloatDefault>(100.0f);

public:
  VISKORES_CONT
  Stepper() = default;

  VISKORES_CONT
  Stepper(const EvaluatorType& evaluator, const viskores::FloatDefault deltaT)
    : Integrator(IntegratorType(evaluator))
    , Evaluator(evaluator)
    , DeltaT(deltaT)
  {
  }

  VISKORES_CONT
  void SetTolerance(viskores::FloatDefault tolerance) { this->Tolerance = tolerance; }

public:
  /// Return the StepperImpl object
  /// Prepares the execution object of Stepper
  VISKORES_CONT auto PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                         viskores::cont::Token& token) const
    -> StepperImpl<decltype(this->Integrator.PrepareForExecution(device, token)),
                   decltype(this->Evaluator.PrepareForExecution(device, token))>
  {
    auto integrator = this->Integrator.PrepareForExecution(device, token);
    auto evaluator = this->Evaluator.PrepareForExecution(device, token);
    using ExecIntegratorType = decltype(integrator);
    using ExecEvaluatorType = decltype(evaluator);
    return StepperImpl<ExecIntegratorType, ExecEvaluatorType>(
      integrator, evaluator, this->DeltaT, this->Tolerance);
  }
};

}
}
} //viskores::worklet::flow

#endif // viskores_filter_flow_worklet_Stepper_h
