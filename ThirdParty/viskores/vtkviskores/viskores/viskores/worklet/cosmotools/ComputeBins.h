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

#ifndef viskores_worklet_cosmotools_compute_bins__h
#define viskores_worklet_cosmotools_compute_bins__h

#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace cosmotools
{

// Worklet for computing the bin id for every particle in domain
template <typename T>
class ComputeBins : public viskores::worklet::WorkletMapField
{
public:
  using TagType = viskores::List<T>;

  using ControlSignature = void(FieldIn xLoc,    // (input) x location in halo
                                FieldIn yLoc,    // (input) y location in halo
                                FieldIn zLoc,    // (input) z location in halo
                                FieldOut binId); // (output) bin Id
  using ExecutionSignature = _4(_1, _2, _3);
  using InputDomain = _1;

  T xMin, xMax, yMin, yMax, zMin, zMax;
  viskores::Id xNum, yNum, zNum;

  // Constructor
  VISKORES_EXEC_CONT
  ComputeBins(T XMin,
              T XMax,
              T YMin,
              T YMax,
              T ZMin,
              T ZMax,
              viskores::Id XNum,
              viskores::Id YNum,
              viskores::Id ZNum)
    : xMin(XMin)
    , xMax(XMax)
    , yMin(YMin)
    , yMax(YMax)
    , zMin(ZMin)
    , zMax(ZMax)
    , xNum(XNum)
    , yNum(YNum)
    , zNum(ZNum)
  {
  }

  VISKORES_EXEC
  viskores::Id operator()(const T& xLoc, const T& yLoc, const T& zLoc) const
  {
    viskores::Id xbin = 0;
    if (xNum > 1)
    {
      xbin = static_cast<viskores::Id>((static_cast<T>(xNum) * (xLoc - xMin)) / (xMax - xMin));
      if (xbin >= xNum)
        xbin = xNum - 1;
    }
    viskores::Id ybin = 0;
    if (yNum > 1)
    {
      ybin = static_cast<viskores::Id>((static_cast<T>(yNum) * (yLoc - yMin)) / (yMax - yMin));
      if (ybin >= yNum)
        ybin = yNum - 1;
    }
    viskores::Id zbin = 0;
    if (zNum > 1)
    {
      zbin = static_cast<viskores::Id>((static_cast<T>(zNum) * (zLoc - zMin)) / (zMax - zMin));
      if (zbin >= zNum)
        zbin = zNum - 1;
    }
    return (xbin + ybin * xNum + zbin * xNum * yNum);
  }
}; // ComputeBins
}
}
}

#endif
