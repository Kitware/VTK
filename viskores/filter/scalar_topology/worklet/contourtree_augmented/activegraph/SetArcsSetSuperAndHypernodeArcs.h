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

#ifndef viskores_worklet_contourtree_augmented_active_graph_set_arcs_set_super_and_hypernode_arcs_h
#define viskores_worklet_contourtree_augmented_active_graph_set_arcs_set_super_and_hypernode_arcs_h

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
class SetArcsSetSuperAndHypernodeArcs : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(
    WholeArrayIn graphGlobalIndex,   // (input) global index from the graph
    WholeArrayIn graphHyperarcs,     // (input) hyperarcs from the portal
    WholeArrayIn graphHyperID,       // (input) active graph hyperID
    WholeArrayOut treeArcs,          // (output) arcs of the tree
    WholeArrayOut treeSuperparents); // (output) superparents of the tree
  typedef void ExecutionSignature(_1, InputIndex, _2, _3, _4, _5);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  SetArcsSetSuperAndHypernodeArcs() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(const InFieldPortalType& graphGlobalIndexPortal,
                                const viskores::Id graphVertex,
                                const InFieldPortalType& graphHyperarcsPortal,
                                const InFieldPortalType& graphHyperIDPortal,
                                const OutFieldPortalType& treeArcsPortal,
                                const OutFieldPortalType& treeSuperparentsPortal) const
  {
    viskores::Id graphTarget = graphHyperarcsPortal.Get(graphVertex);
    // ignore all regular points
    if (!IsSupernode(graphTarget))
      return;

    // copy the target to the arcs array
    viskores::Id nodeID = graphGlobalIndexPortal.Get(graphVertex);
    if (NoSuchElement(graphTarget))
    { // trunk hypernode
      treeArcsPortal.Set(nodeID, ((viskores::Id)NO_SUCH_ELEMENT) | IS_HYPERNODE | IS_SUPERNODE);
      treeSuperparentsPortal.Set(nodeID, graphHyperIDPortal.Get(graphVertex));
    } // trunk hypernode
    else if (IsHypernode(graphTarget))
    { // hypernode
      treeArcsPortal.Set(
        nodeID, graphGlobalIndexPortal.Get(MaskedIndex(graphTarget)) | IS_HYPERNODE | IS_SUPERNODE);
      treeSuperparentsPortal.Set(nodeID, graphHyperIDPortal.Get(graphVertex));
    } // hypernode
    else
    { // supernode
      treeArcsPortal.Set(nodeID,
                         graphGlobalIndexPortal.Get(MaskedIndex(graphTarget)) | IS_SUPERNODE);
    } // supernode

    // In serial this worklet implements the following operation
    /*
      for (indexType graphVertex = 0; graphVertex < globalIndex.size(); graphVertex++)
        { // per graph vertex
          // retrieve the ID stored in the hyperarcs array
          indexType graphTarget = hyperarcs[graphVertex];

          // ignore all regular points
          if (!IsSupernode(graphTarget))
            continue;

          // copy the target to the arcs array
          indexType nodeID = globalIndex[graphVertex];
          if (NoSuchElement(graphTarget))
            { // trunk hypernode
              tree.Arcs[nodeID] = NO_SUCH_ELEMENT | IS_HYPERNODE | IS_SUPERNODE;
              tree.Superparents[nodeID] = hyperID[graphVertex];
            } // trunk hypernode
          else if (IsHypernode(graphTarget))
            { // hypernode
              tree.Arcs[nodeID] = globalIndex[MaskedIndex(graphTarget)] | IS_HYPERNODE | IS_SUPERNODE;
              tree.Superparents[nodeID] = hyperID[graphVertex];
            } // hypernode
          else
            { // supernode
              tree.Arcs[nodeID] = globalIndex[MaskedIndex(graphTarget)] | IS_SUPERNODE;
            } // supernode

        } // per graph vertex
      */
  }

}; // SetArcsSetSuperAndHypernodeArcs

} // namespace active_graph_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
