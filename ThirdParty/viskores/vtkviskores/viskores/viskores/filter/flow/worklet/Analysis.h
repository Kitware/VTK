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

#ifndef viskores_worklet_particleadvection_analysis
#define viskores_worklet_particleadvection_analysis

#include <viskores/Particle.h>
#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/filter/flow/viskores_filter_flow_export.h>

namespace viskores
{
namespace worklet
{
namespace flow
{

template <typename ParticleType>
class VISKORES_FILTER_FLOW_EXPORT NoAnalysisExec
{
public:
  VISKORES_EXEC_CONT
  NoAnalysisExec() {}

  VISKORES_EXEC void PreStepAnalyze(const viskores::Id index, const ParticleType& particle)
  {
    (void)index;
    (void)particle;
  }

  //template <typename ParticleType>
  VISKORES_EXEC void Analyze(const viskores::Id index,
                             const ParticleType& oldParticle,
                             const ParticleType& newParticle)
  {
    // Do nothing
    (void)index;
    (void)oldParticle;
    (void)newParticle;
  }
};

template <typename ParticleType>
class NoAnalysis : public viskores::cont::ExecutionObjectBase
{
public:
  // Intended to store advected particles after Finalize
  viskores::cont::ArrayHandle<ParticleType> Particles;

  VISKORES_CONT
  NoAnalysis()
    : Particles()
  {
  }

  VISKORES_CONT
  void UseAsTemplate(const NoAnalysis& other) { (void)other; }

  VISKORES_CONT
  //template <typename ParticleType>
  void InitializeAnalysis(const viskores::cont::ArrayHandle<ParticleType>& particles)
  {
    (void)particles;
  }

  VISKORES_CONT
  //template <typename ParticleType>
  void FinalizeAnalysis(viskores::cont::ArrayHandle<ParticleType>& particles)
  {
    this->Particles = particles; //, viskores::CopyFlag::Off);
  }

  VISKORES_CONT NoAnalysisExec<ParticleType> PrepareForExecution(
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token) const
  {
    (void)device;
    (void)token;
    return NoAnalysisExec<ParticleType>();
  }

  VISKORES_CONT bool SupportPushOutOfBounds() const { return true; }

  VISKORES_CONT static bool MakeDataSet(viskores::cont::DataSet& dataset,
                                        const std::vector<NoAnalysis>& results);
};

template <typename ParticleType>
class VISKORES_FILTER_FLOW_EXPORT StreamlineAnalysisExec
{
public:
  VISKORES_EXEC_CONT
  StreamlineAnalysisExec()
    : NumParticles(0)
    , MaxSteps(0)
    , Streams()
    , StreamLengths()
    , Validity()
  {
  }

  VISKORES_CONT
  StreamlineAnalysisExec(viskores::Id numParticles,
                         viskores::Id maxSteps,
                         const viskores::cont::ArrayHandle<viskores::Vec3f>& streams,
                         const viskores::cont::ArrayHandle<viskores::Id>& streamLengths,
                         const viskores::cont::ArrayHandle<viskores::Id>& validity,
                         viskores::cont::DeviceAdapterId device,
                         viskores::cont::Token& token)
    : NumParticles(numParticles)
    , MaxSteps(maxSteps + 1)
  {
    Streams = streams.PrepareForOutput(this->NumParticles * this->MaxSteps, device, token);
    StreamLengths = streamLengths.PrepareForInPlace(device, token);
    Validity = validity.PrepareForInPlace(device, token);
  }

  VISKORES_EXEC void PreStepAnalyze(const viskores::Id index, const ParticleType& particle)
  {
    viskores::Id streamLength = this->StreamLengths.Get(index);
    if (streamLength == 0)
    {
      this->StreamLengths.Set(index, 1);
      viskores::Id loc = index * MaxSteps;
      this->Streams.Set(loc, particle.GetPosition());
      this->Validity.Set(loc, 1);
    }
  }

  //template <typename ParticleType>
  VISKORES_EXEC void Analyze(const viskores::Id index,
                             const ParticleType& oldParticle,
                             const ParticleType& newParticle)
  {
    (void)oldParticle;
    viskores::Id streamLength = this->StreamLengths.Get(index);
    viskores::Id loc = index * MaxSteps + streamLength;
    this->StreamLengths.Set(index, ++streamLength);
    this->Streams.Set(loc, newParticle.GetPosition());
    this->Validity.Set(loc, 1);
  }

private:
  using IdPortal = typename viskores::cont::ArrayHandle<viskores::Id>::WritePortalType;
  using VecPortal = typename viskores::cont::ArrayHandle<viskores::Vec3f>::WritePortalType;

  viskores::Id NumParticles;
  viskores::Id MaxSteps;
  VecPortal Streams;
  IdPortal StreamLengths;
  IdPortal Validity;
};

template <typename ParticleType>
class StreamlineAnalysis : public viskores::cont::ExecutionObjectBase
{
public:
  // Intended to store advected particles after Finalize
  viskores::cont::ArrayHandle<ParticleType> Particles;
  viskores::cont::ArrayHandle<viskores::Vec3f> Streams;
  viskores::cont::CellSetExplicit<> PolyLines;

  //Helper functor for compacting history
  struct IsOne
  {
    template <typename T>
    VISKORES_EXEC_CONT bool operator()(const T& x) const
    {
      return x == T(1);
    }
  };

  VISKORES_CONT
  StreamlineAnalysis()
    : Particles()
    , MaxSteps(0)
  {
  }

  VISKORES_CONT
  StreamlineAnalysis(viskores::Id maxSteps)
    : Particles()
    , MaxSteps(maxSteps)
  {
  }

  VISKORES_CONT
  void UseAsTemplate(const StreamlineAnalysis& other) { this->MaxSteps = other.MaxSteps; }

  VISKORES_CONT StreamlineAnalysisExec<ParticleType> PrepareForExecution(
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token) const
  {
    return StreamlineAnalysisExec<ParticleType>(this->NumParticles,
                                                this->MaxSteps,
                                                this->Streams,
                                                this->StreamLengths,
                                                this->Validity,
                                                device,
                                                token);
  }

  VISKORES_CONT bool SupportPushOutOfBounds() const { return true; }

  VISKORES_CONT
  void InitializeAnalysis(const viskores::cont::ArrayHandle<ParticleType>& particles);

  VISKORES_CONT
  //template <typename ParticleType>
  void FinalizeAnalysis(viskores::cont::ArrayHandle<ParticleType>& particles);


  VISKORES_CONT static bool MakeDataSet(viskores::cont::DataSet& dataset,
                                        const std::vector<StreamlineAnalysis>& results);

private:
  viskores::Id NumParticles;
  viskores::Id MaxSteps;

  viskores::cont::ArrayHandle<viskores::Id> StreamLengths;
  viskores::cont::ArrayHandle<viskores::Id> InitialLengths;
  viskores::cont::ArrayHandle<viskores::Id> Validity;
};

#ifndef viskores_filter_flow_worklet_Analysis_cxx
extern template class VISKORES_FILTER_FLOW_TEMPLATE_EXPORT NoAnalysis<viskores::Particle>;
extern template class VISKORES_FILTER_FLOW_TEMPLATE_EXPORT NoAnalysis<viskores::ChargedParticle>;
extern template class VISKORES_FILTER_FLOW_TEMPLATE_EXPORT StreamlineAnalysis<viskores::Particle>;
extern template class VISKORES_FILTER_FLOW_TEMPLATE_EXPORT
  StreamlineAnalysis<viskores::ChargedParticle>;
#endif //!viskores_filter_flow_worklet_Analysis_cxx

} // namespace flow
} // namespace worklet
} // namespace viskores

#endif //viskores_worklet_particleadvection_analysis
