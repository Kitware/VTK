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

#ifndef viskores_worklet_contourtree_augmented_contourtree_maker_inc_contour_tree_supernode_comparator_h
#define viskores_worklet_contourtree_augmented_contourtree_maker_inc_contour_tree_supernode_comparator_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{
namespace contourtree_maker_inc
{


// comparator used for initial sort of data values
class ContourTreeSuperNodeComparatorImpl
{
public:
  using IdPortalType = viskores::cont::ArrayHandle<viskores::Id>::ReadPortalType;

  IdPortalType HyperparentsPortal;
  IdPortalType SupernodesPortal;
  IdPortalType WhenTransferredPortal;

  // constructor
  VISKORES_CONT
  ContourTreeSuperNodeComparatorImpl(const IdArrayType& hyperparents,
                                     const IdArrayType& supernodes,
                                     const IdArrayType& whenTransferred,
                                     viskores::cont::DeviceAdapterId device,
                                     viskores::cont::Token& token)
  {
    this->HyperparentsPortal = hyperparents.PrepareForInput(device, token);
    this->SupernodesPortal = supernodes.PrepareForInput(device, token);
    this->WhenTransferredPortal = whenTransferred.PrepareForInput(device, token);
  }

  // () operator - gets called to do comparison
  VISKORES_EXEC
  bool operator()(const viskores::Id& leftComparand, const viskores::Id& rightComparand) const
  { // operator()
    // first compare the iteration when they were transferred
    viskores::Id leftWhen = MaskedIndex(WhenTransferredPortal.Get(leftComparand));
    viskores::Id rightWhen = MaskedIndex(WhenTransferredPortal.Get(rightComparand));
    if (leftWhen < rightWhen)
      return true;
    if (leftWhen > rightWhen)
      return false;

    // next compare the left & right hyperparents
    viskores::Id leftHyperparent = this->HyperparentsPortal.Get(MaskedIndex(leftComparand));
    viskores::Id rightHyperparent = this->HyperparentsPortal.Get(MaskedIndex(rightComparand));
    if (MaskedIndex(leftHyperparent) < MaskedIndex(rightHyperparent))
      return true;
    if (MaskedIndex(leftHyperparent) > MaskedIndex(rightHyperparent))
      return false;

    // the parents are equal, so we compare the nodes, which are sort indices and thus indicate value
    // we will flip for ascending edges
    viskores::Id leftSupernode = SupernodesPortal.Get(leftComparand);
    viskores::Id rightSupernode = SupernodesPortal.Get(rightComparand);
    if (leftSupernode < rightSupernode)
      return IsAscending(leftHyperparent);
    else if (leftSupernode > rightSupernode)
      return !IsAscending(leftHyperparent);
    else
      return false;
  } // operator()
};  // ContourTreeSuperNodeComparator

class ContourTreeSuperNodeComparator : public viskores::cont::ExecutionObjectBase
{
public:
  // constructor
  VISKORES_CONT
  ContourTreeSuperNodeComparator(const IdArrayType& hyperparents,
                                 const IdArrayType& supernodes,
                                 const IdArrayType& whenTransferred)
    : Hyperparents(hyperparents)
    , Supernodes(supernodes)
    , WhenTransferred(whenTransferred)
  {
  }

  VISKORES_CONT ContourTreeSuperNodeComparatorImpl
  PrepareForExecution(viskores::cont::DeviceAdapterId device, viskores::cont::Token& token)
  {
    return ContourTreeSuperNodeComparatorImpl(
      this->Hyperparents, this->Supernodes, this->WhenTransferred, device, token);
  }

private:
  IdArrayType Hyperparents;
  IdArrayType Supernodes;
  IdArrayType WhenTransferred;
};

} // namespace contourtree_maker_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
