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

#ifndef viskores_worklet_contourtree_distributed_tree_grafter_supernode_when_comparator_h
#define viskores_worklet_contourtree_distributed_tree_grafter_supernode_when_comparator_h


#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{
namespace tree_grafter
{

/// Comparator used  in TreeGrafter::ListNewSupernodes to sort the NewSupernodes arrays
class SuperNodeWhenComparatorImpl
{
public:
  using IdArrayPortalType = viskores::worklet::contourtree_augmented::IdArrayType::ReadPortalType;

  // Default Constructor
  VISKORES_EXEC_CONT
  SuperNodeWhenComparatorImpl(const IdArrayPortalType& whenTransferredPortal,
                              const IdArrayPortalType& hierarchicalHyperparentPortal,
                              const IdArrayPortalType& hierarchicalHyperIdPortal,
                              const IdArrayPortalType& hierarchicalHyperarcPortal,
                              const IdArrayPortalType& contourTreeSupernodesPortal,
                              const IdArrayPortalType& supernodeTypePortal)
    : WhenTransferredPortal(whenTransferredPortal)
    , HierarchicalHyperparentPortal(hierarchicalHyperparentPortal)
    , HierarchicalHyperIdPortal(hierarchicalHyperIdPortal)
    , HierarchicalHyperarcPortal(hierarchicalHyperarcPortal)
    , ContourTreeSupernodesPortal(contourTreeSupernodesPortal)
    , SupernodeTypePortal(supernodeTypePortal)
  {
  }

  VISKORES_EXEC bool operator()(const viskores::Id& leftSuperId,
                                const viskores::Id& rightSuperId) const
  { // operator ()
    viskores::Id maskedLeftWhen = viskores::worklet::contourtree_augmented::MaskedIndex(
      this->WhenTransferredPortal.Get(leftSuperId));
    viskores::Id maskedRightWhen = viskores::worklet::contourtree_augmented::MaskedIndex(
      this->WhenTransferredPortal.Get(rightSuperId));
    if (maskedLeftWhen < maskedRightWhen)
    {
      return true;
    }
    else if (maskedLeftWhen > maskedRightWhen)
    {
      return false;
    }
    // tie break on hyperparent
    viskores::Id leftHyperparent = this->HierarchicalHyperparentPortal.Get(leftSuperId);
    if (this->SupernodeTypePortal.Get(leftSuperId) !=
        viskores::worklet::contourtree_augmented::IS_ATTACHMENT)
    {
      leftHyperparent = this->HierarchicalHyperIdPortal.Get(leftHyperparent);
    }
    viskores::Id rightHyperparent = this->HierarchicalHyperparentPortal.Get(rightSuperId);
    // if it's an attachment, it's already been converted to the new ID. If not, we need to
    if (this->SupernodeTypePortal.Get(rightSuperId) !=
        viskores::worklet::contourtree_augmented::IS_ATTACHMENT)
    {
      rightHyperparent = this->HierarchicalHyperIdPortal.Get(rightHyperparent);
    }
    if (leftHyperparent < rightHyperparent)
    {
      return true;
    }
    else if (leftHyperparent > rightHyperparent)
    {
      return false;
    }
    // OK, they have the same hyperparent.  But there are two possibilities:
    // A.  they are both attachment points, and the hyperparent is already in the hierarchical tree
    // B.  neither is an attachment point (same hyperparent implies this)
    // In A., we can't look up ascent / descent, so we will sort only on sort index - ie always ascending
    // In B., we look up the hyperarc and use it to bias the sort
    bool sortAscending = true;
    // if it's not an attachment, pull the hyperparent in old supernode IDs and use to retrieve hyperarc, then take flag
    if (this->SupernodeTypePortal.Get(leftSuperId) !=
        viskores::worklet::contourtree_augmented::IS_ATTACHMENT)
    {
      sortAscending = viskores::worklet::contourtree_augmented::IsAscending(
        this->HierarchicalHyperarcPortal.Get(HierarchicalHyperparentPortal.Get(leftSuperId)));
    }

    // if they have the same hyperparent and the hyperarc is ascending, use usual test. If not, invert
    if (sortAscending)
    {
      return (this->ContourTreeSupernodesPortal.Get(leftSuperId) <
              this->ContourTreeSupernodesPortal.Get(rightSuperId));
    }
    else
    {
      return (this->ContourTreeSupernodesPortal.Get(leftSuperId) >
              ContourTreeSupernodesPortal.Get(rightSuperId));
    }
  } // operator ()

private:
  IdArrayPortalType WhenTransferredPortal;
  IdArrayPortalType HierarchicalHyperparentPortal;
  IdArrayPortalType HierarchicalHyperIdPortal;
  IdArrayPortalType HierarchicalHyperarcPortal;
  IdArrayPortalType ContourTreeSupernodesPortal;
  IdArrayPortalType SupernodeTypePortal;
}; // SuperNodeWhenComparator


class SuperNodeWhenComparator : public viskores::cont::ExecutionObjectBase
{
public:
  VISKORES_CONT
  SuperNodeWhenComparator(
    const viskores::worklet::contourtree_augmented::IdArrayType& whenTransferred,
    const viskores::worklet::contourtree_augmented::IdArrayType& hierarchicalHyperparent,
    const viskores::worklet::contourtree_augmented::IdArrayType& hierarchicalHyperId,
    const viskores::worklet::contourtree_augmented::IdArrayType& hierarchicalHyperarc,
    const viskores::worklet::contourtree_augmented::IdArrayType& contourTreeSupernodes,
    const viskores::worklet::contourtree_augmented::IdArrayType& supernodeType)
    : WhenTransferred(whenTransferred)
    , HierarchicalHyperparent(hierarchicalHyperparent)
    , HierarchicalHyperId(hierarchicalHyperId)
    , HierarchicalHyperarc(hierarchicalHyperarc)
    , ContourTreeSupernodes(contourTreeSupernodes)
    , SupernodeType(supernodeType)
  {
  }

  VISKORES_CONT SuperNodeWhenComparatorImpl
  PrepareForExecution(viskores::cont::DeviceAdapterId device, viskores::cont::Token& token) const
  {
    return SuperNodeWhenComparatorImpl(this->WhenTransferred.PrepareForInput(device, token),
                                       this->HierarchicalHyperparent.PrepareForInput(device, token),
                                       this->HierarchicalHyperId.PrepareForInput(device, token),
                                       this->HierarchicalHyperarc.PrepareForInput(device, token),
                                       this->ContourTreeSupernodes.PrepareForInput(device, token),
                                       this->SupernodeType.PrepareForInput(device, token));
  }

private:
  const viskores::worklet::contourtree_augmented::IdArrayType& WhenTransferred;
  const viskores::worklet::contourtree_augmented::IdArrayType& HierarchicalHyperparent;
  const viskores::worklet::contourtree_augmented::IdArrayType& HierarchicalHyperId;
  const viskores::worklet::contourtree_augmented::IdArrayType& HierarchicalHyperarc;
  const viskores::worklet::contourtree_augmented::IdArrayType& ContourTreeSupernodes;
  const viskores::worklet::contourtree_augmented::IdArrayType& SupernodeType;
};

} // namespace tree_grafter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
