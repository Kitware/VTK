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


#ifndef viskores_worklet_contourtree_augmented_contourtree_maker_inc_augment_merge_tree_init_new_join_splot_id_and_superparents_h
#define viskores_worklet_contourtree_augmented_contourtree_maker_inc_augment_merge_tree_init_new_join_splot_id_and_superparents_h

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

// Worklet for computing the sort indices from the sort order
class AugmentMergeTrees_InitNewJoinSplitIDAndSuperparents
  : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(
    FieldIn contourTreeSupernodes,      // (input) supernodes from the contour tree
    WholeArrayIn joinTreeSuperparents,  // (input)
    WholeArrayIn splitTreeSuperparents, // (input)
    WholeArrayIn joinTreeSupernodes,    // (input)
    WholeArrayIn splitTreeSupernodes,   // (input)
    WholeArrayOut joinSuperparent,      // (output)
    WholeArrayOut splitSuperparent,     // (output)
    WholeArrayOut newJoinID,            // (output)
    WholeArrayOut newSplitID);          // (output)
  typedef void ExecutionSignature(_1, InputIndex, _2, _3, _4, _5, _6, _7, _8, _9);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  AugmentMergeTrees_InitNewJoinSplitIDAndSuperparents() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& nodeID,
                                const viskores::Id supernode,
                                const InFieldPortalType& joinTreeSuperparentsPortal,
                                const InFieldPortalType& splitTreeSuperparentsPortal,
                                const InFieldPortalType& joinTreeSupernodesPortal,
                                const InFieldPortalType& splitTreeSupernodesPortal,
                                const OutFieldPortalType& joinSuperparentPortal,
                                const OutFieldPortalType& splitSuperparentPortal,
                                const OutFieldPortalType& newJoinIDPortal,
                                const OutFieldPortalType& newSplitIDPortal) const
  {
    // Transfer the join information
    // look up the join superparent in the join tree
    viskores::Id joinSuperparent = joinTreeSuperparentsPortal.Get(nodeID);
    // save the join superparent
    joinSuperparentPortal.Set(supernode, joinSuperparent);
    // now, if the join superparent's mesh ID is the node itself, we're at a join supernode
    if (joinTreeSupernodesPortal.Get(joinSuperparent) == nodeID)
      newJoinIDPortal.Set(joinSuperparent, supernode);

    // Transfer the split information
    // look up the split superparent in the split tree
    viskores::Id splitSuperparent = splitTreeSuperparentsPortal.Get(nodeID);
    // save the split superparent
    splitSuperparentPortal.Set(supernode, splitSuperparent);
    // now, if the split superparent's mesh ID is the node, we're at a split supernode
    if (splitTreeSupernodesPortal.Get(splitSuperparent) == nodeID)
      newSplitIDPortal.Set(splitSuperparent, supernode);

    // In serial this worklet implements the following operation
    /*
      for (viskores::Id supernode = 0; supernode < nSupernodes; supernode++)
        { // per supernode

          // find the regular ID for the supernode
          viskores::Id nodeID = contourTree.Supernodes[supernode];

          // Transfer the join information
          // look up the join superparent in the join tree
          viskores::Id joinSuperparent = joinTree.superparents[nodeID];
          // save the join superparent
          joinSuperparents[supernode] = joinSuperparent;
          // now, if the join superparent's mesh ID is the node itself, we're at a join supernode
          if (joinTree.Supernodes[joinSuperparent] == nodeID)
                  newJoinID[joinSuperparent] = supernode;

          // Transfer the split information
          // look up the split superparent in the split tree
          viskores::Id splitSuperparent = splitTree.superparents[nodeID];
          // save the split superparent
          splitSuperparents[supernode] = splitSuperparent;
          // now, if the split superparent's mesh ID is the node, we're at a split supernode
          if (splitTree.Supernodes[splitSuperparent] == nodeID)
                  newSplitID[splitSuperparent] = supernode;

        } // per supernode
      */
  }

}; // AugmentMergeTrees_InitNewJoinSplitIDAndSuperparents.h

} // namespace contourtree_maker_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
