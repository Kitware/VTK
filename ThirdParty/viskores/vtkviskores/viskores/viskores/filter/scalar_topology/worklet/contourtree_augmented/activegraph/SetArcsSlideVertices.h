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

#ifndef viskores_worklet_contourtree_augmented_active_graph_set_arcs_slide_vertices_h
#define viskores_worklet_contourtree_augmented_active_graph_set_arcs_slide_vertices_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{
namespace active_graph_inc
{

// Worklet for computing the sort indices from the sort order
class SetArcsSlideVertices : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(
    WholeArrayIn treeArcs, // (input) arcs from the tree
    WholeArrayIn
      meshExtrema, // (input) extrema from the mesh (i.e, pits or peaks depending on if we have a join or split tree)
    WholeArrayIn treeFirstSuperchild,  // (input) FirstSuperchild from the tree
    WholeArrayIn treeSupernodes,       // (input) supernodes from the tree
    WholeArrayInOut treeSuperparents); // (input/output) superparents from the tree
  typedef void ExecutionSignature(_1, InputIndex, _2, _3, _4, _5);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  SetArcsSlideVertices(bool isJoinGraph, viskores::Id nSupernodes, viskores::Id nHypernodes)
    : IsJoinGraph(isJoinGraph)
    , NumSupernodes(nSupernodes)
    , NumHypernodes(nHypernodes)
  {
  }

  template <typename InFieldPortalType, typename InOutFieldPortalType>
  VISKORES_EXEC void operator()(const InFieldPortalType& treeArcsPortal,
                                const viskores::Id nodeID,
                                const InFieldPortalType& meshExtremaPortal,
                                const InFieldPortalType& treeFirstSuperchildPortal,
                                const InFieldPortalType& treeSupernodesPortal,
                                const InOutFieldPortalType& treeSuperparentsPortal) const
  {
    // ignore if the flag is already set
    if (IsSupernode(treeArcsPortal.Get(nodeID)))
    {
      return;
    }

    // start at the "top" end, retrieved from initial extremal array
    viskores::Id fromID = meshExtremaPortal.Get(nodeID);

    // get the "bottom" end from arcs array (it's a peak, so it's set already)
    viskores::Id toID = treeArcsPortal.Get(MaskedIndex(fromID));

    // loop to bottom or until to node is "below" this node
    while (!NoSuchElement(toID) &&
           (IsJoinGraph ? (MaskedIndex(toID) > MaskedIndex(nodeID))
                        : (MaskedIndex(toID) < MaskedIndex(nodeID))))
    { // sliding loop
      fromID = toID;
      toID = treeArcsPortal.Get(MaskedIndex(fromID));
    } // sliding loop

    // now we've found a hyperarc, we need to search to place ourselves on a superarc
    // it's a binary search!  first we get the hyperarc ID, which we've stored in superparents.
    viskores::Id hyperID = treeSuperparentsPortal.Get(MaskedIndex(fromID));
    viskores::Id leftSupernodeID = treeFirstSuperchildPortal.Get(hyperID);
    viskores::Id leftNodeID = treeSupernodesPortal.Get(leftSupernodeID);

    // the idea here is to compare the node ID against the node IDs for supernodes along the hyperarc
    // however, the "low" end - i.e. the end to which it is pruned, is not stored explicitly.

    // for the join tree, then, we first test to see whether the node ID is lower than the lowest node
    // ID along the hyperarc - i.e. of the lowest supernode in the range.  Which means the left-hand end.

    // for the split tree, we want to test whether the node ID is higher than the highest node ID along
    // the hyperarc. Because the supernodes & hypernodes are in reverse order in the arrays, this means
    // that the highest node ID is still at the left-hand end

    // special case for the left-hand edge
    if (IsJoinGraph ? (nodeID < leftNodeID) : (nodeID > leftNodeID))
    { // below left hand end
      treeSuperparentsPortal.Set(nodeID, leftSupernodeID);
    } // below left hand end
    else
    { // not below the left hand end
      viskores::Id rightSupernodeID;
      // the test depends on which tree we are computing
      // because the ID numbers are in reverse order in the split tree
      if (IsJoinGraph)
      { // join graph
        if (hyperID == NumHypernodes - 1)
          rightSupernodeID = NumSupernodes - 1;
        else
          rightSupernodeID = treeFirstSuperchildPortal.Get(hyperID + 1) - 1;
      } // join graph
      else
      { // split graph
        if (hyperID == 0)
          rightSupernodeID = NumSupernodes - 1;
        else
          rightSupernodeID = treeFirstSuperchildPortal.Get(hyperID - 1) - 1;
      } // split graph

      // the right end is guaranteed to be the hypernode at the top, which is not
      // being processed, so we now have a left & a right that span the vertex
      // when they meet, they must both be higher than the node itself
      while (leftSupernodeID != rightSupernodeID - 1)
      { // binary search
        viskores::Id midSupernodeID = (leftSupernodeID + rightSupernodeID) / 2;
        viskores::Id midNodeID = treeSupernodesPortal.Get(midSupernodeID);
        // this is NEVER equal, because nodeID cannot be a supernode
        if (IsJoinGraph ? (midNodeID > nodeID) : (midNodeID < nodeID))
          rightSupernodeID = midSupernodeID;
        else
          leftSupernodeID = midSupernodeID;
      } // binary search

      // we have now found the supernode/arc to which the vertex belongs
      treeSuperparentsPortal.Set(nodeID, rightSupernodeID);
    } // not below the left hand end

    // In serial this worklet implements the following operation
    /*
      for (indexType nodeID = 0; nodeID < tree.Arcs.size(); nodeID++)
        { // per node
          // ignore if the flag is already set
          if (IsSupernode(tree.Arcs[nodeID]))
            continue;

          // start at the "top" end, retrieved from initial extremal array
          viskores::Id fromID = extrema[nodeID];

          // get the "bottom" end from arcs array (it's a peak, so it's set already)
          viskores::Id toID = tree.Arcs[MaskedIndex(fromID)];

          // loop to bottom or until to node is "below" this node
          while (!NoSuchElement(toID) && (IsJoinGraph ?
                  (MaskedIndex(toID) > MaskedIndex(nodeID)): (MaskedIndex(toID) < MaskedIndex(nodeID))))
            { // sliding loop
              fromID = toID;
              toID = tree.Arcs[MaskedIndex(fromID)];
            } // sliding loop

          // now we've found a hyperarc, we need to search to place ourselves on a superarc
          // it's a binary search!  first we get the hyperarc ID, which we've stored in superparents.
          indexType hyperID = tree.Superparents[MaskedIndex(fromID)];
          indexType leftSupernodeID = tree.FirstSuperchild[hyperID];
          indexType leftNodeID = tree.Supernodes[leftSupernodeID];

          // the idea here is to compare the node ID against the node IDs for supernodes along the hyperarc
          // however, the "low" end - i.e. the end to which it is pruned, is not stored explicitly.

          // for the join tree, then, we first test to see whether the node ID is lower than the lowest node
          // ID along the hyperarc - i.e. of the lowest supernode in the range.  Which means the left-hand end.

          // for the split tree, we want to test whether the node ID is higher than the highest node ID along
          // the hyperarc. Because the supernodes & hypernodes are in reverse order in the arrays, this means
          // that the highest node ID is still at the left-hand end

          // special case for the left-hand edge
          if (IsJoinGraph ? (nodeID < leftNodeID) : (nodeID > leftNodeID))
            { // below left hand end
              tree.Superparents[nodeID] = leftSupernodeID;
            } // below left hand ned
          else
            { // not below the left hand end
              viskores::Id rightSupernodeID;
              // the test depends on which tree we are computing
              // because the ID numbers are in reverse order in the split tree
              if (IsJoinGraph)
                { // join graph
                  if (hyperID == NumHypernodes - 1)
                    rightSupernodeID = NumSupernodes - 1;
                  else
                    rightSupernodeID = tree.FirstSuperchild[hyperID + 1] - 1;
                } // join graph
              else
                { // split graph
                  if (hyperID == 0)
                    rightSupernodeID = NumSupernodes - 1;
                  else
                    rightSupernodeID = tree.FirstSuperchild[hyperID - 1] - 1;
                } // split graph

              // the right end is guaranteed to be the hypernode at the top, which is not
              // being processed, so we now have a left & a right that span the vertex
              // when they meet, they must both be higher than the node itself
              while (leftSupernodeID != rightSupernodeID-1)
                { // binary search
                  viskores::Id midSupernodeID = (leftSupernodeID + rightSupernodeID) / 2;
                  // std::cout << "Mid Supernode ID:     " << midSupernodeID << std::endl;
                  viskores::Id midNodeID = tree.Supernodes[midSupernodeID];
                  // std::cout << "Mid Node ID:          " << midNodeID << std::endl;
                  // this is NEVER equal, because nodeID cannot be a supernode
                  if (IsJoinGraph ? (midNodeID > nodeID) : (midNodeID < nodeID))
                          rightSupernodeID = midSupernodeID;
                  else
                          leftSupernodeID = midSupernodeID;
                } // binary search

                // we have now found the supernode/arc to which the vertex belongs
                tree.Superparents[nodeID] = rightSupernodeID;
            } // not below the left hand end
        } // per node
      */
  }

private:
  bool IsJoinGraph;
  viskores::Id NumSupernodes;
  viskores::Id NumHypernodes;

}; // SetArcsSlideVertices

} // namespace active_graph_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
