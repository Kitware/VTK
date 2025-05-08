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

#ifndef viskores_worklet_contourtree_augmented_active_graph_set_super_arcs_set_tree_superarcs_h
#define viskores_worklet_contourtree_augmented_active_graph_set_super_arcs_set_tree_superarcs_h

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
class SetSuperArcsSetTreeSuperarcs : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(
    FieldIn treeSupernodes,             // (input) supernodes of the tree
    WholeArrayIn hyperarcs,             // (input) hyperarcs from the active graph
    WholeArrayIn treeHyperparents,      // (input) hyperparents from the tree
    WholeArrayIn superId,               // (input) superID from the active graph
    WholeArrayIn hyperId,               // (input) hyperID from the active graph
    WholeArrayOut treeSuperarcs,        // (output) superarcs from the tree
    WholeArrayOut treeFirstSuperchild); // (output) FirstSuperchild from the tree
  typedef void ExecutionSignature(_1, InputIndex, _2, _3, _4, _5, _6, _7);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  SetSuperArcsSetTreeSuperarcs() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(
    const viskores::Id& /*graphVertex*/, // FIXME: Remove unused parameter?
    const viskores::Id supernode,
    const InFieldPortalType& hyperarcsPortal,
    const InFieldPortalType& treeHyperparentsPortal,
    const InFieldPortalType& superIDPortal,
    const InFieldPortalType& hyperIDPortal,
    const OutFieldPortalType& treeSuperarcsPortal,
    const OutFieldPortalType& treeFirstSuperchildPortal) const
  {
    // retrieve the hyperparent (which is still a graph index, not a hypernode index)
    viskores::Id hyperparent = treeHyperparentsPortal.Get(supernode);

    // work out whether we have the first (closest to saddle) supernode on the hyperarc
    bool firstSupernode = false;
    if (supernode == 0)
    { // end of the array
      firstSupernode = true;
    } // end of the array
    else
    { // the general case
      // retrieve the previous supernode ID & hyperarc ID
      viskores::Id prevHyperparent = treeHyperparentsPortal.Get(supernode - 1);

      // now that we have the two hyperarcs, compare them
      firstSupernode = (hyperparent != prevHyperparent);
    } // the general case

    // now we can set the superarcs
    // the last in the segment retrieves the hyperarc, masks out the flags, then does a reverse lookup
    // to find the position in the supernode index
    if (firstSupernode)
    { // first supernode
      // this needs to point to the supernode at the "bottom" end of the hyperarc
      viskores::Id prunesTo = hyperarcsPortal.Get(hyperparent);

      if (NoSuchElement(prunesTo))
        treeSuperarcsPortal.Set(supernode, (viskores::Id)NO_SUCH_ELEMENT);
      else
        treeSuperarcsPortal.Set(supernode, superIDPortal.Get(MaskedIndex(prunesTo)));

      // we also need to set the first superchild for the hypergraph
      treeFirstSuperchildPortal.Set(hyperIDPortal.Get(hyperparent), supernode);
    } // first supernode
    // all others just point to their neighbour
    else
    { // not first
      treeSuperarcsPortal.Set(supernode, supernode - 1);
    } // not first

    // In serial this worklet implements the following operation
    /*
      // Each supernode points to its neighbour in the list, except at the end of segments
      for (indexType supernode = 0; supernode < nSupernodes; supernode++)
      { // per supernode
        // retrieve the actual supernode ID in the graph
        indexType graphIndex = tree.Supernodes[supernode];
        // retrieve the hyperparent (which is still a graph index, not a hypernode index)
        indexType hyperparent = tree.hyperparents[supernode];

        // work out whether we have the first (closest to saddle) supernode on the hyperarc
        bool firstSupernode = false;
        if (supernode == 0)
          { // end of the array
            firstSupernode = true;
          } // end of the array
        else
          { // the general case
            // retrieve the previous supernode ID & hyperarc ID
            indexType prevHyperparent = tree.hyperparents[supernode-1];

            // now that we have the two hyperarcs, compare them
            firstSupernode = (hyperparent != prevHyperparent);
          } // the general case

        // now we can set the superarcs
        // the last in the segment retrieves the hyperarc, masks out the flags, then does a reverse lookup
        // to find the position in the supernode index
        if (firstSupernode)
          { // first supernode
            // this needs to point to the supernode at the "bottom" end of the hyperarc
            indexType prunesTo = hyperarcs[hyperparent];

            if (NoSuchElement(prunesTo))
              tree.superarcs[supernode] = NO_SUCH_ELEMENT;
            else
              tree.superarcs[supernode] = superID[MaskedIndex(prunesTo)];

            // we also need to set the first superchild for the hypergraph
            tree.FirstSuperchild[hyperID[hyperparent]] = supernode;
          } // first supernode
        // all others just point to their neighbour
        else
          { // not first
            tree.superarcs[supernode] = supernode-1;
          } // not first
      } // per supernode

      */
  }

}; // SetSuperArcsSetTreeSuperarcs

} // namespace active_graph_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
