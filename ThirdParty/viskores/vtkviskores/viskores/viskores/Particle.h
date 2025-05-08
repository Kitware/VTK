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
#ifndef viskores_Particle_h
#define viskores_Particle_h

#include <ostream>
#include <viskores/Bitset.h>
#include <viskores/VecVariable.h>
#include <viskores/VectorAnalysis.h>
#include <viskores/cont/Serialization.h>

namespace viskores
{

//Bit field describing the status:
class ParticleStatus : public viskores::Bitset<viskores::UInt8>
{
public:
  VISKORES_EXEC_CONT ParticleStatus()
  {
    this->SetOk();
    this->ClearTerminate();
  }

  VISKORES_EXEC_CONT void SetOk() { this->set(this->SUCCESS_BIT); }
  VISKORES_EXEC_CONT bool CheckOk() const { return this->test(this->SUCCESS_BIT); }

  VISKORES_EXEC_CONT void SetFail() { this->reset(this->SUCCESS_BIT); }
  VISKORES_EXEC_CONT bool CheckFail() const { return !this->test(this->SUCCESS_BIT); }

  VISKORES_EXEC_CONT void SetTerminate() { this->set(this->TERMINATE_BIT); }
  VISKORES_EXEC_CONT void ClearTerminate() { this->reset(this->TERMINATE_BIT); }
  VISKORES_EXEC_CONT bool CheckTerminate() const { return this->test(this->TERMINATE_BIT); }

  VISKORES_EXEC_CONT void SetSpatialBounds() { this->set(this->SPATIAL_BOUNDS_BIT); }
  VISKORES_EXEC_CONT void ClearSpatialBounds() { this->reset(this->SPATIAL_BOUNDS_BIT); }
  VISKORES_EXEC_CONT bool CheckSpatialBounds() const
  {
    return this->test(this->SPATIAL_BOUNDS_BIT);
  }

  VISKORES_EXEC_CONT void SetTemporalBounds() { this->set(this->TEMPORAL_BOUNDS_BIT); }
  VISKORES_EXEC_CONT void ClearTemporalBounds() { this->reset(this->TEMPORAL_BOUNDS_BIT); }
  VISKORES_EXEC_CONT bool CheckTemporalBounds() const
  {
    return this->test(this->TEMPORAL_BOUNDS_BIT);
  }

  VISKORES_EXEC_CONT void SetTookAnySteps() { this->set(this->TOOK_ANY_STEPS_BIT); }
  VISKORES_EXEC_CONT void ClearTookAnySteps() { this->reset(this->TOOK_ANY_STEPS_BIT); }
  VISKORES_EXEC_CONT bool CheckTookAnySteps() const { return this->test(this->TOOK_ANY_STEPS_BIT); }

  VISKORES_EXEC_CONT void SetInGhostCell() { this->set(this->IN_GHOST_CELL_BIT); }
  VISKORES_EXEC_CONT void ClearInGhostCell() { this->reset(this->IN_GHOST_CELL_BIT); }
  VISKORES_EXEC_CONT bool CheckInGhostCell() const { return this->test(this->IN_GHOST_CELL_BIT); }

  VISKORES_EXEC_CONT void SetZeroVelocity() { this->set(this->ZERO_VELOCITY); }
  VISKORES_EXEC_CONT void ClearZeroVelocity() { this->reset(this->ZERO_VELOCITY); }
  VISKORES_EXEC_CONT bool CheckZeroVelocity() const { return this->test(this->ZERO_VELOCITY); }

  VISKORES_EXEC_CONT bool CanContinue() const
  {
    return this->CheckOk() && !this->CheckTerminate() && !this->CheckSpatialBounds() &&
      !this->CheckTemporalBounds() && !this->CheckInGhostCell() && !this->CheckZeroVelocity();
  }

private:
  static constexpr viskores::Id SUCCESS_BIT = 0;
  static constexpr viskores::Id TERMINATE_BIT = 1;
  static constexpr viskores::Id SPATIAL_BOUNDS_BIT = 2;
  static constexpr viskores::Id TEMPORAL_BOUNDS_BIT = 3;
  static constexpr viskores::Id TOOK_ANY_STEPS_BIT = 4;
  static constexpr viskores::Id IN_GHOST_CELL_BIT = 5;
  static constexpr viskores::Id ZERO_VELOCITY = 6;
};

inline VISKORES_CONT std::ostream& operator<<(std::ostream& s,
                                              const viskores::ParticleStatus& status)
{
  s << "[ok= " << status.CheckOk();
  s << " term= " << status.CheckTerminate();
  s << " spat= " << status.CheckSpatialBounds();
  s << " temp= " << status.CheckTemporalBounds();
  s << " ghst= " << status.CheckInGhostCell();
  s << " zvel= " << status.CheckZeroVelocity();
  s << "]";
  return s;
}

class Particle
{
public:
  VISKORES_EXEC_CONT
  Particle() {}

  VISKORES_EXEC_CONT
  Particle(const viskores::Vec3f& p,
           const viskores::Id& id,
           const viskores::Id& numSteps = 0,
           const viskores::ParticleStatus& status = viskores::ParticleStatus(),
           const viskores::FloatDefault& time = 0)
    : Position(p)
    , ID(id)
    , NumSteps(numSteps)
    , Status(status)
    , Time(time)
  {
  }

  VISKORES_EXEC_CONT
  Particle(const viskores::Particle& p)
    : Position(p.Position)
    , ID(p.ID)
    , NumSteps(p.NumSteps)
    , Status(p.Status)
    , Time(p.Time)
  {
  }

  viskores::Particle& operator=(const viskores::Particle&) = default;

  VISKORES_EXEC_CONT ~Particle() noexcept
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  VISKORES_EXEC_CONT const viskores::Vec3f& GetPosition() const { return this->Position; }
  VISKORES_EXEC_CONT void SetPosition(const viskores::Vec3f& position)
  {
    this->Position = position;
  }

  VISKORES_EXEC_CONT viskores::Id GetID() const { return this->ID; }
  VISKORES_EXEC_CONT void SetID(viskores::Id id) { this->ID = id; }

  VISKORES_EXEC_CONT viskores::Id GetNumberOfSteps() const { return this->NumSteps; }
  VISKORES_EXEC_CONT void SetNumberOfSteps(viskores::Id numSteps) { this->NumSteps = numSteps; }

  VISKORES_EXEC_CONT viskores::ParticleStatus GetStatus() const { return this->Status; }
  VISKORES_EXEC_CONT viskores::ParticleStatus& GetStatus() { return this->Status; }
  VISKORES_EXEC_CONT void SetStatus(viskores::ParticleStatus status) { this->Status = status; }

  VISKORES_EXEC_CONT viskores::FloatDefault GetTime() const { return this->Time; }
  VISKORES_EXEC_CONT void SetTime(viskores::FloatDefault time) { this->Time = time; }

  VISKORES_EXEC_CONT
  viskores::Vec3f Velocity(const viskores::VecVariable<viskores::Vec3f, 2>& vectors,
                           const viskores::FloatDefault& viskoresNotUsed(length)) const
  {
    // Velocity is evaluated from the Velocity field
    // and is not influenced by the particle
    VISKORES_ASSERT(vectors.GetNumberOfComponents() > 0);
    return vectors[0];
  }

  VISKORES_EXEC_CONT
  viskores::Vec3f GetEvaluationPosition(const viskores::FloatDefault& deltaT) const
  {
    (void)deltaT; // unused for a general particle advection case
    return this->Position;
  }

  inline VISKORES_CONT friend std::ostream& operator<<(std::ostream& out,
                                                       const viskores::Particle& p)
  {
    out << "v(" << p.Time << ") = " << p.Position << ", ID: " << p.ID
        << ", NumSteps: " << p.NumSteps << ", Status: " << p.Status;
    return out;
  }

private:
  viskores::Vec3f Position;
  viskores::Id ID = -1;
  viskores::Id NumSteps = 0;
  viskores::ParticleStatus Status;
  viskores::FloatDefault Time = 0;

public:
  static size_t Sizeof()
  {
    constexpr std::size_t sz = sizeof(viskores::Vec3f) // Pos
      + sizeof(viskores::Id)                           // ID
      + sizeof(viskores::Id)                           // NumSteps
      + sizeof(viskores::UInt8)                        // Status
      + sizeof(viskores::FloatDefault);                // Time

    return sz;
  }
};

class ChargedParticle
{
public:
  VISKORES_EXEC_CONT
  ChargedParticle() {}

  VISKORES_EXEC_CONT
  ChargedParticle(const viskores::Vec3f& position,
                  const viskores::Id& id,
                  const viskores::Float64& mass,
                  const viskores::Float64& charge,
                  const viskores::Float64& weighting,
                  const viskores::Vec3f& momentum,
                  const viskores::Id& numSteps = 0,
                  const viskores::ParticleStatus& status = viskores::ParticleStatus(),
                  const viskores::FloatDefault& time = 0)
    : Position(position)
    , ID(id)
    , NumSteps(numSteps)
    , Status(status)
    , Time(time)
    , Mass(mass)
    , Charge(charge)
    , Weighting(weighting)
    , Momentum(momentum)
  {
  }

  VISKORES_EXEC_CONT
  ChargedParticle(const viskores::ChargedParticle& other)
    : Position(other.Position)
    , ID(other.ID)
    , NumSteps(other.NumSteps)
    , Status(other.Status)
    , Time(other.Time)
    , Mass(other.Mass)
    , Charge(other.Charge)
    , Weighting(other.Weighting)
    , Momentum(other.Momentum)
  {
  }

  viskores::ChargedParticle& operator=(const viskores::ChargedParticle&) = default;

  VISKORES_EXEC_CONT
  ~ChargedParticle() noexcept
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  VISKORES_EXEC_CONT const viskores::Vec3f& GetPosition() const { return this->Position; }
  VISKORES_EXEC_CONT void SetPosition(const viskores::Vec3f& position)
  {
    this->Position = position;
  }

  VISKORES_EXEC_CONT viskores::Id GetID() const { return this->ID; }
  VISKORES_EXEC_CONT void SetID(viskores::Id id) { this->ID = id; }

  VISKORES_EXEC_CONT viskores::Id GetNumberOfSteps() const { return this->NumSteps; }
  VISKORES_EXEC_CONT void SetNumberOfSteps(viskores::Id numSteps) { this->NumSteps = numSteps; }

  VISKORES_EXEC_CONT viskores::ParticleStatus GetStatus() const { return this->Status; }
  VISKORES_EXEC_CONT viskores::ParticleStatus& GetStatus() { return this->Status; }
  VISKORES_EXEC_CONT void SetStatus(viskores::ParticleStatus status) { this->Status = status; }

  VISKORES_EXEC_CONT viskores::FloatDefault GetTime() const { return this->Time; }
  VISKORES_EXEC_CONT void SetTime(viskores::FloatDefault time) { this->Time = time; }

  VISKORES_EXEC_CONT
  viskores::Float64 Gamma(const viskores::Vec3f& momentum, bool reciprocal = false) const
  {
    constexpr viskores::FloatDefault c2 = SPEED_OF_LIGHT * SPEED_OF_LIGHT;
    const viskores::Float64 fMom2 = viskores::MagnitudeSquared(momentum);
    const viskores::Float64 m2 = this->Mass * this->Mass;
    const viskores::Float64 m2_c2_reci = 1.0 / (m2 * c2);
    if (reciprocal)
      return viskores::RSqrt(1.0 + fMom2 * m2_c2_reci);
    else
      return viskores::Sqrt(1.0 + fMom2 * m2_c2_reci);
  }

  VISKORES_EXEC_CONT
  viskores::Vec3f Velocity(const viskores::VecVariable<viskores::Vec3f, 2>& vectors,
                           const viskores::FloatDefault& length) const
  {
    VISKORES_ASSERT(vectors.GetNumberOfComponents() == 2);

    // Suppress unused warning
    (void)this->Weighting;

    viskores::Vec3f eField = vectors[0];
    viskores::Vec3f bField = vectors[1];

    const viskores::Float64 QoM = this->Charge / this->Mass;
    const viskores::Vec3f mom_minus = this->Momentum + (0.5 * this->Charge * eField * length);

    // Get reciprocal of Gamma
    viskores::Vec3f gamma_reci = static_cast<viskores::FloatDefault>(this->Gamma(mom_minus, true));
    const viskores::Vec3f t = 0.5 * QoM * length * bField * gamma_reci;
    const viskores::Vec3f s = 2.0f * t * (1.0 / (1.0 + viskores::Magnitude(t)));
    const viskores::Vec3f mom_prime = mom_minus + viskores::Cross(mom_minus, t);
    const viskores::Vec3f mom_plus = mom_minus + viskores::Cross(mom_prime, s);

    const viskores::Vec3f mom_new = mom_plus + 0.5 * this->Charge * eField * length;
    this->Momentum = mom_new;

    // momentum = velocity * mass * gamma;
    // --> velocity = momentum / (mass * gamma)
    // --> velocity = ( momentum / mass ) * gamma_reci
    viskores::Vec3f velocity = (mom_new / this->Mass) * this->Gamma(mom_new, true);
    return velocity;
  }

  VISKORES_EXEC_CONT
  viskores::Vec3f GetEvaluationPosition(const viskores::FloatDefault& deltaT) const
  {
    // Translation is in -ve Z direction,
    // this needs to be a parameter.
    auto translation = this->NumSteps * deltaT * SPEED_OF_LIGHT * viskores::Vec3f{ 0., 0., -1.0 };
    return this->Position + translation;
  }

  inline VISKORES_CONT friend std::ostream& operator<<(std::ostream& out,
                                                       const viskores::ChargedParticle& p)
  {
    out << "v(" << p.Time << ") = " << p.Position << ", ID: " << p.ID
        << ", NumSteps: " << p.NumSteps << ", Status: " << p.Status;
    return out;
  }

private:
  viskores::Vec3f Position;
  viskores::Id ID = -1;
  viskores::Id NumSteps = 0;
  viskores::ParticleStatus Status;
  viskores::FloatDefault Time = 0;
  viskores::Float64 Mass;
  viskores::Float64 Charge;
  viskores::Float64 Weighting;
  mutable viskores::Vec3f Momentum;
  constexpr static viskores::FloatDefault SPEED_OF_LIGHT =
    static_cast<viskores::FloatDefault>(2.99792458e8);

  friend struct mangled_diy_namespace::Serialization<viskores::ChargedParticle>;

public:
  static size_t Sizeof()
  {
    constexpr std::size_t sz = sizeof(viskores::Vec3f) // Pos
      + sizeof(viskores::Id)                           // ID
      + sizeof(viskores::Id)                           // NumSteps
      + sizeof(viskores::UInt8)                        // Status
      + sizeof(viskores::FloatDefault)                 // Time
      + sizeof(viskores::Float64)                      //Mass
      + sizeof(viskores::Float64)                      //Charge
      + sizeof(viskores::Float64)                      //Weighting
      + sizeof(viskores::Vec3f);                       //Momentum

    return sz;
  }
};

} //namespace viskores

namespace mangled_diy_namespace
{
template <>
struct Serialization<viskores::Particle>
{
public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const viskores::Particle& p)
  {
    viskoresdiy::save(bb, p.GetPosition());
    viskoresdiy::save(bb, p.GetID());
    viskoresdiy::save(bb, p.GetNumberOfSteps());
    viskoresdiy::save(bb, p.GetStatus());
    viskoresdiy::save(bb, p.GetTime());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, viskores::Particle& p)
  {
    viskores::Vec3f pos;
    viskoresdiy::load(bb, pos);
    p.SetPosition(pos);

    viskores::Id id;
    viskoresdiy::load(bb, id);
    p.SetID(id);

    viskores::Id numSteps;
    viskoresdiy::load(bb, numSteps);
    p.SetNumberOfSteps(numSteps);

    viskores::ParticleStatus status;
    viskoresdiy::load(bb, status);
    p.SetStatus(status);

    viskores::FloatDefault time;
    viskoresdiy::load(bb, time);
    p.SetTime(time);
  }
};

template <>
struct Serialization<viskores::ChargedParticle>
{
public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const viskores::ChargedParticle& e)
  {
    viskoresdiy::save(bb, e.Position);
    viskoresdiy::save(bb, e.ID);
    viskoresdiy::save(bb, e.NumSteps);
    viskoresdiy::save(bb, e.Status);
    viskoresdiy::save(bb, e.Time);
    viskoresdiy::save(bb, e.Mass);
    viskoresdiy::save(bb, e.Charge);
    viskoresdiy::save(bb, e.Weighting);
    viskoresdiy::save(bb, e.Momentum);
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, viskores::ChargedParticle& e)
  {
    viskoresdiy::load(bb, e.Position);
    viskoresdiy::load(bb, e.ID);
    viskoresdiy::load(bb, e.NumSteps);
    viskoresdiy::load(bb, e.Status);
    viskoresdiy::load(bb, e.Time);
    viskoresdiy::load(bb, e.Mass);
    viskoresdiy::load(bb, e.Charge);
    viskoresdiy::load(bb, e.Weighting);
    viskoresdiy::load(bb, e.Momentum);
  }
};
}

#endif // viskores_Particle_h
