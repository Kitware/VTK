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

#ifndef viskores_worklet_cosmotools_compute_potential_neighbors_h
#define viskores_worklet_cosmotools_compute_potential_neighbors_h

#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace cosmotools
{

// Worklet for computing the potential for a particle using 27 neighbor bins
template <typename T>
class ComputePotentialNeighbors : public viskores::worklet::WorkletMapField
{
public:
  using TagType = viskores::List<T>;

  using ControlSignature = void(FieldIn index,                // (input) particle Id
                                WholeArrayIn binId,           // (input) bin id for this particle
                                WholeArrayIn partId,          // (input) first particle in halo
                                WholeArrayIn xLoc,            // (input) x location in domain
                                WholeArrayIn yLoc,            // (input) y location in domain
                                WholeArrayIn zLoc,            // (input) z location in domain
                                WholeArrayIn firstParticleId, // (input) first particle in halo
                                WholeArrayIn lastParticleId,  // (input) last particle in halo
                                FieldOut potential);          // (output) bin ID
  using ExecutionSignature = _9(_1, _2, _3, _4, _5, _6, _7, _8);
  using InputDomain = _1;

  viskores::Id xNum, yNum, zNum;
  viskores::Id NUM_NEIGHBORS;
  T mass;

  // Constructor
  VISKORES_EXEC_CONT
  ComputePotentialNeighbors(const viskores::Id XNum,
                            const viskores::Id YNum,
                            const viskores::Id ZNum,
                            const viskores::Id NumNeighbors,
                            T Mass)
    : xNum(XNum)
    , yNum(YNum)
    , zNum(ZNum)
    , NUM_NEIGHBORS(NumNeighbors)
    , mass(Mass)
  {
  }

  template <typename InFieldPortalType, typename InIdPortalType, typename InVectorPortalType>
  VISKORES_EXEC T operator()(const viskores::Id& i,
                             const InIdPortalType& binId,
                             const InIdPortalType& partId,
                             const InFieldPortalType& xLoc,
                             const InFieldPortalType& yLoc,
                             const InFieldPortalType& zLoc,
                             const InVectorPortalType& firstParticleId,
                             const InVectorPortalType& lastParticleId) const
  {
    viskores::Id iId = partId.Get(i);
    viskores::Id ibin = binId.Get(i);

    const viskores::Id yVal = (ibin / xNum) % yNum;
    const viskores::Id zVal = ibin / (xNum * yNum);
    viskores::Id cnt = 0;
    T potential = 0.0f;

    // Iterate on the 9 sets of neighbor particles
    for (viskores::Id z = zVal - 1; z <= zVal + 1; z++)
    {
      for (viskores::Id y = yVal - 1; y <= yVal + 1; y++)
      {
        viskores::Id firstBinId = NUM_NEIGHBORS * i + cnt;
        viskores::Id startParticle = firstParticleId.Get(firstBinId);
        viskores::Id endParticle = lastParticleId.Get(firstBinId);

        for (viskores::Id j = startParticle; j < endParticle; j++)
        {
          viskores::Id jId = partId.Get(j);
          T xDist = xLoc.Get(iId) - xLoc.Get(jId);
          T yDist = yLoc.Get(iId) - yLoc.Get(jId);
          T zDist = zLoc.Get(iId) - zLoc.Get(jId);
          T r = viskores::Sqrt((xDist * xDist) + (yDist * yDist) + (zDist * zDist));
          if ((i != j) && (fabs(r) > 0.00000000001f))
            potential -= mass / r;
        }
        cnt++;
      }
    }
    return potential;
  }
}; // ComputePotentialNeighbors
}
}
}

#endif
