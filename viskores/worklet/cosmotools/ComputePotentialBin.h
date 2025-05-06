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

#ifndef viskores_worklet_cosmotools_compute_potential_bin_h
#define viskores_worklet_cosmotools_compute_potential_bin_h

#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace cosmotools
{

// Worklet for computing the potential for a bin in one halo
template <typename T>
class ComputePotentialBin : public viskores::worklet::WorkletMapField
{
public:
  using TagType = viskores::List<T>;

  using ControlSignature = void(FieldIn binId,         // (input) bin Id
                                WholeArrayIn binCount, // (input) particles per bin
                                WholeArrayIn binX,     // (input) x index in bin
                                WholeArrayIn binY,     // (input) y index in bin
                                WholeArrayIn binZ,     // (input) z index in bin
                                FieldInOut bestPot,    // (output) best potential estimate
                                FieldInOut worstPot);  // (output) worst potential estimate
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7);
  using InputDomain = _1;

  viskores::Id nBins; // Number of bins
  T mass;             // Particle mass
  T linkLen;          // Linking length is side of bin

  // Constructor
  VISKORES_EXEC_CONT
  ComputePotentialBin(viskores::Id N, T Mass, T LinkLen)
    : nBins(N)
    , mass(Mass)
    , linkLen(LinkLen)
  {
  }

  template <typename InIdPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& i,
                                const InIdPortalType& count,
                                const InIdPortalType& binX,
                                const InIdPortalType& binY,
                                const InIdPortalType& binZ,
                                T& bestPotential,
                                T& worstPotential) const
  {
    viskores::Id ibinX = binX.Get(i);
    viskores::Id ibinY = binY.Get(i);
    viskores::Id ibinZ = binZ.Get(i);

    for (viskores::Id j = 0; j < nBins; j++)
    {
      viskores::Id xDelta = viskores::Abs(ibinX - binX.Get(j));
      viskores::Id yDelta = viskores::Abs(ibinY - binY.Get(j));
      viskores::Id zDelta = viskores::Abs(ibinZ - binZ.Get(j));

      if ((count.Get(j) != 0) && (xDelta > 1) && (yDelta > 1) && (zDelta > 1))
      {
        T xDistNear = static_cast<T>((xDelta - 1)) * linkLen;
        T yDistNear = static_cast<T>((yDelta - 1)) * linkLen;
        T zDistNear = static_cast<T>((zDelta - 1)) * linkLen;
        T xDistFar = static_cast<T>((xDelta + 1)) * linkLen;
        T yDistFar = static_cast<T>((yDelta + 1)) * linkLen;
        T zDistFar = static_cast<T>((zDelta + 1)) * linkLen;

        T rNear = viskores::Sqrt((xDistNear * xDistNear) + (yDistNear * yDistNear) +
                                 (zDistNear * zDistNear));
        T rFar =
          viskores::Sqrt((xDistFar * xDistFar) + (yDistFar * yDistFar) + (zDistFar * zDistFar));

        if (rFar > 0.00000000001f)
        {
          worstPotential -= (static_cast<T>(count.Get(j)) * mass) / rFar;
        }
        if (rNear > 0.00000000001f)
        {
          bestPotential -= (static_cast<T>(count.Get(j)) * mass) / rNear;
        }
      }
    }
  }
}; // ComputePotentialBin
}
}
}

#endif
