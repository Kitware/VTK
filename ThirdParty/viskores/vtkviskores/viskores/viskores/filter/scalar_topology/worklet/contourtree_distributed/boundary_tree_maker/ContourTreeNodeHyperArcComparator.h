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

#ifndef viskores_worklet_contourtree_distributed_bract_maker_contourtree_node_hyperarc_comperator_h
#define viskores_worklet_contourtree_distributed_bract_maker_contourtree_node_hyperarc_comperator_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

namespace ctaug = viskores::worklet::contourtree_augmented;

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{
namespace bract_maker
{


// device implementation of the ContourTreeNodeHyperArcComparator
class ContourTreeNodeHyperArcComparatorImpl
{
public:
  using IdArrayPortalType = viskores::cont::ArrayHandle<viskores::Id>::ReadPortalType;

  // constructor - takes vectors as parameters
  VISKORES_CONT
  ContourTreeNodeHyperArcComparatorImpl(const IdArrayPortalType& superarcsPortal,
                                        const IdArrayPortalType& superparentsPortal)
    : SuperarcsPortal(superarcsPortal)
    , SuperparentsPortal(superparentsPortal)
  { // constructor
  } // constructor

  // () operator - gets called to do comparison
  VISKORES_EXEC
  bool operator()(const viskores::Id& i, const viskores::Id& j) const
  { // operator()
    // retrieve the left & right superparents
    viskores::Id leftSuperparent = this->SuperparentsPortal.Get(ctaug::MaskedIndex(i));
    viskores::Id rightSuperparent = this->SuperparentsPortal.Get(ctaug::MaskedIndex(j));
    // compare the superparents
    if (ctaug::MaskedIndex(leftSuperparent) < ctaug::MaskedIndex(rightSuperparent))
    {
      return true;
    }
    if (ctaug::MaskedIndex(leftSuperparent) > ctaug::MaskedIndex(rightSuperparent))
    {
      return false;
    }
    // parents are equal, so retrieve superarc for ascending flag & compare indices
    // TODO We masked the leftSuperparent here. Check if we need to do this or not
    bool ascentFlag =
      ctaug::IsAscending(this->SuperarcsPortal.Get(ctaug::MaskedIndex(leftSuperparent)));

    // compare the sort indices, flipping based on ascending index from superparent
    if (i < j)
    {
      return ascentFlag;
    }
    else if (i > j)
    {
      return !ascentFlag;
    }
    else
    {
      return false;
    }
  } // operator()

private:
  IdArrayPortalType SuperarcsPortal;
  IdArrayPortalType SuperparentsPortal;
}; // ContourTreeNodeHyperArcComparatorImpl

/// comparator to use for sorting nodes by hyperparent (i.e. amalgamates augmentation & sorting)
/// for this one, we don't care (yet) about *WHEN* they were transferred
/// The original version of this took the sortIndex array, but it turns out that it only gets used
/// on an array that already contains sortIndex, and this simplifies code elsewhere, so I have removed it
class ContourTreeNodeHyperArcComparator : public viskores::cont::ExecutionObjectBase
{
public:
  // constructor - takes vectors as parameters
  VISKORES_CONT
  ContourTreeNodeHyperArcComparator(const ctaug::IdArrayType superarcs,
                                    const ctaug::IdArrayType superparents)
    : Superarcs(superarcs)
    , Superparents(superparents)
  { // constructor
  } // constructor

  VISKORES_CONT ContourTreeNodeHyperArcComparatorImpl
  PrepareForExecution(viskores::cont::DeviceAdapterId device, viskores::cont::Token& token) const
  {
    return ContourTreeNodeHyperArcComparatorImpl(this->Superarcs.PrepareForInput(device, token),
                                                 this->Superparents.PrepareForInput(device, token));
  }

private:
  ctaug::IdArrayType Superarcs;
  ctaug::IdArrayType Superparents;
}; // ContourTreeNodeHyperArcComparator

} // namespace bract_maker
} // namespace contourtree_distributes
} // namespace worklet
} // namespace viskores

#endif
