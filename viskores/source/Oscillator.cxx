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
#include <viskores/source/Oscillator.h>
#include <viskores/worklet/WorkletMapField.h>

namespace
{

struct Oscillation
{
  viskores::Vec3f Center;
  viskores::FloatDefault Radius;
  viskores::FloatDefault Omega;
  viskores::FloatDefault Zeta;
};

class OscillatorWorklet : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldOut);
  typedef _2 ExecutionSignature(_1);

  VISKORES_CONT
  void AddPeriodic(viskores::FloatDefault x,
                   viskores::FloatDefault y,
                   viskores::FloatDefault z,
                   viskores::FloatDefault radius,
                   viskores::FloatDefault omega,
                   viskores::FloatDefault zeta)
  {
    if (this->PeriodicOscillators.GetNumberOfComponents() < MAX_OSCILLATORS)
    {
      this->PeriodicOscillators.Append(Oscillation{ { x, y, z }, radius, omega, zeta });
    }
  }

  VISKORES_CONT
  void AddDamped(viskores::FloatDefault x,
                 viskores::FloatDefault y,
                 viskores::FloatDefault z,
                 viskores::FloatDefault radius,
                 viskores::FloatDefault omega,
                 viskores::FloatDefault zeta)
  {
    if (this->DampedOscillators.GetNumberOfComponents() < MAX_OSCILLATORS)
    {
      this->DampedOscillators.Append(Oscillation{ { x, y, z }, radius, omega, zeta });
    }
  }

  VISKORES_CONT
  void AddDecaying(viskores::FloatDefault x,
                   viskores::FloatDefault y,
                   viskores::FloatDefault z,
                   viskores::FloatDefault radius,
                   viskores::FloatDefault omega,
                   viskores::FloatDefault zeta)
  {
    if (this->DecayingOscillators.GetNumberOfComponents() < MAX_OSCILLATORS)
    {
      this->DecayingOscillators.Append(Oscillation{ { x, y, z }, radius, omega, zeta });
    }
  }

  VISKORES_CONT
  void SetTime(viskores::FloatDefault time) { this->Time = time; }

  VISKORES_EXEC
  viskores::FloatDefault operator()(const viskores::Vec3f& vec) const
  {
    viskores::IdComponent oIdx;
    viskores::FloatDefault t0, t, result = 0;
    const Oscillation* oscillator;

    t0 = 0.0;
    t = viskores::FloatDefault(this->Time * 2 * 3.14159265358979323846);

    // Compute damped
    for (oIdx = 0; oIdx < this->DampedOscillators.GetNumberOfComponents(); oIdx++)
    {
      oscillator = &this->DampedOscillators[oIdx];

      viskores::Vec3f delta = oscillator->Center - vec;
      viskores::FloatDefault dist2 = dot(delta, delta);
      viskores::FloatDefault dist_damp =
        viskores::Exp(-dist2 / (2 * oscillator->Radius * oscillator->Radius));
      viskores::FloatDefault phi = viskores::ACos(oscillator->Zeta);
      viskores::FloatDefault val = viskores::FloatDefault(
        1. -
        viskores::Exp(-oscillator->Zeta * oscillator->Omega * t0) *
          (viskores::Sin(viskores::Sqrt(1 - oscillator->Zeta * oscillator->Zeta) *
                           oscillator->Omega * t +
                         phi) /
           viskores::Sin(phi)));
      result += val * dist_damp;
    }

    // Compute decaying
    for (oIdx = 0; oIdx < this->DecayingOscillators.GetNumberOfComponents(); oIdx++)
    {
      oscillator = &this->DecayingOscillators[oIdx];
      t = t0 + 1 / oscillator->Omega;
      viskores::Vec3f delta = oscillator->Center - vec;
      viskores::FloatDefault dist2 = dot(delta, delta);
      viskores::FloatDefault dist_damp =
        viskores::Exp(-dist2 / (2 * oscillator->Radius * oscillator->Radius));
      viskores::FloatDefault val = viskores::Sin(t / oscillator->Omega) / (oscillator->Omega * t);
      result += val * dist_damp;
    }

    // Compute periodic
    for (oIdx = 0; oIdx < this->PeriodicOscillators.GetNumberOfComponents(); oIdx++)
    {
      oscillator = &this->PeriodicOscillators[oIdx];
      t = t0 + 1 / oscillator->Omega;
      viskores::Vec3f delta = oscillator->Center - vec;
      viskores::FloatDefault dist2 = dot(delta, delta);
      viskores::FloatDefault dist_damp =
        viskores::Exp(-dist2 / (2 * oscillator->Radius * oscillator->Radius));
      viskores::FloatDefault val = viskores::Sin(t / oscillator->Omega);
      result += val * dist_damp;
    }

    // We are done...
    return result;
  }

private:
  static constexpr viskores::IdComponent MAX_OSCILLATORS = 10;
  viskores::VecVariable<Oscillation, MAX_OSCILLATORS> PeriodicOscillators;
  viskores::VecVariable<Oscillation, MAX_OSCILLATORS> DampedOscillators;
  viskores::VecVariable<Oscillation, MAX_OSCILLATORS> DecayingOscillators;
  viskores::FloatDefault Time{};
}; // OscillatorWorklet

} // anonymous namespace

namespace viskores
{
namespace source
{

//-----------------------------------------------------------------------------
struct Oscillator::InternalStruct
{
  viskores::Id3 PointDimensions = { 3, 3, 3 };
  OscillatorWorklet Worklet;
};

//-----------------------------------------------------------------------------
Oscillator::Oscillator()
  : Internals(new InternalStruct)
{
}

//-----------------------------------------------------------------------------
Oscillator::Oscillator(viskores::Id3 dims)
  : Internals(new InternalStruct)
{
  this->SetCellDimensions(dims);
}

Oscillator::~Oscillator() = default;

//-----------------------------------------------------------------------------
void Oscillator::SetPointDimensions(viskores::Id3 pointDimensions)
{
  this->Internals->PointDimensions = pointDimensions;
}
viskores::Id3 Oscillator::GetPointDimensions() const
{
  return this->Internals->PointDimensions;
}

void Oscillator::SetCellDimensions(viskores::Id3 cellDimensions)
{
  this->SetPointDimensions(cellDimensions + viskores::Id3(1));
}
viskores::Id3 Oscillator::GetCellDimensions() const
{
  return this->GetPointDimensions() - viskores::Id3(1);
}

//-----------------------------------------------------------------------------
void Oscillator::SetTime(viskores::FloatDefault time)
{
  this->Internals->Worklet.SetTime(time);
}

//-----------------------------------------------------------------------------
void Oscillator::AddPeriodic(viskores::FloatDefault x,
                             viskores::FloatDefault y,
                             viskores::FloatDefault z,
                             viskores::FloatDefault radius,
                             viskores::FloatDefault omega,
                             viskores::FloatDefault zeta)
{
  this->Internals->Worklet.AddPeriodic(x, y, z, radius, omega, zeta);
}

//-----------------------------------------------------------------------------
void Oscillator::AddDamped(viskores::FloatDefault x,
                           viskores::FloatDefault y,
                           viskores::FloatDefault z,
                           viskores::FloatDefault radius,
                           viskores::FloatDefault omega,
                           viskores::FloatDefault zeta)
{
  this->Internals->Worklet.AddDamped(x, y, z, radius, omega, zeta);
}

//-----------------------------------------------------------------------------
void Oscillator::AddDecaying(viskores::FloatDefault x,
                             viskores::FloatDefault y,
                             viskores::FloatDefault z,
                             viskores::FloatDefault radius,
                             viskores::FloatDefault omega,
                             viskores::FloatDefault zeta)
{
  this->Internals->Worklet.AddDecaying(x, y, z, radius, omega, zeta);
}


//-----------------------------------------------------------------------------
viskores::cont::DataSet Oscillator::DoExecute() const
{
  VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Perf);

  viskores::cont::DataSet dataSet;

  viskores::cont::CellSetStructured<3> cellSet;
  viskores::Id3 pointDims = this->GetPointDimensions();
  cellSet.SetPointDimensions(pointDims);
  dataSet.SetCellSet(cellSet);

  viskores::Id3 cellDims = this->GetCellDimensions();
  const viskores::Vec3f origin(0.0f, 0.0f, 0.0f);
  const viskores::Vec3f spacing(1.0f / static_cast<viskores::FloatDefault>(cellDims[0]),
                                1.0f / static_cast<viskores::FloatDefault>(cellDims[1]),
                                1.0f / static_cast<viskores::FloatDefault>(cellDims[2]));

  viskores::cont::ArrayHandleUniformPointCoordinates coordinates(pointDims, origin, spacing);
  dataSet.AddCoordinateSystem(viskores::cont::CoordinateSystem("coordinates", coordinates));


  viskores::cont::ArrayHandle<viskores::FloatDefault> outArray;
  this->Invoke(this->Internals->Worklet, coordinates, outArray);
  dataSet.AddField(viskores::cont::make_FieldPoint("oscillating", outArray));

  return dataSet;
}
}
} // namespace viskores::filter
