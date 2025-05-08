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

#ifndef viskores_worklet_particleadvection_termination
#define viskores_worklet_particleadvection_termination

#include <viskores/Types.h>
#include <viskores/cont/ExecutionObjectBase.h>

namespace viskores
{
namespace worklet
{
namespace flow
{

class NormalTerminationExec
{
public:
  VISKORES_EXEC_CONT
  NormalTerminationExec()
    : MaxSteps(0)
  {
  }

  VISKORES_EXEC_CONT
  NormalTerminationExec(viskores::Id maxSteps)
    : MaxSteps(maxSteps)
  {
  }

  template <typename ParticleType>
  VISKORES_EXEC bool CheckTermination(ParticleType& particle) const
  {
    /// Checks particle properties to make a decision for termination
    /// -- Check if the particle is out of spatial boundaries
    /// -- Check if the particle has reached the maximum number of steps
    /// -- Check if the particle is in a zero velocity region
    auto& status = particle.GetStatus();
    if (particle.GetNumberOfSteps() == this->MaxSteps)
    {
      status.SetTerminate();
      particle.SetStatus(status);
    }
    bool terminate = status.CheckOk() && !status.CheckTerminate() && !status.CheckSpatialBounds() &&
      !status.CheckTemporalBounds() && !status.CheckInGhostCell() && !status.CheckZeroVelocity();
    return terminate;
  }

private:
  viskores::Id MaxSteps;
};

class NormalTermination : public viskores::cont::ExecutionObjectBase
{
public:
  VISKORES_CONT
  NormalTermination()
    : MaxSteps(0)
  {
  }

  VISKORES_CONT
  NormalTermination(viskores::Id maxSteps)
    : MaxSteps(maxSteps)
  {
  }

  VISKORES_CONT
  viskores::Id AllocationSize() const { return this->MaxSteps; }

  VISKORES_CONT viskores::worklet::flow::NormalTerminationExec PrepareForExecution(
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token) const
  {
    (void)device;
    (void)token;
    return viskores::worklet::flow::NormalTerminationExec(MaxSteps);
  }

private:
  viskores::Id MaxSteps;
};

class PoincareTerminationExec
{
public:
  VISKORES_EXEC_CONT
  PoincareTerminationExec()
    : MaxSteps(0)
    , MaxPunctures(0)
  {
  }

  VISKORES_EXEC_CONT
  PoincareTerminationExec(viskores::Id maxSteps, viskores::Id maxPunctures)
    : MaxSteps(maxSteps)
    , MaxPunctures(maxPunctures)
  {
  }

  template <typename ParticleType>
  VISKORES_EXEC bool CheckTermination(ParticleType& particle) const
  {
    /// Checks particle properties to make a decision for termination
    /// -- Check if the particle is out of spatial boundaries
    /// -- Check if the particle has reached the maximum number of steps
    /// -- Check if the particle is in a zero velocity region
    auto& status = particle.GetStatus();
    if (particle.GetNumberOfSteps() >= this->MaxSteps ||
        particle.GetNumberOfPunctures() >= this->MaxPunctures)
    {
      status.SetTerminate();
      particle.SetStatus(status);
    }
    bool terminate = status.CheckOk() && !status.CheckTerminate() && !status.CheckSpatialBounds() &&
      !status.CheckTemporalBounds() && !status.CheckInGhostCell() && !status.CheckZeroVelocity();
    return terminate;
  }

private:
  viskores::Id MaxSteps;
  viskores::Id MaxPunctures;
};

class PoincareTermination : public viskores::cont::ExecutionObjectBase
{
public:
  VISKORES_CONT
  PoincareTermination()
    : MaxSteps(0)
    , MaxPunctures(0)
  {
  }

  VISKORES_CONT
  PoincareTermination(viskores::Id maxSteps, viskores::Id maxPunctures)
    : MaxSteps(maxSteps)
    , MaxPunctures(maxPunctures)
  {
  }

  VISKORES_CONT
  viskores::Id AllocationSize() const { return this->MaxPunctures; }

  VISKORES_CONT viskores::worklet::flow::PoincareTerminationExec PrepareForExecution(
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token) const
  {
    (void)device;
    (void)token;
    return viskores::worklet::flow::PoincareTerminationExec(MaxSteps, MaxPunctures);
  }

private:
  viskores::Id MaxSteps;
  viskores::Id MaxPunctures;
};

} // namespace particleadvection
} // namespace worklet
} // namespace viskores

#endif
