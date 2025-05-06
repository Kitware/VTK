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
//  Copyright (c) 2016, Los Alamos National Security, LLC
//  All rights reserved.
//
//  Copyright 2016. Los Alamos National Security, LLC.
//  This software was produced under U.S. Government contract DE-AC52-06NA25396
//  for Los Alamos National Laboratory (LANL), which is operated by
//  Los Alamos National Security, LLC for the U.S. Department of Energy.
//  The U.S. Government has rights to use, reproduce, and distribute this
//  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC
//  MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE
//  USE OF THIS SOFTWARE.  If software is modified to produce derivative works,
//  such modified software should be clearly marked, so as not to confuse it
//  with the version available from LANL.
//
//  Additionally, redistribution and use in source and binary forms, with or
//  without modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Los Alamos National Security, LLC, Los Alamos
//     National Laboratory, LANL, the U.S. Government, nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND
//  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
//  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS
//  NATIONAL SECURITY, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
//  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

#ifndef viskores_worklet_cosmotools_cosmotools_h
#define viskores_worklet_cosmotools_cosmotools_h

#include <viskores/worklet/cosmotools/ComputeBinIndices.h>
#include <viskores/worklet/cosmotools/ComputeBinRange.h>
#include <viskores/worklet/cosmotools/ComputeBins.h>
#include <viskores/worklet/cosmotools/ComputeNeighborBins.h>
#include <viskores/worklet/cosmotools/GraftParticles.h>
#include <viskores/worklet/cosmotools/IsStar.h>
#include <viskores/worklet/cosmotools/MarkActiveNeighbors.h>
#include <viskores/worklet/cosmotools/PointerJump.h>
#include <viskores/worklet/cosmotools/ValidHalo.h>

#include <viskores/worklet/cosmotools/ComputePotential.h>
#include <viskores/worklet/cosmotools/ComputePotentialBin.h>
#include <viskores/worklet/cosmotools/ComputePotentialMxN.h>
#include <viskores/worklet/cosmotools/ComputePotentialNeighbors.h>
#include <viskores/worklet/cosmotools/ComputePotentialNxN.h>
#include <viskores/worklet/cosmotools/ComputePotentialOnCandidates.h>
#include <viskores/worklet/cosmotools/EqualsMinimumPotential.h>
#include <viskores/worklet/cosmotools/SetCandidateParticles.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleCompositeVector.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandleReverse.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/Invoker.h>
#include <viskores/worklet/ScatterCounting.h>

#include <viskores/BinaryPredicates.h>
#include <viskores/Math.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>

namespace
{

///////////////////////////////////////////////////////////////////////////////
//
// Debug prints
//
///////////////////////////////////////////////////////////////////////////////
template <typename U>
void DebugPrint(const char* msg, viskores::cont::ArrayHandle<U>& array)
{
  viskores::Id count = 20;
  count = std::min(count, array.GetNumberOfValues());
  auto portal = array.ReadPortal();
  std::cout << std::setw(15) << msg << ": ";
  for (viskores::Id i = 0; i < count; i++)
    std::cout << std::setprecision(3) << std::setw(5) << portal.Get(i) << " ";
  std::cout << std::endl;
}

template <typename U>
void DebugPrint(const char* msg,
                viskores::cont::ArrayHandleReverse<viskores::cont::ArrayHandle<U>>& array)
{
  viskores::Id count = 20;
  count = std::min(count, array.GetNumberOfValues());
  auto portal = array.ReadPortal();
  std::cout << std::setw(15) << msg << ": ";
  for (viskores::Id i = 0; i < count; i++)
    std::cout << std::setw(5) << portal.Get(i) << " ";
  std::cout << std::endl;
}
}

namespace viskores
{
namespace worklet
{
namespace cosmotools
{


///////////////////////////////////////////////////////////////////////////////
//
// Scatter the result of a reduced array
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct ScatterWorklet : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inIndices, FieldOut outIndices);
  using ExecutionSignature = void(_1, _2);
  using ScatterType = viskores::worklet::ScatterCounting;

  VISKORES_EXEC
  void operator()(T inputIndex, T& outputIndex) const { outputIndex = inputIndex; }
};

///////////////////////////////////////////////////////////////////////////////
//
// Scale or offset values of an array
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct ScaleBiasFunctor
{
  T Scale;
  T Bias;

  VISKORES_CONT
  ScaleBiasFunctor(T scale = T(1), T bias = T(0))
    : Scale(scale)
    , Bias(bias)
  {
  }

  VISKORES_EXEC_CONT
  T operator()(T value) const { return (Scale * value + Bias); }
};

template <typename T, typename StorageType>
class CosmoTools
{
public:
  using DeviceAlgorithm = viskores::cont::Algorithm;
  const viskores::Id NUM_NEIGHBORS = 9;

  // geometry of domain
  const viskores::Id nParticles;
  const T particleMass;
  const viskores::Id minPartPerHalo;
  const T linkLen;
  viskores::Id numBinsX;
  viskores::Id numBinsY;
  viskores::Id numBinsZ;

  // particle locations within domain
  using LocationType = typename viskores::cont::ArrayHandle<T, StorageType>;
  LocationType& xLoc;
  LocationType& yLoc;
  LocationType& zLoc;

  // cosmo tools constructor for all particles
  CosmoTools(const viskores::Id NParticles,                  // Number of particles
             const T mass,                                   // Particle mass for potential
             const viskores::Id pmin,                        // Minimum particles per halo
             const T bb,                                     // Linking length between particles
             viskores::cont::ArrayHandle<T, StorageType>& X, // Physical location of each particle
             viskores::cont::ArrayHandle<T, StorageType>& Y,
             viskores::cont::ArrayHandle<T, StorageType>& Z);

  // cosmo tools constructor for particles in one halo
  CosmoTools(const viskores::Id NParticles,                  // Number of particles
             const T mass,                                   // Particle mass for potential
             viskores::cont::ArrayHandle<T, StorageType>& X, // Physical location of each particle
             viskores::cont::ArrayHandle<T, StorageType>& Y,
             viskores::cont::ArrayHandle<T, StorageType>& Z);
  ~CosmoTools() {}

  // Halo finding and center finding on halos
  void HaloFinder(viskores::cont::ArrayHandle<viskores::Id>& resultHaloId,
                  viskores::cont::ArrayHandle<viskores::Id>& resultMBP,
                  viskores::cont::ArrayHandle<T>& resultPot);
  void BinParticlesAll(viskores::cont::ArrayHandle<viskores::Id>& partId,
                       viskores::cont::ArrayHandle<viskores::Id>& binId,
                       viskores::cont::ArrayHandle<viskores::Id>& leftNeighbor,
                       viskores::cont::ArrayHandle<viskores::Id>& rightNeighbor);
  void MBPCenterFindingByHalo(viskores::cont::ArrayHandle<viskores::Id>& partId,
                              viskores::cont::ArrayHandle<viskores::Id>& haloId,
                              viskores::cont::ArrayHandle<viskores::Id>& mbpId,
                              viskores::cont::ArrayHandle<T>& minPotential);

  // MBP Center finding on single halo using NxN algorithm
  viskores::Id MBPCenterFinderNxN(T* nxnPotential);

  // MBP Center finding on single halo using MxN estimation
  viskores::Id MBPCenterFinderMxN(T* mxnPotential);

  void BinParticlesHalo(viskores::cont::ArrayHandle<viskores::Id>& partId,
                        viskores::cont::ArrayHandle<viskores::Id>& binId,
                        viskores::cont::ArrayHandle<viskores::Id>& uniqueBins,
                        viskores::cont::ArrayHandle<viskores::Id>& partPerBin,
                        viskores::cont::ArrayHandle<viskores::Id>& particleOffset,
                        viskores::cont::ArrayHandle<viskores::Id>& binX,
                        viskores::cont::ArrayHandle<viskores::Id>& binY,
                        viskores::cont::ArrayHandle<viskores::Id>& binZ);
  void MBPCenterFindingByKey(viskores::cont::ArrayHandle<viskores::Id>& keyId,
                             viskores::cont::ArrayHandle<viskores::Id>& partId,
                             viskores::cont::ArrayHandle<T>& minPotential);
};

///////////////////////////////////////////////////////////////////////////////
//
// Constructor for all particles in the system
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename StorageType>
CosmoTools<T, StorageType>::CosmoTools(const viskores::Id NParticles,
                                       const T mass,
                                       const viskores::Id pmin,
                                       const T bb,
                                       viskores::cont::ArrayHandle<T, StorageType>& X,
                                       viskores::cont::ArrayHandle<T, StorageType>& Y,
                                       viskores::cont::ArrayHandle<T, StorageType>& Z)
  : nParticles(NParticles)
  , particleMass(mass)
  , minPartPerHalo(pmin)
  , linkLen(bb)
  , xLoc(X)
  , yLoc(Y)
  , zLoc(Z)
{
}

///////////////////////////////////////////////////////////////////////////////
//
// Constructor for particles in a single halo
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename StorageType>
CosmoTools<T, StorageType>::CosmoTools(const viskores::Id NParticles,
                                       const T mass,
                                       viskores::cont::ArrayHandle<T, StorageType>& X,
                                       viskores::cont::ArrayHandle<T, StorageType>& Y,
                                       viskores::cont::ArrayHandle<T, StorageType>& Z)
  : nParticles(NParticles)
  , particleMass(mass)
  , minPartPerHalo(10)
  , linkLen(0.2f)
  , xLoc(X)
  , yLoc(Y)
  , zLoc(Z)
{
}
}
}
}
#endif
