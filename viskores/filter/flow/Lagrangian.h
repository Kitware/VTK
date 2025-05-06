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

#ifndef viskores_filter_flow_Lagrangian_h
#define viskores_filter_flow_Lagrangian_h

#include <viskores/Particle.h>
#include <viskores/filter/Filter.h>
#include <viskores/filter/flow/viskores_filter_flow_export.h>

namespace viskores
{
namespace filter
{
namespace flow
{

class VISKORES_FILTER_FLOW_EXPORT Lagrangian : public viskores::filter::Filter
{
public:
  VISKORES_CONT
  bool CanThread() const override { return false; }

  VISKORES_CONT
  void SetInitFlag(bool val) { this->InitFlag = val; }

  VISKORES_CONT
  void SetExtractFlows(bool val) { this->ExtractFlows = val; }

  VISKORES_CONT
  void SetResetParticles(bool val) { this->ResetParticles = val; }

  VISKORES_CONT
  void SetStepSize(viskores::Float32 val) { this->StepSize = val; }

  VISKORES_CONT
  void SetWriteFrequency(viskores::Id val) { this->WriteFrequency = val; }

  VISKORES_CONT
  void SetSeedResolutionInX(viskores::Id val) { this->ResX = val; }

  VISKORES_CONT
  void SetSeedResolutionInY(viskores::Id val) { this->ResY = val; }

  VISKORES_CONT
  void SetSeedResolutionInZ(viskores::Id val) { this->ResZ = val; }

  VISKORES_CONT
  void SetCustomSeedResolution(viskores::Id val) { this->CustRes = val; }

  VISKORES_CONT
  void SetSeedingResolution(viskores::Id3 val) { this->SeedRes = val; }

  VISKORES_CONT
  void UpdateSeedResolution(viskores::cont::DataSet input);

  VISKORES_CONT
  void InitializeSeedPositions(const viskores::cont::DataSet& input);

  VISKORES_CONT
  void SetCycle(viskores::Id cycle) { this->Cycle = cycle; }
  VISKORES_CONT
  viskores::Id GetCycle() const { return this->Cycle; }

  VISKORES_CONT
  void SetBasisParticles(const viskores::cont::ArrayHandle<viskores::Particle>& basisParticles)
  {
    this->BasisParticles = basisParticles;
  }
  VISKORES_CONT
  viskores::cont::ArrayHandle<viskores::Particle> GetBasisParticles() const
  {
    return this->BasisParticles;
  }

  VISKORES_CONT
  void SetBasisParticlesOriginal(
    const viskores::cont::ArrayHandle<viskores::Particle>& basisParticles)
  {
    this->BasisParticlesOriginal = basisParticles;
  }
  VISKORES_CONT
  viskores::cont::ArrayHandle<viskores::Particle> GetBasisParticlesOriginal() const
  {
    return this->BasisParticlesOriginal;
  }

  VISKORES_CONT
  void SetBasisParticleValidity(const viskores::cont::ArrayHandle<viskores::Id>& valid)
  {
    this->BasisParticlesValidity = valid;
  }
  VISKORES_CONT
  viskores::cont::ArrayHandle<viskores::Id> GetBasisParticleValidity() const
  {
    return this->BasisParticlesValidity;
  }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& inData) override;

  viskores::cont::ArrayHandle<viskores::Particle> BasisParticles;
  viskores::cont::ArrayHandle<viskores::Particle> BasisParticlesOriginal;
  viskores::cont::ArrayHandle<viskores::Id> BasisParticlesValidity;
  viskores::Id CustRes = 0;
  viskores::Id Cycle = 0;
  bool ExtractFlows = false;
  bool InitFlag = true;
  bool ResetParticles = true;
  viskores::Id ResX = 1;
  viskores::Id ResY = 1;
  viskores::Id ResZ = 1;
  viskores::FloatDefault StepSize;
  viskores::Id3 SeedRes = { 1, 1, 1 };
  viskores::Id WriteFrequency = 0;
};

}
}
} //viskores::filter::flow

#endif // #define viskores_filter_flow_Lagrangian_h
