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

#ifndef viskores_worklet_contourtree_distributed_bract_maker_find_boundary_tree_superacrs_superarc_to_worklet_h
#define viskores_worklet_contourtree_distributed_bract_maker_find_boundary_tree_superacrs_superarc_to_worklet_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{
namespace bract_maker
{

/// Compute the superarc "to" for every bract node
/// Part of the BoundaryRestrictedAugmentedContourTree.FindBoundaryTreeSuperarcs function
class FindBoundaryTreeSuperarcsSuperarcToWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn bractVertexSuperset,     // input
                                WholeArrayIn boundaryIndices,         // input
                                WholeArrayIn boundaryTreeId,          // input
                                WholeArrayIn contourtreeSuperparents, // input
                                WholeArrayIn contourtreeHyperparents, // input
                                WholeArrayIn contourtreeHyperarcs,    // input
                                WholeArrayIn contourtreeSupernodes,   // input
                                WholeArrayIn meshSortOrder,           // input
                                WholeArrayOut treeToSuperset,         // output
                                FieldOut bractSuperarcs               // output
  );
  using ExecutionSignature = _10(InputIndex, _1, _2, _3, _4, _5, _6, _7, _8, _9);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  FindBoundaryTreeSuperarcsSuperarcToWorklet() {}

  template <typename InFieldPortalType,
            typename MeshSortOrderPortalType,
            typename OutFieldPortalType>
  VISKORES_EXEC viskores::Id operator()(const viskores::Id& from,
                                        const InFieldPortalType& bractVertexSupersetPortal,
                                        const InFieldPortalType& boundaryIndicesPortal,
                                        const InFieldPortalType& boundaryTreeIdPortal,
                                        const InFieldPortalType& contourtreeSuperparentsPortal,
                                        const InFieldPortalType& contourtreeHyperparentsPortal,
                                        const InFieldPortalType& contourtreeHyperarcsPortal,
                                        const InFieldPortalType& contourtreeSupernodesPortal,
                                        const MeshSortOrderPortalType& meshSortOrderPortal,
                                        const OutFieldPortalType& treeToSupersetPortal) const
  {
    // find the sort order, super- and hyper- parent
    // viskores::Id fromIndex = bractVertexSupersetPortal.Get(from);
    viskores::Id fromSort = boundaryIndicesPortal.Get(from);
    viskores::Id fromSuper = contourtreeSuperparentsPortal.Get(fromSort);
    viskores::Id fromHyper = contourtreeHyperparentsPortal.Get(fromSuper);

    // also allocate space for the from, with index, &c.
    viskores::Id to = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
    // viskores::Id toIndex = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;  // set but not used
    viskores::Id toSort = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
    viskores::Id toSuper = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
    viskores::Id toHyper = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;

    // for any vertex OTHER than the one at the RHE, set these variables
    if (from != bractVertexSupersetPortal.GetNumberOfValues() - 1)
    { // not RHE
      to = from + 1;
      // toIndex = bractVertexSupersetPortal.Get(to);  // set but not used
      toSort = boundaryIndicesPortal.Get(to);
      toSuper = contourtreeSuperparentsPortal.Get(toSort);
      toHyper = contourtreeHyperparentsPortal.Get(toSuper);
    } // not RHE

    // while we are here, we want to establish the mapping from the contour tree ID to the superset ID
    // so we test whether the node is a supernode, by seeing if it's sort ID matches its superparent
    // In the original code this was done last. We do this here first, because the values for
    // bractSuperarcs are set by return statements, so we don't reach the end of the function
    // and reordering the operation should be no problem.
    if (contourtreeSupernodesPortal.Get(fromSuper) == fromSort)
    { // supernode
      treeToSupersetPortal.Set(fromSuper, from);
    } // supernode

    // the easy case - there is a "hyper-neighbour" to link to
    if (fromHyper == toHyper)
    {            // hyperparents match
      return to; // same as bractSuperarcs.WritePortal().Set(from, to);
    }            // hyperparents match
    // the rest: we're at the RHE of a hyperarc and need to connect onwards
    else
    { // hyperparents do not match
      // retrieve the hypertarget
      viskores::Id hyperTarget = contourtreeHyperarcsPortal.Get(fromHyper);

      // if it's non-existent, we're at the root, in which case our from vertex becomes the root
      if (viskores::worklet::contourtree_augmented::NoSuchElement(hyperTarget))
      { // root vertex
        // same as bractSuperarcs.WritePortal().Set(from, NO_SUCH_ELEMENT)
        return viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
      } // root vertex
      // otherwise it points to a supernode
      else
      { // not root vertex
        // check whether the target will be in the BRACT
        viskores::Id regularTargetId = meshSortOrderPortal.Get(contourtreeSupernodesPortal.Get(
          viskores::worklet::contourtree_augmented::MaskedIndex(hyperTarget)));
        // now look up the ID in the BRACT
        viskores::Id bract_id = boundaryTreeIdPortal.Get(regularTargetId);

        // if it is not in the tree, then this node becomes the root
        if (viskores::worklet::contourtree_augmented::NoSuchElement(bract_id))
        {
          // same as bractSuperarcs.WritePortal().Set(from, NO_SUCH_ELEMENT)
          return viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
        }
        // otherwise, we use the ID just retrieved
        else
        {
          // same as // same as bractSuperarcs.WritePortal().Set(from, bract_it)
          return bract_id;
        }
      } // not root vertex
    }   // hyperparents do not match

    // In serial this worklet implements the following operation
    /*
    for (indexType from = 0; from < bractVertexSuperset.size(); from++)
    { // per vertex in boundary tree
      // find the sort order, super- and hyper- parent
      indexType fromIndex = bractVertexSuperset[from], fromSort = boundaryIndices[from];
      indexType fromSuper = contourTree->superparents[fromSort], fromHyper = contourTree->hyperparents[fromSuper];

      // also allocate space for the from, with index, &c.
      indexType to = NO_SUCH_ELEMENT, toIndex = NO_SUCH_ELEMENT, toSort = NO_SUCH_ELEMENT;
      indexType toSuper = NO_SUCH_ELEMENT, toHyper = NO_SUCH_ELEMENT;

      // for any vertex OTHER than the one at the RHE, set these variables
      if (from != bractVertexSuperset.size() - 1)
        { // not RHE
        to = from + 1;
        toIndex = bractVertexSuperset[to];
        toSort = boundaryIndices[to];
        toSuper = contourTree->superparents[toSort];
        toHyper = contourTree->hyperparents[toSuper];
        } // not RHE

      // the easy case - there is a "hyper-neighbour" to link to
      if (fromHyper == toHyper)
        { // hyperparents match
        bract->superarcs[from] = to;
        } // hyperparents match
      else
      // the rest: we're at the RHE of a hyperarc and need to connect onwards
        { // hyperparents do not match
        // retrieve the hypertarget
        indexType hyperTarget = contourTree->hyperarcs[fromHyper];

        // if it's non-existent, we're at the root, in which case our from vertex becomes the root
        if (noSuchElement(hyperTarget))
          { // root vertex
          bract->superarcs[from] = NO_SUCH_ELEMENT;
          } // root vertex
        // otherwise it points to a supernode
        else
          { // not root vertex
          // check whether the target will be in the BRACT
          indexType regularTargetID = mesh->SortOrder(contourTree->supernodes[maskedIndex(hyperTarget)]);

          // now look up the ID in the BRACT
          indexType BRACTID = boundaryTreeID[regularTargetID];

          // if it is not in the tree, then this node becomes the root
          if (noSuchElement(BRACTID))
            bract->superarcs[from] = NO_SUCH_ELEMENT;
          // otherwise, we use the ID just retrieved
          else
            bract->superarcs[from] = BRACTID;

          } // not root vertex

        } // hyperparents do not match

      // while we are here, we want to establish the mapping from the contour tree ID to the superset ID
      // so we test whether the node is a supernode, by seeing if it's sort ID matches its superparent
      if (contourTree->supernodes[fromSuper] == fromSort)
        { // supernode
        tree2Superset[fromSuper] = from;
        } // supernode
    } // per vertex in boundary tree
    */
  } // operator()()

}; // FindBoundaryTreeSuperarcsSuperarcToWorklet


} // namespace bract_maker
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
