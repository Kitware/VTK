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

#ifndef viskores_filter_flow_worklet_Particles_h
#define viskores_filter_flow_worklet_Particles_h

#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/filter/flow/worklet/IntegratorStatus.h>

#include <viskores/filter/flow/worklet/Analysis.h>
#include <viskores/filter/flow/worklet/Termination.h>

namespace viskores
{
namespace worklet
{
namespace flow
{

template <typename ParticleType, typename TerminationType, typename AnalysisType>
class ParticleExecutionObject
{
public:
  VISKORES_EXEC_CONT
  ParticleExecutionObject()
    : Particles()
    , Termination()
    , Analysis()
  {
  }

  ParticleExecutionObject(viskores::cont::ArrayHandle<ParticleType> particleArray,
                          const TerminationType& termination,
                          const AnalysisType& analysis,
                          viskores::cont::DeviceAdapterId device,
                          viskores::cont::Token& token)
    : Termination(termination)
    , Analysis(analysis)
  {
    Particles = particleArray.PrepareForInPlace(device, token);
  }

  VISKORES_EXEC
  ParticleType GetParticle(const viskores::Id& idx) { return this->Particles.Get(idx); }

  VISKORES_EXEC
  void PreStepUpdate(const viskores::Id& idx, const ParticleType& particle)
  {
    this->Analysis.PreStepAnalyze(idx, particle);
  }

  VISKORES_EXEC
  void StepUpdate(const viskores::Id& idx,
                  const ParticleType& particle,
                  viskores::FloatDefault time,
                  const viskores::Vec3f& pt)
  {
    ParticleType newParticle(particle);
    newParticle.SetPosition(pt);
    newParticle.SetTime(time);
    newParticle.SetNumberOfSteps(particle.GetNumberOfSteps() + 1);
    this->Analysis.Analyze(idx, particle, newParticle);
    this->Particles.Set(idx, newParticle);
  }

  VISKORES_EXEC
  void StatusUpdate(const viskores::Id& idx,
                    const viskores::worklet::flow::IntegratorStatus& status)
  {
    ParticleType p(this->GetParticle(idx));

    if (status.CheckFail())
      p.GetStatus().SetFail();
    if (status.CheckSpatialBounds())
      p.GetStatus().SetSpatialBounds();
    if (status.CheckTemporalBounds())
      p.GetStatus().SetTemporalBounds();
    if (status.CheckInGhostCell())
      p.GetStatus().SetInGhostCell();
    if (status.CheckZeroVelocity())
    {
      p.GetStatus().SetZeroVelocity();
      p.GetStatus().SetTerminate();
    }

    this->Particles.Set(idx, p);
  }

  VISKORES_EXEC
  bool CanContinue(const viskores::Id& idx)
  {
    ParticleType particle(this->GetParticle(idx));
    auto terminate = this->Termination.CheckTermination(particle);
    this->Particles.Set(idx, particle);
    return terminate;
  }

  VISKORES_EXEC
  void UpdateTookSteps(const viskores::Id& idx, bool val)
  {
    ParticleType p(this->GetParticle(idx));
    if (val)
      p.GetStatus().SetTookAnySteps();
    else
      p.GetStatus().ClearTookAnySteps();
    this->Particles.Set(idx, p);
  }

protected:
  using ParticlePortal = typename viskores::cont::ArrayHandle<ParticleType>::WritePortalType;
  ParticlePortal Particles;
  TerminationType Termination;
  AnalysisType Analysis;
};

template <typename ParticleType, typename TerminationType, typename AnalysisType>
class Particles : public viskores::cont::ExecutionObjectBase
{
protected:
  viskores::cont::ArrayHandle<ParticleType> ParticleArray;
  TerminationType Termination;
  AnalysisType Analysis;

public:
  VISKORES_CONT
  Particles() = default;

  VISKORES_CONT
  Particles(viskores::cont::ArrayHandle<ParticleType>& pArray,
            TerminationType termination,
            AnalysisType analysis)
    : ParticleArray(pArray)
    , Termination(termination)
    , Analysis(analysis)
  {
  }

  VISKORES_CONT auto PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                         viskores::cont::Token& token) const
    -> viskores::worklet::flow::ParticleExecutionObject<
      ParticleType,
      decltype(this->Termination.PrepareForExecution(device, token)),
      decltype(this->Analysis.PrepareForExecution(device, token))>
  {
    auto termination = this->Termination.PrepareForExecution(device, token);
    auto analysis = this->Analysis.PrepareForExecution(device, token);
    return viskores::worklet::flow::
      ParticleExecutionObject<ParticleType, decltype(termination), decltype(analysis)>(
        this->ParticleArray, termination, analysis, device, token);
  }
};

}
}
} //viskores::worklet::flow

#endif // viskores_filter_flow_worklet_Particles_h
//============================================================================
