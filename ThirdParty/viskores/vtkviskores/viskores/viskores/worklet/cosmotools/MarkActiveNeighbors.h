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

#ifndef viskores_worklet_cosmotools_mark_active_neighbors_h
#define viskores_worklet_cosmotools_mark_active_neighbors_h

#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/cosmotools/TagTypes.h>

namespace viskores
{
namespace worklet
{
namespace cosmotools
{
// Worklet for particles to indicate which neighbors are active
// because at least one particle in that bin is within linking length
template <typename T>
class MarkActiveNeighbors : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature =
    void(FieldIn index,            // (input) particle index
         FieldIn partId,           // (input) particle id sorted
         FieldIn binId,            // (input) bin Id per particle
         WholeArrayIn partIdArray, // (input) sequence imposed on sorted particle Ids
         WholeArrayIn location,    // (input) location of particles
         WholeArrayIn firstPartId, // (input) vector of first particle indices
         WholeArrayIn lastPartId,  // (input) vector of last particle indices
         FieldOut flag);           // (output) active bin neighbors mask
  using ExecutionSignature = _8(_1, _2, _3, _4, _5, _6, _7);
  using InputDomain = _1;

  viskores::Id xNum, yNum, zNum;
  viskores::Id NUM_NEIGHBORS;
  T linkLenSq;

  // Constructor
  VISKORES_EXEC_CONT
  MarkActiveNeighbors(const viskores::Id XNum,
                      const viskores::Id YNum,
                      const viskores::Id ZNum,
                      const viskores::Id NumNeighbors,
                      const T LinkLen)
    : xNum(XNum)
    , yNum(YNum)
    , zNum(ZNum)
    , NUM_NEIGHBORS(NumNeighbors)
    , linkLenSq(LinkLen * LinkLen)
  {
  }

  template <typename InIdPortalType, typename InFieldPortalType, typename InVectorPortalType>
  VISKORES_EXEC viskores::UInt32 operator()(const viskores::Id& i,
                                            const viskores::Id& iPartId,
                                            const viskores::Id& iBinId,
                                            const InIdPortalType& partIdArray,
                                            const InFieldPortalType& location,
                                            const InVectorPortalType& firstPartId,
                                            const InVectorPortalType& lastPartId) const
  {
    const viskores::Id ybin = (iBinId / xNum) % yNum;
    const viskores::Id zbin = iBinId / (xNum * yNum);
    viskores::UInt32 activeFlag = 0;
    viskores::UInt32 bcnt = 1;
    viskores::Id cnt = 0;

    // Examine all neighbor bins surrounding this particle
    for (viskores::Id z = zbin - 1; z <= zbin + 1; z++)
    {
      for (viskores::Id y = ybin - 1; y <= ybin + 1; y++)
      {
        if ((y >= 0) && (y < yNum) && (z >= 0) && (z < zNum))
        {
          viskores::Id pos = NUM_NEIGHBORS * i + cnt;
          viskores::Id startParticle = firstPartId.Get(pos);
          viskores::Id endParticle = lastPartId.Get(pos);

          // If the bin has any particles, check to see if any of those
          // are within the linking length from this particle
          for (viskores::Id j = startParticle; j < endParticle; j++)
          {
            viskores::Id jPartId = partIdArray.Get(j);
            viskores::Vec<T, 3> iloc = location.Get(iPartId);
            viskores::Vec<T, 3> jloc = location.Get(jPartId);
            T xDist = iloc[0] - jloc[0];
            T yDist = iloc[1] - jloc[1];
            T zDist = iloc[2] - jloc[2];

            // Found a particle within linking length so this bin is active
            if ((xDist * xDist + yDist * yDist + zDist * zDist) <= linkLenSq)
            {
              activeFlag = activeFlag | bcnt;
              break;
            }
          }
        }
        bcnt = bcnt << 1;
        cnt++;
      }
    }
    return activeFlag;
  }
}; // MarkActiveNeighbors
}
}
}

#endif
