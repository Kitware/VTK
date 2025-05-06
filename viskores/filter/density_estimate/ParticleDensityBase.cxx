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

#include <viskores/filter/density_estimate/ParticleDensityBase.h>
#include <viskores/worklet/WorkletMapField.h>

namespace
{
class DivideByVolumeWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut field);
  using ExecutionSignature = void(_1);

  VISKORES_EXEC_CONT
  explicit DivideByVolumeWorklet(viskores::Float64 volume)
    : Volume(volume)
  {
  }

  template <typename T>
  VISKORES_EXEC void operator()(T& value) const
  {
    value = static_cast<T>(value / Volume);
  }

private:
  viskores::Float64 Volume;
}; // class DivideByVolumeWorklet
}

namespace viskores
{
namespace filter
{
namespace density_estimate
{

VISKORES_CONT void ParticleDensityBase::DoDivideByVolume(
  const viskores::cont::UnknownArrayHandle& density) const
{
  auto volume = this->Spacing[0] * this->Spacing[1] * this->Spacing[2];
  auto resolve = [&](const auto& concreteDensity)
  { this->Invoke(DivideByVolumeWorklet{ volume }, concreteDensity); };
  this->CastAndCallScalarField(density, resolve);
}
} // namespace density_estimate
} // namespace filter
} // namespace viskores
