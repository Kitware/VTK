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
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================
//
//  This code is an extension of the algorithm presented in the paper:
//  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.
//
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#ifndef viskores_worklet_contourtree_augmented_process_contourtree_inc_superarc_volumetric_comperator_h
#define viskores_worklet_contourtree_augmented_process_contourtree_inc_superarc_volumetric_comperator_h

#include <viskores/Pair.h>
#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{
namespace process_contourtree_inc
{

class SuperArcVolumetricComparatorImpl
{ // SuperArcVolumetricComparatorImpl
public:
  using IdPortalType = viskores::cont::ArrayHandle<viskores::Id>::ReadPortalType;
  using EdgePairArrayPortalType = EdgePairArray::ReadPortalType;

  IdPortalType weightPortal;
  bool pairsAtLowEnd;
  EdgePairArrayPortalType superarcListPortal;

  // constructor
  SuperArcVolumetricComparatorImpl(const IdArrayType& Weight,
                                   const EdgePairArray& SuperarcList,
                                   bool PairsAtLowEnd,
                                   viskores::cont::DeviceAdapterId device,
                                   viskores::cont::Token& token)
    : pairsAtLowEnd(PairsAtLowEnd)
  { // constructor
    weightPortal = Weight.PrepareForInput(device, token);
    superarcListPortal = SuperarcList.PrepareForInput(device, token);

  } // constructor

  // () operator - gets called to do comparison
  VISKORES_EXEC
  bool operator()(const viskores::Id& i1, const viskores::Id& i2) const
  { // operator()
    // get local references to the edge details
    EdgePair e1 = superarcListPortal.Get(i1);
    EdgePair e2 = superarcListPortal.Get(i2);

    if (pairsAtLowEnd)
    { // pairs at low end
      // test by low end ID
      if (e1.first < e2.first)
        return true;
      if (e1.first > e2.first)
        return false;

      // test by volumetric measure
      if (weightPortal.Get(i1) < weightPortal.Get(i2))
        return true;
      if (weightPortal.Get(i1) > weightPortal.Get(i2))
        return false;

      // test by ID (persistence)
      if (e1.second < e2.second)
        return true;
      if (e1.second > e2.second)
        return false;

      // fallback
      return false;
    } // pairs at low end
    else
    { // pairs at high end
      // test by high end ID
      if (e1.second < e2.second)
        return true;
      if (e1.second > e2.second)
        return false;

      // test by volumetric measure
      if (weightPortal.Get(i1) < weightPortal.Get(i2))
        return true;
      if (weightPortal.Get(i1) > weightPortal.Get(i2))
        return false;

      // test by ID (persistence)
      // Note the reversal from above - we want the greatest difference, not
      // the greatest value
      if (e1.first > e2.first)
        return true;
      if (e1.first < e2.first)
        return false;

      // fallback
      return false;
    } // pairs at high end
  }   // operator()
};    // SuperArcVolumetricComparatorImpl

class SuperArcVolumetricComparator : public viskores::cont::ExecutionObjectBase
{ // SuperArcVolumetricComparator
public:
  // constructor
  SuperArcVolumetricComparator(const IdArrayType& weight,
                               const EdgePairArray& superArcList,
                               bool pairsAtLowEnd)
    : Weight(weight)
    , SuperArcList(superArcList)
    , PairsAtLowEnd(pairsAtLowEnd)
  {
  }

  VISKORES_CONT SuperArcVolumetricComparatorImpl
  PrepareForExecution(viskores::cont::DeviceAdapterId device, viskores::cont::Token& token)
  {
    return SuperArcVolumetricComparatorImpl(
      this->Weight, this->SuperArcList, this->PairsAtLowEnd, device, token);
  }

private:
  IdArrayType Weight;
  EdgePairArray SuperArcList;
  bool PairsAtLowEnd;
}; // SuperArcVolumetricComparator

} // namespace process_contourtree_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
