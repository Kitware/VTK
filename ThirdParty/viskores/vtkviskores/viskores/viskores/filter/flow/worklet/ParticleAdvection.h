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

#ifndef viskores_filter_flow_worklet_ParticleAdvection_h
#define viskores_filter_flow_worklet_ParticleAdvection_h

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/Invoker.h>
#include <viskores/filter/flow/viskores_filter_flow_export.h>
#include <viskores/internal/Instantiations.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace flow
{

namespace detail
{
class CopyToParticle : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature =
    void(FieldIn pt, FieldIn id, FieldIn time, FieldIn step, FieldOut particle);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename ParticleType>
  VISKORES_EXEC void operator()(const viskores::Vec3f& pt,
                                const viskores::Id& id,
                                const viskores::FloatDefault& time,
                                const viskores::Id& step,
                                ParticleType& particle) const
  {
    particle.SetPosition(pt);
    particle.SetID(id);
    particle.SetTime(time);
    particle.SetNumberOfSteps(step);
    particle.GetStatus().SetOk();
  }
};

} //detail

class ParticleAdvection
{
public:
  template <typename IntegratorType,
            typename ParticleType,
            typename ParticleStorage,
            typename TerminationType,
            typename AnalysisType>
  static void Run(const IntegratorType& it,
                  viskores::cont::ArrayHandle<ParticleType, ParticleStorage>& particles,
                  const TerminationType& termination,
                  AnalysisType& analysis);

  template <typename IntegratorType,
            typename ParticleType,
            typename PointStorage,
            typename TerminationType,
            typename AnalysisType>
  static void Run(const IntegratorType& it,
                  const viskores::cont::ArrayHandle<viskores::Vec3f, PointStorage>& points,
                  const TerminationType& termination,
                  AnalysisType& analysis)
  {
    viskores::cont::ArrayHandle<ParticleType> particles;
    viskores::cont::ArrayHandle<viskores::Id> step, ids;
    viskores::cont::ArrayHandle<viskores::FloatDefault> time;
    viskores::cont::Invoker invoke;

    viskores::Id numPts = points.GetNumberOfValues();
    viskores::cont::ArrayHandleConstant<viskores::Id> s(0, numPts);
    viskores::cont::ArrayHandleConstant<viskores::FloatDefault> t(0, numPts);
    viskores::cont::ArrayHandleCounting<viskores::Id> id(0, 1, numPts);

    //Copy input to viskores::Particle
    viskores::cont::ArrayCopy(s, step);
    viskores::cont::ArrayCopy(t, time);
    viskores::cont::ArrayCopy(id, ids);
    invoke(detail::CopyToParticle{}, points, ids, time, step, particles);

    Run(it, particles, termination, analysis);
  }
};

}
}
} // viskores::worklet::flow

// Includes for instantiations
#include <viskores/Particle.h>
#include <viskores/filter/flow/worklet/Analysis.h>
#include <viskores/filter/flow/worklet/EulerIntegrator.h>
#include <viskores/filter/flow/worklet/GridEvaluators.h>
#include <viskores/filter/flow/worklet/RK4Integrator.h>
#include <viskores/filter/flow/worklet/Stepper.h>
#include <viskores/filter/flow/worklet/TemporalGridEvaluators.h>
#include <viskores/filter/flow/worklet/Termination.h>

VISKORES_INSTANTIATION_BEGIN
extern template VISKORES_FILTER_FLOW_EXPORT void viskores::worklet::flow::ParticleAdvection::Run<
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::RK4Integrator<
      viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::VelocityField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::VelocityField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
  viskores::Particle,
  viskores::cont::StorageTagBasic,
  viskores::worklet::flow::NormalTermination,
  viskores::worklet::flow::NoAnalysis<viskores::Particle>>(
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::RK4Integrator<
      viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::VelocityField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::VelocityField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>> const&,
  viskores::cont::ArrayHandle<viskores::Particle, viskores::cont::StorageTagBasic>&,
  viskores::worklet::flow::NormalTermination const&,
  viskores::worklet::flow::NoAnalysis<viskores::Particle>&);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template VISKORES_FILTER_FLOW_EXPORT void viskores::worklet::flow::ParticleAdvection::Run<
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::RK4Integrator<
      viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::VelocityField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::VelocityField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
  viskores::Particle,
  viskores::cont::StorageTagBasic,
  viskores::worklet::flow::NormalTermination,
  viskores::worklet::flow::StreamlineAnalysis<viskores::Particle>>(
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::RK4Integrator<
      viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::VelocityField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::VelocityField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>> const&,
  viskores::cont::ArrayHandle<viskores::Particle, viskores::cont::StorageTagBasic>&,
  viskores::worklet::flow::NormalTermination const&,
  viskores::worklet::flow::StreamlineAnalysis<viskores::Particle>&);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template VISKORES_FILTER_FLOW_EXPORT void viskores::worklet::flow::ParticleAdvection::Run<
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::RK4Integrator<
      viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::ElectroMagneticField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::ElectroMagneticField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
  viskores::ChargedParticle,
  viskores::cont::StorageTagBasic,
  viskores::worklet::flow::NormalTermination,
  viskores::worklet::flow::StreamlineAnalysis<viskores::ChargedParticle>>(
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::RK4Integrator<
      viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::ElectroMagneticField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::ElectroMagneticField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>> const&,
  viskores::cont::ArrayHandle<viskores::ChargedParticle, viskores::cont::StorageTagBasic>&,
  viskores::worklet::flow::NormalTermination const&,
  viskores::worklet::flow::StreamlineAnalysis<viskores::ChargedParticle>&);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template VISKORES_FILTER_FLOW_EXPORT void viskores::worklet::flow::ParticleAdvection::Run<
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::RK4Integrator<
      viskores::worklet::flow::TemporalGridEvaluator<viskores::worklet::flow::VelocityField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::TemporalGridEvaluator<viskores::worklet::flow::VelocityField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
  viskores::Particle,
  viskores::cont::StorageTagBasic,
  viskores::worklet::flow::NormalTermination,
  viskores::worklet::flow::NoAnalysis<viskores::Particle>>(
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::RK4Integrator<
      viskores::worklet::flow::TemporalGridEvaluator<viskores::worklet::flow::VelocityField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::TemporalGridEvaluator<viskores::worklet::flow::VelocityField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>> const&,
  viskores::cont::ArrayHandle<viskores::Particle, viskores::cont::StorageTagBasic>&,
  viskores::worklet::flow::NormalTermination const&,
  viskores::worklet::flow::NoAnalysis<viskores::Particle>&);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template VISKORES_FILTER_FLOW_EXPORT void viskores::worklet::flow::ParticleAdvection::Run<
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::RK4Integrator<
      viskores::worklet::flow::TemporalGridEvaluator<viskores::worklet::flow::VelocityField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::TemporalGridEvaluator<viskores::worklet::flow::VelocityField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
  viskores::Particle,
  viskores::cont::StorageTagBasic,
  viskores::worklet::flow::NormalTermination,
  viskores::worklet::flow::StreamlineAnalysis<viskores::Particle>>(
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::RK4Integrator<
      viskores::worklet::flow::TemporalGridEvaluator<viskores::worklet::flow::VelocityField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::TemporalGridEvaluator<viskores::worklet::flow::VelocityField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>> const&,
  viskores::cont::ArrayHandle<viskores::Particle, viskores::cont::StorageTagBasic>&,
  viskores::worklet::flow::NormalTermination const&,
  viskores::worklet::flow::StreamlineAnalysis<viskores::Particle>&);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template VISKORES_FILTER_FLOW_EXPORT void viskores::worklet::flow::ParticleAdvection::Run<
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::EulerIntegrator<
      viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::VelocityField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::VelocityField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
  viskores::Particle,
  viskores::cont::StorageTagBasic,
  viskores::worklet::flow::NormalTermination,
  viskores::worklet::flow::NoAnalysis<viskores::Particle>>(
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::EulerIntegrator<
      viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::VelocityField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::VelocityField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>> const&,
  viskores::cont::ArrayHandle<viskores::Particle, viskores::cont::StorageTagBasic>&,
  viskores::worklet::flow::NormalTermination const&,
  viskores::worklet::flow::NoAnalysis<viskores::Particle>&);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template VISKORES_FILTER_FLOW_EXPORT void viskores::worklet::flow::ParticleAdvection::Run<
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::EulerIntegrator<
      viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::VelocityField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::VelocityField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
  viskores::Particle,
  viskores::cont::StorageTagBasic,
  viskores::worklet::flow::NormalTermination,
  viskores::worklet::flow::StreamlineAnalysis<viskores::Particle>>(
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::EulerIntegrator<
      viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::VelocityField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::VelocityField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>> const&,
  viskores::cont::ArrayHandle<viskores::Particle, viskores::cont::StorageTagBasic>&,
  viskores::worklet::flow::NormalTermination const&,
  viskores::worklet::flow::StreamlineAnalysis<viskores::Particle>&);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template VISKORES_FILTER_FLOW_EXPORT void viskores::worklet::flow::ParticleAdvection::Run<
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::EulerIntegrator<
      viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::ElectroMagneticField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::ElectroMagneticField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
  viskores::ChargedParticle,
  viskores::cont::StorageTagBasic,
  viskores::worklet::flow::NormalTermination,
  viskores::worklet::flow::StreamlineAnalysis<viskores::ChargedParticle>>(
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::EulerIntegrator<
      viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::ElectroMagneticField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::GridEvaluator<viskores::worklet::flow::ElectroMagneticField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>> const&,
  viskores::cont::ArrayHandle<viskores::ChargedParticle, viskores::cont::StorageTagBasic>&,
  viskores::worklet::flow::NormalTermination const&,
  viskores::worklet::flow::StreamlineAnalysis<viskores::ChargedParticle>&);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template VISKORES_FILTER_FLOW_EXPORT void viskores::worklet::flow::ParticleAdvection::Run<
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::EulerIntegrator<
      viskores::worklet::flow::TemporalGridEvaluator<viskores::worklet::flow::VelocityField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::TemporalGridEvaluator<viskores::worklet::flow::VelocityField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
  viskores::Particle,
  viskores::cont::StorageTagBasic,
  viskores::worklet::flow::NormalTermination,
  viskores::worklet::flow::NoAnalysis<viskores::Particle>>(
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::EulerIntegrator<
      viskores::worklet::flow::TemporalGridEvaluator<viskores::worklet::flow::VelocityField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::TemporalGridEvaluator<viskores::worklet::flow::VelocityField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>> const&,
  viskores::cont::ArrayHandle<viskores::Particle, viskores::cont::StorageTagBasic>&,
  viskores::worklet::flow::NormalTermination const&,
  viskores::worklet::flow::NoAnalysis<viskores::Particle>&);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template VISKORES_FILTER_FLOW_EXPORT void viskores::worklet::flow::ParticleAdvection::Run<
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::EulerIntegrator<
      viskores::worklet::flow::TemporalGridEvaluator<viskores::worklet::flow::VelocityField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::TemporalGridEvaluator<viskores::worklet::flow::VelocityField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
  viskores::Particle,
  viskores::cont::StorageTagBasic,
  viskores::worklet::flow::NormalTermination,
  viskores::worklet::flow::StreamlineAnalysis<viskores::Particle>>(
  viskores::worklet::flow::Stepper<
    viskores::worklet::flow::EulerIntegrator<
      viskores::worklet::flow::TemporalGridEvaluator<viskores::worklet::flow::VelocityField<
        viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>>,
    viskores::worklet::flow::TemporalGridEvaluator<viskores::worklet::flow::VelocityField<
      viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>>>> const&,
  viskores::cont::ArrayHandle<viskores::Particle, viskores::cont::StorageTagBasic>&,
  viskores::worklet::flow::NormalTermination const&,
  viskores::worklet::flow::StreamlineAnalysis<viskores::Particle>&);
VISKORES_INSTANTIATION_END

#endif // viskores_filter_flow_worklet_ParticleAdvection_h
