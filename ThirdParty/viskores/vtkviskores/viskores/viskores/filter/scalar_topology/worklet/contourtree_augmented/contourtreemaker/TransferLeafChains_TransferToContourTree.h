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

#ifndef viskores_worklet_contourtree_augmented_contourtree_maker_inc_transfer_leaf_chains_transfer_to_contour_tree_h
#define viskores_worklet_contourtree_augmented_contourtree_maker_inc_transfer_leaf_chains_transfer_to_contour_tree_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{
namespace contourtree_maker_inc
{

// Worklet to transfer leaf chains to contour tree"
// a. for leaves (tested by degree),
//              i.      we use inbound as the hyperarc
//              ii.     we use inwards as the superarc
//              iii.we use self as the hyperparent
// b. for regular vertices pointing to a leaf (test by outbound's degree),
//              i.      we use outbound as the hyperparent
//              ii. we use inwards as the superarc
// c. for all other vertics
//              ignore
class TransferLeafChains_TransferToContourTree : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn activeSupernodes,                // (input)
                                WholeArrayOut contourTreeHyperparents,   // (output)
                                WholeArrayOut contourTreeHyperarcs,      // (output)
                                WholeArrayOut contourTreeSuperarcs,      // (output)
                                WholeArrayOut contourTreeWhenTransferred // (output)
  );

  typedef void ExecutionSignature(_1, InputIndex, _2, _3, _4, _5);
  using InputDomain = _1;

  // viskores only allows 9 parameters for the operator so we need to do these inputs manually via the constructor
  using IdPortalType = viskores::cont::ArrayHandle<viskores::Id>::ReadPortalType;
  IdPortalType OutdegreePortal;
  IdPortalType IndegreePortal;
  IdPortalType OutboundPortal;
  IdPortalType InboundPortal;
  IdPortalType InwardsPortal;
  viskores::Id NumIterations;
  bool isJoin;


  // Default Constructor
  TransferLeafChains_TransferToContourTree(const viskores::Id nIterations,
                                           const bool IsJoin,
                                           const IdArrayType& outdegree,
                                           const IdArrayType& indegree,
                                           const IdArrayType& outbound,
                                           const IdArrayType& inbound,
                                           const IdArrayType& inwards,
                                           viskores::cont::DeviceAdapterId device,
                                           viskores::cont::Token& token)
    : NumIterations(nIterations)
    , isJoin(IsJoin)
  {
    this->OutdegreePortal = outdegree.PrepareForInput(device, token);
    this->IndegreePortal = indegree.PrepareForInput(device, token);
    this->OutboundPortal = outbound.PrepareForInput(device, token);
    this->InboundPortal = inbound.PrepareForInput(device, token);
    this->InwardsPortal = inwards.PrepareForInput(device, token);
  }


  template <typename OutFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& superID,
                                const viskores::Id /*activeID*/, // FIXME: Remove unused parameter?
                                const OutFieldPortalType& contourTreeHyperparentsPortal,
                                const OutFieldPortalType& contourTreeHyperarcsPortal,
                                const OutFieldPortalType& contourTreeSuperarcsPortal,
                                const OutFieldPortalType& contourTreeWhenTransferredPortal) const
  {
    if ((this->OutdegreePortal.Get(superID) == 0) && (this->IndegreePortal.Get(superID) == 1))
    { // a leaf
      contourTreeHyperparentsPortal.Set(superID, superID | (this->isJoin ? 0 : IS_ASCENDING));
      contourTreeHyperarcsPortal.Set(
        superID, MaskedIndex(this->InboundPortal.Get(superID)) | (this->isJoin ? 0 : IS_ASCENDING));
      contourTreeSuperarcsPortal.Set(
        superID, MaskedIndex(this->InwardsPortal.Get(superID)) | (this->isJoin ? 0 : IS_ASCENDING));
      contourTreeWhenTransferredPortal.Set(superID, this->NumIterations | IS_HYPERNODE);
    } // a leaf
    else
    { // not a leaf
      // retrieve the out neighbour
      viskores::Id outNeighbour = MaskedIndex(this->OutboundPortal.Get(superID));

      // test whether outneighbour is a leaf
      if ((this->OutdegreePortal.Get(outNeighbour) != 0) ||
          (this->IndegreePortal.Get(outNeighbour) != 1))
      {
      }
      else
      {
        // set superarc, &c.
        contourTreeSuperarcsPortal.Set(superID,
                                       MaskedIndex(this->InwardsPortal.Get(superID)) |
                                         (this->isJoin ? 0 : IS_ASCENDING));
        contourTreeHyperparentsPortal.Set(superID,
                                          outNeighbour | (this->isJoin ? 0 : IS_ASCENDING));
        contourTreeWhenTransferredPortal.Set(superID, this->NumIterations | IS_SUPERNODE);
      }
    } // not a leaf


    // In serial this worklet implements the following operation
    /*
      for (viskores::Id activeID = 0; activeID < activeSupernodes.GetNumberOfValues(); activeID++)
      { // per active supernode
        // retrieve the supernode ID
        viskores::Id superID = activeSupernodes[activeID];

        // test for leaf
        if ((outdegree[superID] == 0) && (indegree[superID] == 1))
                { // a leaf
                contourTree.hyperparents[superID] = superID | (isJoin ? 0 : IS_ASCENDING);
                contourTree.hyperarcs[superID] = MaskedIndex(inbound[superID]) | (isJoin ? 0 : IS_ASCENDING);
                contourTree.superarcs[superID] = MaskedIndex(inwards[superID]) | (isJoin ? 0 : IS_ASCENDING);
                contourTree.whenTransferred[superID] = this->NumIterations | IS_HYPERNODE;
                } // a leaf
        else
                { // not a leaf
                // retrieve the out neighbour
                viskores::Id outNeighbour = MaskedIndex(outbound[superID]);

                // test whether outneighbour is a leaf
                if ((outdegree[outNeighbour] != 0) || (indegree[outNeighbour] != 1))
                        continue;

                // set superarc, &c.
                contourTree.superarcs[superID] = MaskedIndex(inwards[superID]) | (isJoin ? 0 : IS_ASCENDING);
                contourTree.hyperparents[superID] = outNeighbour | (isJoin ? 0 : IS_ASCENDING);
                contourTree.whenTransferred[superID] = this->NumIterations | IS_SUPERNODE;
                } // not a leaf
      } // per active supernode

      */
  }

}; // TransferLeafChains_TransferToContourTree

} // namespace contourtree_maker_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
