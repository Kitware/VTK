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
#ifndef viskores_cont_ParticleArrayCopy_h
#define viskores_cont_ParticleArrayCopy_h

#include <viskores/Particle.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/viskores_cont_export.h>

namespace viskores
{
namespace cont
{

/// \brief Copy fields in viskores::Particle to standard types.
///
/// Given an \c ArrayHandle of viskores::Particle, this function copies the
/// position field into an \c ArrayHandle of \c Vec3f objects.
///

template <typename ParticleType>
VISKORES_ALWAYS_EXPORT inline void ParticleArrayCopy(
  const viskores::cont::ArrayHandle<ParticleType, viskores::cont::StorageTagBasic>& inP,
  viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>& outPos,
  bool CopyTerminatedOnly = false);


/// \brief Copy fields in viskores::Particle to standard types.
///
/// Given a std::vector of \c ArrayHandle of viskores::Particle, this function copies the
/// position field into an \c ArrayHandle of \c Vec3f objects.
///
template <typename ParticleType>
VISKORES_ALWAYS_EXPORT inline void ParticleArrayCopy(
  const std::vector<viskores::cont::ArrayHandle<ParticleType, viskores::cont::StorageTagBasic>>&
    inputs,
  viskores::cont::ArrayHandle<viskores::Vec3f, viskores::cont::StorageTagBasic>& outPos);

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
  viskores::cont::ArrayHandle<viskores::FloatDefault, viskores::cont::StorageTagBasic>& outTime);
}
} // namespace viskores::cont

#ifndef viskores_cont_ParticleArrayCopy_hxx
#include <viskores/cont/ParticleArrayCopy.hxx>
#endif //viskores_cont_ParticleArrayCopy_hxx

#endif //viskores_cont_ParticleArrayCopy_h
