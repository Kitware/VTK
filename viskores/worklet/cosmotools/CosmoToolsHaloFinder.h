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

#ifndef viskores_worklet_cosmotools_cosmotools_halofinder_h
#define viskores_worklet_cosmotools_cosmotools_halofinder_h

#include <viskores/worklet/cosmotools/CosmoTools.h>

#include <viskores/cont/ArrayGetValues.h>

namespace viskores
{
namespace worklet
{
namespace cosmotools
{

///////////////////////////////////////////////////////////////////////////////////////////
//
// Halo finder for all particles in domain
//
///////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename StorageType>
void CosmoTools<T, StorageType>::HaloFinder(viskores::cont::ArrayHandle<viskores::Id>& resultHaloId,
                                            viskores::cont::ArrayHandle<viskores::Id>& resultMBP,
                                            viskores::cont::ArrayHandle<T>& resultPot)
{
  // Package locations for worklets
  using CompositeLocationType =
    typename viskores::cont::ArrayHandleCompositeVector<LocationType, LocationType, LocationType>;
  CompositeLocationType location;
  location = make_ArrayHandleCompositeVector(xLoc, yLoc, zLoc);

  viskores::cont::ArrayHandle<viskores::Id>
    leftNeighbor; // lower particle id to check for linking length
  viskores::cont::ArrayHandle<viskores::Id>
    rightNeighbor; // upper particle id to check for linking length
  viskores::cont::ArrayHandle<viskores::UInt32>
    activeMask; // mask per particle indicating active neighbor bins
  viskores::cont::ArrayHandle<viskores::Id> partId; // index into all particles
  viskores::cont::ArrayHandle<viskores::Id> binId;  // bin id for each particle in each FOF halo

  leftNeighbor.Allocate(NUM_NEIGHBORS * nParticles);
  rightNeighbor.Allocate(NUM_NEIGHBORS * nParticles);

  viskores::cont::ArrayHandleConstant<bool> trueArray(true, nParticles);
  viskores::cont::ArrayHandleIndex indexArray(nParticles);

  // Bin all particles in domain into bins of size linking length
  BinParticlesAll(partId, binId, leftNeighbor, rightNeighbor);

  // Mark active neighbor bins, meaning at least one particle in the bin
  // is within linking length of the given particle indicated by mask
  MarkActiveNeighbors<T> markActiveNeighbors(numBinsX, numBinsY, numBinsZ, NUM_NEIGHBORS, linkLen);
  viskores::worklet::DispatcherMapField<MarkActiveNeighbors<T>> markActiveNeighborsDispatcher(
    markActiveNeighbors);
  markActiveNeighborsDispatcher.Invoke(
    indexArray,    // (input) index into all particles
    partId,        // (input) particle id sorted by bin
    binId,         // (input) bin id sorted
    partId,        // (input) particle id (whole array)
    location,      // (input) location on original particle order
    leftNeighbor,  // (input) first partId for neighbor vector
    rightNeighbor, // (input) last partId for neighbor vector
    activeMask);   // (output) mask per particle indicating valid neighbors

  // Initialize halo id of each particle to itself
  viskores::cont::ArrayHandle<viskores::Id> haloIdCurrent;
  viskores::cont::ArrayHandle<viskores::Id> haloIdLast;
  DeviceAlgorithm::Copy(indexArray, haloIdCurrent);
  DeviceAlgorithm::Copy(indexArray, haloIdLast);

  // rooted star is nchecked each iteration for all particles being rooted in a halo
  viskores::cont::ArrayHandle<bool> rootedStar;

  // Iterate over particles graft together to form halos
  while (true)
  {
    // Connect each particle to another close particle to build halos
    GraftParticles<T> graftParticles(numBinsX, numBinsY, numBinsZ, NUM_NEIGHBORS, linkLen);
    viskores::worklet::DispatcherMapField<GraftParticles<T>> graftParticlesDispatcher(
      graftParticles);

    graftParticlesDispatcher.Invoke(indexArray,   // (input) index into particles
                                    partId,       // (input) particle id sorted by bin
                                    binId,        // (input) bin id sorted by bin
                                    activeMask,   // (input) flag indicates if neighor range is used
                                    partId,       // (input) particle id (whole array)
                                    location,     // (input) location on original particle order
                                    leftNeighbor, // (input) first partId for neighbor
                                    rightNeighbor,  // (input) last partId for neighbor
                                    haloIdCurrent); // (output)
#ifdef DEBUG_PRINT
    DebugPrint("haloIdCurrent", haloIdCurrent);
#endif

    // Reininitialize rootedStar for each pass
    DeviceAlgorithm::Copy(trueArray, rootedStar);

    // By comparing the haloIds from the last pass and this one
    // determine if any particles are still migrating to halos
    IsStar isStar;
    viskores::worklet::DispatcherMapField<IsStar> isStarDispatcher(isStar);
    isStarDispatcher.Invoke(indexArray,
                            haloIdCurrent, // input (whole array)
                            haloIdLast,    // input (whole array)
                            rootedStar);   // output (whole array)

    // If all vertices are in rooted stars, algorithm is complete
    bool allStars = DeviceAlgorithm::Reduce(rootedStar, true, viskores::LogicalAnd());
    if (allStars)
    {
      break;
    }
    else
    // Otherwise copy current halo ids to last pass halo ids
    {
      PointerJump pointerJump;
      viskores::worklet::DispatcherMapField<PointerJump> pointerJumpDispatcher(pointerJump);
      pointerJumpDispatcher.Invoke(indexArray, haloIdCurrent); // input (whole array)
      DeviceAlgorithm::Copy(haloIdCurrent, haloIdLast);
    }
  }

  // Index into final halo id is the original particle ordering
  // not the particles sorted by bin
  DeviceAlgorithm::Copy(indexArray, partId);
#ifdef DEBUG_PRINT
  DebugPrint("FINAL haloId", haloIdCurrent);
  DebugPrint("FINAL partId", partId);
#endif

  // Call center finding on all halos using method with ReduceByKey and Scatter
  DeviceAlgorithm::Copy(haloIdCurrent, resultHaloId);
  MBPCenterFindingByHalo(partId, resultHaloId, resultMBP, resultPot);
}

///////////////////////////////////////////////////////////////////////////////
//
// Bin all particles in the system for halo finding
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename StorageType>
void CosmoTools<T, StorageType>::BinParticlesAll(
  viskores::cont::ArrayHandle<viskores::Id>& partId,
  viskores::cont::ArrayHandle<viskores::Id>& binId,
  viskores::cont::ArrayHandle<viskores::Id>& leftNeighbor,
  viskores::cont::ArrayHandle<viskores::Id>& rightNeighbor)
{
  // Compute number of bins and ranges for each bin
  viskores::Vec<T, 2> result;
  viskores::Vec<T, 2> xInit(viskores::cont::ArrayGetValue(0, xLoc));
  viskores::Vec<T, 2> yInit(viskores::cont::ArrayGetValue(0, yLoc));
  viskores::Vec<T, 2> zInit(viskores::cont::ArrayGetValue(0, zLoc));
  result = DeviceAlgorithm::Reduce(xLoc, xInit, viskores::MinAndMax<T>());
  T minX = result[0];
  T maxX = result[1];
  result = DeviceAlgorithm::Reduce(yLoc, yInit, viskores::MinAndMax<T>());
  T minY = result[0];
  T maxY = result[1];
  result = DeviceAlgorithm::Reduce(zLoc, zInit, viskores::MinAndMax<T>());
  T minZ = result[0];
  T maxZ = result[1];

  viskores::Id maxBins = 1048576;
  viskores::Id minBins = 1;

  numBinsX = static_cast<viskores::Id>(viskores::Floor((maxX - minX) / linkLen));
  numBinsY = static_cast<viskores::Id>(viskores::Floor((maxY - minY) / linkLen));
  numBinsZ = static_cast<viskores::Id>(viskores::Floor((maxZ - minZ) / linkLen));

  numBinsX = std::min(maxBins, numBinsX);
  numBinsY = std::min(maxBins, numBinsY);
  numBinsZ = std::min(maxBins, numBinsZ);

  numBinsX = std::max(minBins, numBinsX);
  numBinsY = std::max(minBins, numBinsY);
  numBinsZ = std::max(minBins, numBinsZ);

  // Compute which bin each particle is in
  ComputeBins<T> computeBins(minX,
                             maxX, // Physical range on domain
                             minY,
                             maxY,
                             minZ,
                             maxZ,
                             numBinsX,
                             numBinsY,
                             numBinsZ); // Size of superimposed mesh
  viskores::worklet::DispatcherMapField<ComputeBins<T>> computeBinsDispatcher(computeBins);
  computeBinsDispatcher.Invoke(xLoc,   // input
                               yLoc,   // input
                               zLoc,   // input
                               binId); // output

  viskores::cont::ArrayHandleIndex indexArray(nParticles);
  DeviceAlgorithm::Copy(indexArray, partId);

#ifdef DEBUG_PRINT
  std::cout << std::endl
            << "** BinParticlesAll (" << numBinsX << ", " << numBinsY << ", " << numBinsZ << ")"
            << std::endl;
  DebugPrint("xLoc", xLoc);
  DebugPrint("yLoc", yLoc);
  DebugPrint("zLoc", zLoc);
  DebugPrint("partId", partId);
  DebugPrint("binId", binId);
  std::cout << std::endl;
#endif

  // Sort the particles by bin (remember that xLoc and yLoc are not sorted)
  DeviceAlgorithm::SortByKey(binId, partId);
#ifdef DEBUG_PRINT
  DebugPrint("partId", partId);
  DebugPrint("binId", binId);
#endif

  // Compute indices of all left neighbor bins
  viskores::cont::ArrayHandleIndex countArray(nParticles);
  ComputeNeighborBins computeNeighborBins(numBinsX, numBinsY, numBinsZ, NUM_NEIGHBORS);
  viskores::worklet::DispatcherMapField<ComputeNeighborBins> computeNeighborBinsDispatcher(
    computeNeighborBins);
  computeNeighborBinsDispatcher.Invoke(countArray, binId, leftNeighbor);

  // Compute indices of all right neighbor bins
  ComputeBinRange computeBinRange(numBinsX);
  viskores::worklet::DispatcherMapField<ComputeBinRange> computeBinRangeDispatcher(computeBinRange);
  computeBinRangeDispatcher.Invoke(leftNeighbor, rightNeighbor);

  // Convert bin range to particle range within the bins
  DeviceAlgorithm::LowerBounds(binId, leftNeighbor, leftNeighbor);
  DeviceAlgorithm::UpperBounds(binId, rightNeighbor, rightNeighbor);
}

///////////////////////////////////////////////////////////////////////////////
//
// Center finder for all particles given location, particle id and halo id
// MBP (Most Bound Particle) is particle with the minimum potential energy
// Method uses ReduceByKey() and Scatter()
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename StorageType>
void CosmoTools<T, StorageType>::MBPCenterFindingByHalo(
  viskores::cont::ArrayHandle<viskores::Id>& partId,
  viskores::cont::ArrayHandle<viskores::Id>& haloId,
  viskores::cont::ArrayHandle<viskores::Id>& mbpId,
  viskores::cont::ArrayHandle<T>& minPotential)
{
  // Sort particles into groups according to halo id using an index into WholeArrays
  DeviceAlgorithm::SortByKey(haloId, partId);
#ifdef DEBUG_PRINT
  DebugPrint("Sorted haloId", haloId);
  DebugPrint("Sorted partId", partId);
#endif

  // Find the particle in each halo with the lowest potential
  // Compute starting and ending indices of each halo
  viskores::cont::ArrayHandleConstant<viskores::Id> constArray(1, nParticles);
  viskores::cont::ArrayHandleIndex indexArray(nParticles);
  viskores::cont::ArrayHandle<viskores::Id> uniqueHaloIds;
  viskores::cont::ArrayHandle<viskores::Id> particlesPerHalo;
  viskores::cont::ArrayHandle<viskores::Id> minParticle;
  viskores::cont::ArrayHandle<viskores::Id> maxParticle;
  viskores::cont::ArrayHandle<T> potential;
  viskores::cont::ArrayHandle<viskores::Id> tempI;
  viskores::cont::ArrayHandle<T> tempT;

  // Halo ids have been sorted, reduce to find the number of particles per halo
  DeviceAlgorithm::ReduceByKey(
    haloId, constArray, uniqueHaloIds, particlesPerHalo, viskores::Add());
#ifdef DEBUG_PRINT
  DebugPrint("uniqueHaloId", uniqueHaloIds);
  DebugPrint("partPerHalo", particlesPerHalo);
  std::cout << std::endl;
#endif

  // Setup the ScatterCounting worklets needed to expand the ReduceByKeyResults
  viskores::worklet::ScatterCounting scatter(particlesPerHalo);
  viskores::cont::Invoker invoke;

  // Calculate the minimum particle index per halo id and scatter
  DeviceAlgorithm::ScanExclusive(particlesPerHalo, tempI);
  invoke(ScatterWorklet<viskores::Id>{}, scatter, tempI, minParticle);

  // Calculate the maximum particle index per halo id and scatter
  DeviceAlgorithm::ScanInclusive(particlesPerHalo, tempI);
  invoke(ScatterWorklet<viskores::Id>{}, scatter, tempI, maxParticle);

  using IdArrayType = viskores::cont::ArrayHandle<viskores::Id>;
  viskores::cont::ArrayHandleTransform<IdArrayType, ScaleBiasFunctor<viskores::Id>> scaleBias =
    viskores::cont::make_ArrayHandleTransform<IdArrayType>(maxParticle,
                                                           ScaleBiasFunctor<viskores::Id>(1, -1));

  DeviceAlgorithm::Copy(scaleBias, maxParticle);
#ifdef DEBUG_PRINT
  DebugPrint("minParticle", minParticle);
  DebugPrint("maxParticle", maxParticle);
#endif

  // Compute potentials
  ComputePotential<T> computePotential(particleMass);
  viskores::worklet::DispatcherMapField<ComputePotential<T>> computePotentialDispatcher(
    computePotential);

  computePotentialDispatcher.Invoke(indexArray,
                                    partId,      // input (whole array)
                                    xLoc,        // input (whole array)
                                    yLoc,        // input (whole array)
                                    zLoc,        // input (whole array)
                                    minParticle, // input (whole array)
                                    maxParticle, // input (whole array)
                                    potential);  // output

  // Find minimum potential for all particles in a halo and scatter
  DeviceAlgorithm::ReduceByKey(haloId, potential, uniqueHaloIds, tempT, viskores::Minimum());
  invoke(ScatterWorklet<T>{}, scatter, tempT, minPotential);
#ifdef DEBUG_PRINT
  DebugPrint("potential", potential);
  DebugPrint("minPotential", minPotential);
#endif

  // Find the particle id matching the minimum potential (Worklet)
  EqualsMinimumPotential<T> equalsMinimumPotential;
  viskores::worklet::DispatcherMapField<EqualsMinimumPotential<T>> equalsMinimumPotentialDispatcher(
    equalsMinimumPotential);

  equalsMinimumPotentialDispatcher.Invoke(partId, potential, minPotential, mbpId);

  // Fill out entire array with center index, another reduce and scatter
  viskores::cont::ArrayHandle<viskores::Id> minIndx;
  minIndx.Allocate(nParticles);
  DeviceAlgorithm::ReduceByKey(haloId, mbpId, uniqueHaloIds, minIndx, viskores::Maximum());
  invoke(ScatterWorklet<viskores::Id>{}, scatter, minIndx, mbpId);

  // Resort particle ids and mbpId to starting order
  viskores::cont::ArrayHandle<viskores::Id> savePartId;
  DeviceAlgorithm::Copy(partId, savePartId);

  DeviceAlgorithm::SortByKey(partId, haloId);
  DeviceAlgorithm::Copy(savePartId, partId);
  DeviceAlgorithm::SortByKey(partId, mbpId);
  DeviceAlgorithm::Copy(savePartId, partId);
  DeviceAlgorithm::SortByKey(partId, minPotential);

#ifdef DEBUG_PRINT
  std::cout << std::endl;
  DebugPrint("partId", partId);
  DebugPrint("xLoc", xLoc);
  DebugPrint("yLoc", yLoc);
  DebugPrint("haloId", haloId);
  DebugPrint("mbpId", mbpId);
  DebugPrint("minPotential", minPotential);
#endif
}
}
}
}
#endif
