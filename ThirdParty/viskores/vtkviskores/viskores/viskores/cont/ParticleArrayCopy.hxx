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

#ifndef viskores_cont_ParticleArrayCopy_hxx
#define viskores_cont_ParticleArrayCopy_hxx

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/ParticleArrayCopy.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace cont
{

namespace detail
{

template <typename ParticleType>
struct ExtractPositionFunctor
{
  VISKORES_EXEC_CONT
  viskores::Vec3f operator()(const ParticleType& p) const { return p.GetPosition(); }
};

template <typename ParticleType>
struct ExtractTerminatedFunctor
{
  VISKORES_EXEC_CONT
  bool operator()(const ParticleType& p) const { return p.GetStatus().CheckTerminate(); }
};

template <typename ParticleType>
struct CopyParticleAllWorklet : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inParticle,
                                FieldOut outPos,
                                FieldOut outID,
                                FieldOut outSteps,
                                FieldOut outStatus,
                                FieldOut outTime);

  VISKORES_EXEC void operator()(const ParticleType& inParticle,
                                viskores::Vec3f& outPos,
                                viskores::Id& outID,
                                viskores::Id& outSteps,
                                viskores::ParticleStatus& outStatus,
                                viskores::FloatDefault& outTime) const
  {
    outPos = inParticle.GetPosition();
    outID = inParticle.GetID();
    outSteps = inParticle.GetNumberOfSteps();
    outStatus = inParticle.GetStatus();
    outTime = inParticle.GetTime();
  }
};

} // namespace detail


template <typename ParticleType>
VISKORES_ALWAYS_EXPORT inline void ParticleArrayCopy(
  const viskores::cont::ArrayHandle<ParticleType, viskores::cont::StorageTagBasic>& inP,
  viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>& outPos,
  bool CopyTerminatedOnly)
{
  auto posTrn =
    viskores::cont::make_ArrayHandleTransform(inP, detail::ExtractPositionFunctor<ParticleType>());

  if (CopyTerminatedOnly)
  {
    auto termTrn = viskores::cont::make_ArrayHandleTransform(
      inP, detail::ExtractTerminatedFunctor<ParticleType>());
    viskores::cont::Algorithm::CopyIf(posTrn, termTrn, outPos);
  }
  else
    viskores::cont::Algorithm::Copy(posTrn, outPos);
}


template <typename ParticleType>
VISKORES_ALWAYS_EXPORT inline void ParticleArrayCopy(
  const std::vector<viskores::cont::ArrayHandle<ParticleType, viskores::cont::StorageTagBasic>>&
    inputs,
  viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>& outPos)
{
  viskores::Id numParticles = 0;
  for (const auto& v : inputs)
    numParticles += v.GetNumberOfValues();
  outPos.Allocate(numParticles);

  viskores::Id idx = 0;
  for (const auto& v : inputs)
  {
    auto posTrn =
      viskores::cont::make_ArrayHandleTransform(v, detail::ExtractPositionFunctor<ParticleType>());
    viskores::Id n = posTrn.GetNumberOfValues();
    viskores::cont::Algorithm::CopySubRange(posTrn, 0, n, outPos, idx);
    idx += n;
  }
}


/// \brief Copy all fields in viskores::Particle to standard types.
///
/// Given an \c ArrayHandle of viskores::Particle, this function copies the
/// position, ID, number of steps, status and time into a separate
/// \c ArrayHandle.
///

template <typename ParticleType>
VISKORES_ALWAYS_EXPORT inline void ParticleArrayCopy(
  const viskores::cont::ArrayHandle<ParticleType, viskores::cont::StorageTagBasic>& inP,
  viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>& outPos,
  viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic>& outID,
  viskores::cont::ArrayHandle<viskores::Id, viskores::cont::StorageTagBasic>& outSteps,
  viskores::cont::ArrayHandle<viskores::ParticleStatus, viskores::cont::StorageTagBasic>& outStatus,
  viskores::cont::ArrayHandle<viskores::FloatDefault, viskores::cont::StorageTagBasic>& outTime)
{
  viskores::cont::Invoker invoke;
  detail::CopyParticleAllWorklet<ParticleType> worklet;

  invoke(worklet, inP, outPos, outID, outSteps, outStatus, outTime);
}
}
} // namespace viskores::cont

#endif //viskores_cont_ParticleArrayCopy_hxx
