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

#ifndef viskores_worklet_cosmotools_compute_potential_mxn_h
#define viskores_worklet_cosmotools_compute_potential_mxn_h

#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace cosmotools
{

// Worklet for computing the exact potential for all particles in range vs all particles in system
template <typename T>
class ComputePotentialMxN : public viskores::worklet::WorkletMapField
{
public:
  using TagType = viskores::List<T>;

  using ControlSignature = void(FieldIn index,       // (input) index into particles for one bin
                                WholeArrayIn partId, // (input) original particle id
                                WholeArrayIn xLoc,   // (input) x location in domain
                                WholeArrayIn yLoc,   // (input) y location in domain
                                WholeArrayIn zLoc,   // (input) z location in domain
                                FieldOut potential); // (output) bin ID
  using ExecutionSignature = _6(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  viskores::Id nParticles; // Number of particles in halo
  T mass;                  // Particle mass

  // Constructor
  VISKORES_EXEC_CONT
  ComputePotentialMxN(viskores::Id N, T Mass)
    : nParticles(N)
    , mass(Mass)
  {
  }

  template <typename InIdPortalType, typename InFieldPortalType>
  VISKORES_EXEC T operator()(const viskores::Id& i,
                             const InIdPortalType& partId,
                             const InFieldPortalType& xLoc,
                             const InFieldPortalType& yLoc,
                             const InFieldPortalType& zLoc) const
  {
    T potential = 0.0f;
    viskores::Id iPartId = partId.Get(i);
    for (viskores::Id j = 0; j < nParticles; j++)
    {
      T xDist = xLoc.Get(iPartId) - xLoc.Get(j);
      T yDist = yLoc.Get(iPartId) - yLoc.Get(j);
      T zDist = zLoc.Get(iPartId) - zLoc.Get(j);
      T r = sqrt((xDist * xDist) + (yDist * yDist) + (zDist * zDist));
      if (fabs(r) > 0.00000000001f)
      {
        potential -= mass / r;
      }
    }
    return potential;
  }
}; // ComputePotentialMxN
}
}
}

#endif
