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

#ifndef viskores_filter_flow_worklet_ParticleAdvectionWorklets_h
#define viskores_filter_flow_worklet_ParticleAdvectionWorklets_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/cont/Invoker.h>

#include <viskores/Particle.h>
#include <viskores/filter/flow/worklet/Particles.h>
#include <viskores/worklet/WorkletMapField.h>

#ifdef VISKORES_CUDA
#include <viskores/cont/cuda/internal/ScopedCudaStackSize.h>
#endif

namespace viskores
{
namespace worklet
{
namespace flow
{

class ParticleAdvectWorklet : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_EXEC_CONT
  ParticleAdvectWorklet()
    : PushOutOfBounds(true)
  {
  }

  VISKORES_EXEC_CONT
  ParticleAdvectWorklet(bool pushOutOfBounds)
    : PushOutOfBounds(pushOutOfBounds)
  {
  }

  using ControlSignature = void(FieldIn idx, ExecObject integrator, ExecObject integralCurve);
  using ExecutionSignature = void(_1 idx, _2 integrator, _3 integralCurve);
  using InputDomain = _1;

  template <typename IntegratorType, typename IntegralCurveType>
  VISKORES_EXEC void operator()(const viskores::Id& idx,
                                const IntegratorType& integrator,
                                IntegralCurveType& integralCurve) const
  {
    auto particle = integralCurve.GetParticle(idx);
    viskores::FloatDefault time = particle.GetTime();
    bool tookAnySteps = false;

    //the integrator status needs to be more robust:
    // 1. you could have success AND at temporal boundary.
    // 2. could you have success AND at spatial?
    // 3. all three?
    integralCurve.PreStepUpdate(idx, particle);
    do
    {
      particle = integralCurve.GetParticle(idx);
      viskores::Vec3f outpos;
      auto status = integrator.Step(particle, time, outpos);
      if (status.CheckOk())
      {
        integralCurve.StepUpdate(idx, particle, time, outpos);
        tookAnySteps = true;
      }

      //We can't take a step inside spatial boundary.
      //Try and take a step just past the boundary.
      else if (status.CheckSpatialBounds() && this->PushOutOfBounds)
      {
        status = integrator.SmallStep(particle, time, outpos);
        if (status.CheckOk())
        {
          integralCurve.StepUpdate(idx, particle, time, outpos);
          tookAnySteps = true;
        }
      }
      integralCurve.StatusUpdate(idx, status);
    } while (integralCurve.CanContinue(idx));

    //Mark if any steps taken
    integralCurve.UpdateTookSteps(idx, tookAnySteps);
  }

private:
  bool PushOutOfBounds;
};


template <typename IntegratorType,
          typename ParticleType,
          typename TerminationType,
          typename AnalysisType>
class ParticleAdvectionWorklet
{
public:
  VISKORES_EXEC_CONT ParticleAdvectionWorklet() {}

  ~ParticleAdvectionWorklet() {}

  void Run(const IntegratorType& integrator,
           viskores::cont::ArrayHandle<ParticleType>& particles,
           const TerminationType& termination,
           AnalysisType& analysis)
  {

    using ParticleArrayType =
      viskores::worklet::flow::Particles<ParticleType, TerminationType, AnalysisType>;

    viskores::Id numSeeds = static_cast<viskores::Id>(particles.GetNumberOfValues());
    //Create and invoke the particle advection.
    //viskores::cont::ArrayHandleConstant<viskores::Id> maxSteps(MaxSteps, numSeeds);
    viskores::cont::ArrayHandleIndex idxArray(numSeeds);
    // TODO: The particle advection sometimes behaves incorrectly on CUDA if the stack size
    // is not changed thusly. This is concerning as the compiler should be able to determine
    // staticly the required stack depth. What is even more concerning is that the runtime
    // does not report a stack overflow. Rather, the worklet just silently reports the wrong
    // value. Until we determine the root cause, other problems may pop up.
#ifdef VISKORES_CUDA
    // This worklet needs some extra space on CUDA.
    viskores::cont::cuda::internal::ScopedCudaStackSize stack(16 * 1024);
    (void)stack;
#endif // VISKORES_CUDA

    // Initialize all the pre-requisites needed to start analysis
    // It's based on the existing properties of the particles,
    // for e.g. the number of steps they've already taken
    analysis.InitializeAnalysis(particles);

    ParticleArrayType particlesObj(particles, termination, analysis);

    viskores::worklet::flow::ParticleAdvectWorklet worklet(analysis.SupportPushOutOfBounds());

    viskores::cont::Invoker invoker;
    invoker(worklet, idxArray, integrator, particlesObj);

    // Finalize the analysis and clear intermittant arrays.
    analysis.FinalizeAnalysis(particles);
  }
};

}
}
} // namespace viskores::worklet::flow

#endif // viskores_filter_flow_worklet_ParticleAdvectionWorklets_h
