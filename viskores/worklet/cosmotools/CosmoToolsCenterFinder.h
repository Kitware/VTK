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

#ifndef viskores_worklet_cosmotools_cosmotools_centerfinder_h
#define viskores_worklet_cosmotools_cosmotools_centerfinder_h

#include <viskores/worklet/cosmotools/CosmoTools.h>

#include <viskores/cont/ArrayGetValues.h>

namespace viskores
{
namespace worklet
{
namespace cosmotools
{

///////////////////////////////////////////////////////////////////////////////
//
// Center finder for particles in FOF halo using estimations but with exact final answer
// MBP (Most Bound Particle) is particle with the minimum potential energy
//
///////////////////////////////////////////////////////////////////////////////

template <typename T, typename StorageType>
viskores::Id CosmoTools<T, StorageType>::MBPCenterFinderMxN(T* mxnPotential)
{
  viskores::cont::ArrayHandle<viskores::Id> partId;
  viskores::cont::ArrayHandle<viskores::Id> binId;

  viskores::cont::ArrayHandle<viskores::Id> uniqueBins;
  viskores::cont::ArrayHandle<viskores::Id> partPerBin;
  viskores::cont::ArrayHandle<viskores::Id> particleOffset;

  viskores::cont::ArrayHandle<viskores::Id> binX;
  viskores::cont::ArrayHandle<viskores::Id> binY;
  viskores::cont::ArrayHandle<viskores::Id> binZ;

  // Bin all particles in the halo into bins of size linking length
  BinParticlesHalo(partId, binId, uniqueBins, partPerBin, particleOffset, binX, binY, binZ);
#ifdef DEBUG_PRINT
  DebugPrint("uniqueBins", uniqueBins);
  DebugPrint("partPerBin", partPerBin);
#endif

  // Compute the estimated potential per bin using 27 contiguous bins
  viskores::cont::ArrayHandle<T> partPotential;
  MBPCenterFindingByKey(binId, partId, partPotential);

  // Reduce by key to get the estimated minimum potential per bin within 27 neighbors
  viskores::cont::ArrayHandle<viskores::Id> tempId;
  viskores::cont::ArrayHandle<T> minPotential;
  DeviceAlgorithm::ReduceByKey(binId, partPotential, tempId, minPotential, viskores::Minimum());

  // Reduce by key to get the estimated maximum potential per bin within 27 neighbors
  viskores::cont::ArrayHandle<T> maxPotential;
  DeviceAlgorithm::ReduceByKey(binId, partPotential, tempId, maxPotential, viskores::Maximum());
#ifdef DEBUG_PRINT
  DebugPrint("minPotential", minPotential);
  DebugPrint("maxPotential", maxPotential);
#endif

  // Compute potentials estimate for a bin using all other bins
  // Particles in the other bins are located at the closest point to this bin
  viskores::cont::ArrayHandleIndex uniqueIndex(uniqueBins.GetNumberOfValues());
  viskores::cont::ArrayHandle<T> bestEstPotential;
  viskores::cont::ArrayHandle<T> worstEstPotential;

  // Initialize each bin potential with the nxn for that bin
  DeviceAlgorithm::Copy(minPotential, bestEstPotential);
  DeviceAlgorithm::Copy(maxPotential, worstEstPotential);

  // Estimate only across the uniqueBins that contain particles
  ComputePotentialBin<T> computePotentialBin(uniqueBins.GetNumberOfValues(), particleMass, linkLen);
  viskores::worklet::DispatcherMapField<ComputePotentialBin<T>> computePotentialBinDispatcher(
    computePotentialBin);

  computePotentialBinDispatcher.Invoke(uniqueIndex,        // input
                                       partPerBin,         // input (whole array)
                                       binX,               // input (whole array)
                                       binY,               // input (whole array)
                                       binZ,               // input (whole array)
                                       bestEstPotential,   // input/output
                                       worstEstPotential); // input/output
#ifdef DEBUG_PRINT
  DebugPrint("bestEstPotential", bestEstPotential);
  DebugPrint("worstEstPotential", worstEstPotential);
  std::cout << "Number of bestEstPotential " << bestEstPotential.GetNumberOfValues() << std::endl;
  std::cout << "Number of worstEstPotential " << worstEstPotential.GetNumberOfValues() << std::endl;
#endif

  // Sort everything by the best estimated potential per bin
  viskores::cont::ArrayHandle<T> tempBest;
  DeviceAlgorithm::Copy(bestEstPotential, tempBest);
  DeviceAlgorithm::SortByKey(tempBest, worstEstPotential);

  // Use the worst estimate for the first selected bin to compare to best of all others
  // Any bin that passes is a candidate for having the MBP
  T cutoffPotential = viskores::cont::ArrayGetValue(0, worstEstPotential);
  worstEstPotential.ReleaseResources();
  tempBest.ReleaseResources();

  viskores::cont::ArrayHandle<viskores::Id> candidate;
  DeviceAlgorithm::Copy(viskores::cont::ArrayHandleConstant<viskores::Id>(0, nParticles),
                        candidate);

  SetCandidateParticles<T> setCandidateParticles(cutoffPotential);
  viskores::worklet::DispatcherMapField<SetCandidateParticles<T>> setCandidateParticlesDispatcher(
    setCandidateParticles);
  setCandidateParticlesDispatcher.Invoke(bestEstPotential, // input
                                         particleOffset,   // input
                                         partPerBin,       // input
                                         candidate);       // output (whole array)

  // Copy the M candidate particles to a new array
  viskores::cont::ArrayHandle<viskores::Id> mparticles;
  DeviceAlgorithm::CopyIf(partId, candidate, mparticles);

  // Compute potentials only on the candidate particles
  viskores::cont::ArrayHandle<T> mpotential;
  ComputePotentialOnCandidates<T> computePotentialOnCandidates(nParticles, particleMass);
  viskores::worklet::DispatcherMapField<ComputePotentialOnCandidates<T>>
    computePotentialOnCandidatesDispatcher(computePotentialOnCandidates);

  computePotentialOnCandidatesDispatcher.Invoke(mparticles,
                                                xLoc,        // input (whole array)
                                                yLoc,        // input (whole array)
                                                zLoc,        // input (whole array)
                                                mpotential); // output

  // Of the M candidate particles which has the minimum potential
  DeviceAlgorithm::SortByKey(mpotential, mparticles);
#ifdef DEBUG_PRINT
  DebugPrint("mparticles", mparticles);
  DebugPrint("mpotential", mpotential);
#endif

  // Return the found MBP particle and its potential
  viskores::Id mxnMBP = viskores::cont::ArrayGetValue(0, mparticles);
  *mxnPotential = viskores::cont::ArrayGetValue(0, mpotential);

  return mxnMBP;
}

///////////////////////////////////////////////////////////////////////////////
//
// Bin particles in one halo for quick MBP finding
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename StorageType>
void CosmoTools<T, StorageType>::BinParticlesHalo(
  viskores::cont::ArrayHandle<viskores::Id>& partId,
  viskores::cont::ArrayHandle<viskores::Id>& binId,
  viskores::cont::ArrayHandle<viskores::Id>& uniqueBins,
  viskores::cont::ArrayHandle<viskores::Id>& partPerBin,
  viskores::cont::ArrayHandle<viskores::Id>& particleOffset,
  viskores::cont::ArrayHandle<viskores::Id>& binX,
  viskores::cont::ArrayHandle<viskores::Id>& binY,
  viskores::cont::ArrayHandle<viskores::Id>& binZ)
{
  // Compute number of bins and ranges for each bin
  viskores::Vec<T, 2> xRange(viskores::cont::ArrayGetValue(0, xLoc));
  viskores::Vec<T, 2> yRange(viskores::cont::ArrayGetValue(0, yLoc));
  viskores::Vec<T, 2> zRange(viskores::cont::ArrayGetValue(0, zLoc));
  xRange = DeviceAlgorithm::Reduce(xLoc, xRange, viskores::MinAndMax<T>());
  T minX = xRange[0];
  T maxX = xRange[1];
  yRange = DeviceAlgorithm::Reduce(yLoc, yRange, viskores::MinAndMax<T>());
  T minY = yRange[0];
  T maxY = yRange[1];
  zRange = DeviceAlgorithm::Reduce(zLoc, zRange, viskores::MinAndMax<T>());
  T minZ = zRange[0];
  T maxZ = zRange[1];

  numBinsX = static_cast<viskores::Id>(viskores::Floor((maxX - minX) / linkLen));
  numBinsY = static_cast<viskores::Id>(viskores::Floor((maxY - minY) / linkLen));
  numBinsZ = static_cast<viskores::Id>(viskores::Floor((maxZ - minZ) / linkLen));

  viskores::Id maxBins = 1048576;
  numBinsX = std::min(maxBins, numBinsX);
  numBinsY = std::min(maxBins, numBinsY);
  numBinsZ = std::min(maxBins, numBinsZ);

  viskores::Id minBins = 1;
  numBinsX = std::max(minBins, numBinsX);
  numBinsY = std::max(minBins, numBinsY);
  numBinsZ = std::max(minBins, numBinsZ);

#ifdef DEBUG_PRINT
  std::cout << std::endl
            << "** BinParticlesHalo (" << numBinsX << ", " << numBinsY << ", " << numBinsZ << ") ("
            << minX << ", " << minY << ", " << minZ << ") (" << maxX << ", " << maxY << ", " << maxZ
            << ")" << std::endl;
#endif

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
  DebugPrint("xLoc", xLoc);
  DebugPrint("yLoc", yLoc);
  DebugPrint("zLoc", zLoc);
  DebugPrint("partId", partId);
  DebugPrint("binId", binId);
#endif

  // Sort the particles by bin
  DeviceAlgorithm::SortByKey(binId, partId);

  // Count the number of particles per bin
  viskores::cont::ArrayHandleConstant<viskores::Id> constArray(1, nParticles);
  DeviceAlgorithm::ReduceByKey(binId, constArray, uniqueBins, partPerBin, viskores::Add());
#ifdef DEBUG_PRINT
  DebugPrint("sorted binId", binId);
  DebugPrint("sorted partId", partId);
  DebugPrint("uniqueBins", uniqueBins);
  DebugPrint("partPerBin", partPerBin);
#endif

  // Calculate the bin indices
  ComputeBinIndices<T> computeBinIndices(numBinsX, numBinsY, numBinsZ);
  viskores::worklet::DispatcherMapField<ComputeBinIndices<T>> computeBinIndicesDispatcher(
    computeBinIndices);

  computeBinIndicesDispatcher.Invoke(uniqueBins, // input
                                     binX,       // input
                                     binY,       // input
                                     binZ);      // input

  DeviceAlgorithm::ScanExclusive(partPerBin, particleOffset);
}

///////////////////////////////////////////////////////////////////////////////
//
// Center finder for all particles given location, particle id and key id
// Assumed that key and particles are already sorted
// MBP (Most Bound Particle) is particle with the minimum potential energy
// Method uses ScanInclusiveByKey() and ArrayHandleReverse
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename StorageType>
void CosmoTools<T, StorageType>::MBPCenterFindingByKey(
  viskores::cont::ArrayHandle<viskores::Id>& keyId,
  viskores::cont::ArrayHandle<viskores::Id>& partId,
  viskores::cont::ArrayHandle<T>& minPotential)
{
  // Compute starting and ending indices of each key (bin or halo)
  viskores::cont::ArrayHandleIndex indexArray(nParticles);
  viskores::cont::ArrayHandle<T> potential;

  viskores::cont::ArrayHandleReverse<viskores::cont::ArrayHandle<viskores::Id>> keyReverse(keyId);
  viskores::cont::ArrayHandleReverse<viskores::cont::ArrayHandle<T>> minPotReverse(minPotential);

  // Compute indices of all left neighbor bins per bin not per particle
  viskores::cont::ArrayHandle<viskores::Id> leftNeighbor;
  viskores::cont::ArrayHandle<viskores::Id> rightNeighbor;
  leftNeighbor.Allocate(NUM_NEIGHBORS * nParticles);
  rightNeighbor.Allocate(NUM_NEIGHBORS * nParticles);

  viskores::cont::ArrayHandleIndex countArray(nParticles);
  ComputeNeighborBins computeNeighborBins(numBinsX, numBinsY, numBinsZ, NUM_NEIGHBORS);
  viskores::worklet::DispatcherMapField<ComputeNeighborBins> computeNeighborBinsDispatcher(
    computeNeighborBins);
  computeNeighborBinsDispatcher.Invoke(countArray, keyId, leftNeighbor);

  // Compute indices of all right neighbor bins
  ComputeBinRange computeBinRange(numBinsX);
  viskores::worklet::DispatcherMapField<ComputeBinRange> computeBinRangeDispatcher(computeBinRange);
  computeBinRangeDispatcher.Invoke(leftNeighbor, rightNeighbor);

  // Convert bin range to particle range within the bins
  DeviceAlgorithm::LowerBounds(keyId, leftNeighbor, leftNeighbor);
  DeviceAlgorithm::UpperBounds(keyId, rightNeighbor, rightNeighbor);
#ifdef DEBUG_PRINT
  DebugPrint("leftNeighbor", leftNeighbor);
  DebugPrint("rightNeighbor", rightNeighbor);
#endif

  // Initialize halo id of each particle to itself
  // Compute potentials on particles in 27 neighbors to find minimum
  ComputePotentialNeighbors<T> computePotentialNeighbors(
    numBinsX, numBinsY, numBinsZ, NUM_NEIGHBORS, particleMass);
  viskores::worklet::DispatcherMapField<ComputePotentialNeighbors<T>>
    computePotentialNeighborsDispatcher(computePotentialNeighbors);

  computePotentialNeighborsDispatcher.Invoke(indexArray,
                                             keyId,         // input (whole array)
                                             partId,        // input (whole array)
                                             xLoc,          // input (whole array)
                                             yLoc,          // input (whole array)
                                             zLoc,          // input (whole array)
                                             leftNeighbor,  // input (whole array)
                                             rightNeighbor, // input (whole array)
                                             potential);    // output

  // Find minimum potential for all particles in a halo
  DeviceAlgorithm::ScanInclusiveByKey(keyId, potential, minPotential, viskores::Minimum());
  DeviceAlgorithm::ScanInclusiveByKey(
    keyReverse, minPotReverse, minPotReverse, viskores::Minimum());
#ifdef DEBUG_PRINT
  DebugPrint("potential", potential);
  DebugPrint("minPotential", minPotential);
#endif

  // Find the particle id matching the minimum potential
  viskores::cont::ArrayHandle<viskores::Id> centerId;
  EqualsMinimumPotential<T> equalsMinimumPotential;
  viskores::worklet::DispatcherMapField<EqualsMinimumPotential<T>> equalsMinimumPotentialDispatcher(
    equalsMinimumPotential);

  equalsMinimumPotentialDispatcher.Invoke(partId, potential, minPotential, centerId);
}

///////////////////////////////////////////////////////////////////////////////
//
// Center finder for particles in a single halo given location and particle id
// MBP (Most Bound Particle) is particle with the minimum potential energy
// Method uses ScanInclusiveByKey() and ArrayHandleReverse
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename StorageType>
viskores::Id CosmoTools<T, StorageType>::MBPCenterFinderNxN(T* nxnPotential)
{
  viskores::cont::ArrayHandle<T> potential;
  viskores::cont::ArrayHandle<T> minPotential;

  viskores::cont::ArrayHandleReverse<viskores::cont::ArrayHandle<T>> minPotReverse(minPotential);

  viskores::cont::ArrayHandleIndex particleIndex(nParticles);

  // Compute potentials (Worklet)
  ComputePotentialNxN<T> computePotentialHalo(nParticles, particleMass);
  viskores::worklet::DispatcherMapField<ComputePotentialNxN<T>> computePotentialHaloDispatcher(
    computePotentialHalo);

  computePotentialHaloDispatcher.Invoke(particleIndex, // input
                                        xLoc,          // input (whole array)
                                        yLoc,          // input (whole array)
                                        zLoc,          // input (whole array)
                                        potential);    // output

  // Find minimum potential for all particles in a halo
  DeviceAlgorithm::ScanInclusive(potential, minPotential, viskores::Minimum());
  DeviceAlgorithm::ScanInclusive(minPotReverse, minPotReverse, viskores::Minimum());

  // Find the particle id matching the minimum potential
  viskores::cont::ArrayHandle<viskores::Id> centerId;
  EqualsMinimumPotential<T> equalsMinimumPotential;
  viskores::worklet::DispatcherMapField<EqualsMinimumPotential<T>> equalsMinimumPotentialDispatcher(
    equalsMinimumPotential);

  equalsMinimumPotentialDispatcher.Invoke(particleIndex, potential, minPotential, centerId);

  // Fill out entire array with center index
  viskores::cont::ArrayHandleReverse<viskores::cont::ArrayHandle<viskores::Id>> centerIdReverse(
    centerId);
  DeviceAlgorithm::ScanInclusive(centerId, centerId, viskores::Maximum());
  DeviceAlgorithm::ScanInclusive(centerIdReverse, centerIdReverse, viskores::Maximum());

  viskores::Id nxnMBP = viskores::cont::ArrayGetValue(0, centerId);
  *nxnPotential = viskores::cont::ArrayGetValue(nxnMBP, potential);

  return nxnMBP;
}
}
}
}
#endif
