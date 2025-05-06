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

#include <viskores/cont/Invoker.h>
#include <viskores/cont/ParticleArrayCopy.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace cont
{

namespace detail
{
struct CopyParticlePositionWorklet : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inParticle, FieldOut outPos);

  VISKORES_EXEC void operator()(const viskores::Particle& inParticle, viskores::Vec3f& outPos) const
  {
    outPos = inParticle.Pos;
  }
};

struct CopyParticleAllWorklet : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inParticle,
                                FieldOut outPos,
                                FieldOut outID,
                                FieldOut outSteps,
                                FieldOut outStatus,
                                FieldOut outTime);

  VISKORES_EXEC void operator()(const viskores::Particle& inParticle,
                                viskores::Vec3f& outPos,
                                viskores::Id& outID,
                                viskores::Id& outSteps,
                                viskores::ParticleStatus& outStatus,
                                viskores::FloatDefault& outTime) const
  {
    outPos = inParticle.Pos;
    outID = inParticle.ID;
    outSteps = inParticle.NumSteps;
    outStatus = inParticle.Status;
    outTime = inParticle.Time;
  }
};

} // namespace detail

VISKORES_CONT void ParticleArrayCopy(
  const viskores::cont::ArrayHandle<viskores::Particle, viskores::cont::StorageTagBasic>& inP,
  viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>& outPos)
{
  viskores::cont::Invoker invoke;
  detail::CopyParticlePositionWorklet worklet;

  invoke(worklet, inP, outPos);
}

/// \brief Copy all fields in viskores::Particle to standard types.
///
/// Given an \c ArrayHandle of viskores::Particle, this function copies the
/// position, ID, number of steps, status and time into a separate
/// \c ArrayHandle.
///


VISKORES_CONT void ParticleArrayCopy(
  const viskores::cont::ArrayHandle<viskores::Particle, viskores::cont::StorageTagBasic>& inP,
  viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>& outPos,
  viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic>& outID,
  viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic>& outSteps,
  viskores::cont::ArrayHandle<viskores::ParticleStatus, viskores::cont::StorageTagBasic>& outStatus,
  viskores::cont::ArrayHandle<viskores::FloatDefault, viskores::cont::StorageTagBasic>& outTime)
{
  viskores::cont::Invoker invoke;
  detail::CopyParticleAllWorklet worklet;

  invoke(worklet, inP, outPos, outID, outSteps, outStatus, outTime);
}
}
} // namespace viskores::cont
