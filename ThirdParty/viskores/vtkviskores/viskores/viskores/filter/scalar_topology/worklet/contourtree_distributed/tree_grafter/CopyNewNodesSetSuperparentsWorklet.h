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

#ifndef viskores_worklet_contourtree_distributed_tree_grafter_copy_new_nodes_set_superparents_worklet_h
#define viskores_worklet_contourtree_distributed_tree_grafter_copy_new_nodes_set_superparents_worklet_h


#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{
namespace tree_grafter
{

// Worklet implementing the sorting out the superparents as part of TreeGrafter::CopyNewNodes
class CopyNewNodesSetSuperparentsWorklet : public viskores::worklet::WorkletMapField
{
public:
  // TODO: Some WholeArrayIn could potentially be made FieldIn if we reshuffeled the arrays via newNodes beforehand
  using ControlSignature = void(FieldIn newNodes,                     // input and iteration index
                                WholeArrayIn meshSortIndex,           // input
                                WholeArrayIn meshSortOrder,           // input
                                WholeArrayIn contourTreeSuperparents, // input
                                WholeArrayIn contourTreeSuperarcs,    // input
                                WholeArrayIn contourTreeSupernodes,   // input
                                WholeArrayIn hierarchicalRegularId,   // input
                                WholeArrayIn hierarchicalTreeId,      // input
                                WholeArrayIn hierarchicalTreeRegularNodeGlobalIds, // input
                                WholeArrayIn hierarchicalTreeDataValues,           // input
                                ExecObject findSuperArcForUnknownNode,             // input
                                WholeArrayOut hierarchicalTreeSuperparents         // output
  );

  using ExecutionSignature = void(InputIndex, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  CopyNewNodesSetSuperparentsWorklet(const viskores::Id& numOldNodes)
    : NumOldNodes(numOldNodes)
  {
  }

  template <typename InFieldPortalType,
            typename MeshSortIndexPortalType,
            typename MeshSortOrderPortalType,
            typename DataValuePortalType,
            typename FindSuperExecType,
            typename OutFieldPortalType>
  VISKORES_EXEC void operator()(
    const viskores::Id& newNode,
    const viskores::Id&
      oldNodeId, // convert to a node Id in the current level's tree when calling the worklet
    const MeshSortIndexPortalType& meshSortIndexPortal,
    const MeshSortOrderPortalType& meshSortOrderPortal,
    const InFieldPortalType& contourTreeSuperparentsPortal,
    const InFieldPortalType& contourTreeSuperarcsPortal,
    const InFieldPortalType& contourTreeSupernodesPortal,
    const InFieldPortalType& hierarchicalRegularIdPortal,
    const InFieldPortalType& hierarchicalTreeIdPortal,
    const InFieldPortalType& hierarchicalTreeRegularNodeGlobalIdsPortal,
    const DataValuePortalType& hierarchicalTreeDataValuesPortal,
    const FindSuperExecType& findSuperArcForUnknownNode,
    const OutFieldPortalType& hierarchicalTreeSuperparentsPortal

  ) const
  { // operator ()
    // per new node
    // index into hierarchical tree
    viskores::Id newNodeId = this->NumOldNodes + newNode;
    // retrieve the old parent superarc
    viskores::Id oldSortIndex = meshSortIndexPortal.Get(oldNodeId);
    viskores::Id oldSuperparent = contourTreeSuperparentsPortal.Get(oldSortIndex);
    // and to a regular Id
    viskores::Id oldSuperparentNewRegularId = hierarchicalRegularIdPortal.Get(oldSuperparent);

    // Assuming that the new supernodes & hypernodes have been transferred, EVERY supernode in the old tree
    // now has hierarchicalRegularId set correctly.  Since every regular node belongs on a superarc in the old tree,
    // we can use the ends of the superarc to invoke a search in the hierarchical tree for the superparent.
    // This is therefore logically dependent on having the superstructure & hyperstructure updated first

    // supernodes will already have their superparent set in CopyNewSupernodes()
    if (viskores::worklet::contourtree_augmented::NoSuchElement(
          hierarchicalTreeSuperparentsPortal.Get(newNodeId)))
    { // it's not a supernode
      // retrieve the end of the superarc & convert to hierarchical regular Id, plus identify whether it ascends
      viskores::Id oldSupertargetSuperId = contourTreeSuperarcsPortal.Get(oldSuperparent);
      bool oldSuperarcAscends =
        viskores::worklet::contourtree_augmented::IsAscending(oldSupertargetSuperId);
      oldSupertargetSuperId =
        viskores::worklet::contourtree_augmented::MaskedIndex(oldSupertargetSuperId);
      viskores::Id oldSupertargetOldSortId = contourTreeSupernodesPortal.Get(oldSupertargetSuperId);
      viskores::Id oldSupertargetOldRegularId = meshSortOrderPortal.Get(oldSupertargetOldSortId);
      viskores::Id oldSupertargetNewRegularId =
        hierarchicalTreeIdPortal.Get(oldSupertargetOldRegularId);

      // set up variables for our pruning search
      // collect the low end's values
      viskores::Id lowEndRegularId =
        oldSuperarcAscends ? oldSuperparentNewRegularId : oldSupertargetNewRegularId;
      viskores::Id highEndRegularId =
        oldSuperarcAscends ? oldSupertargetNewRegularId : oldSuperparentNewRegularId;

      // pull the data value at the node
      viskores::Id nodeGlobalId = hierarchicalTreeRegularNodeGlobalIdsPortal.Get(newNodeId);
      auto nodeValue = hierarchicalTreeDataValuesPortal.Get(newNodeId);

      // now ask the hierarchical tree for the correct superparent
      hierarchicalTreeSuperparentsPortal.Set(
        newNodeId,
        findSuperArcForUnknownNode.FindSuperArcForUnknownNode(
          nodeGlobalId, nodeValue, highEndRegularId, lowEndRegularId));
    } // it's not a supernode


    // In serial this worklet implements the following operation
    /*
    for (indexType newNode = 0; newNode < newNodes.size(); newNode++)
    { // per new node
      // convert to a node ID in the current level's tree
      indexType oldNodeID = newNodes[newNode];
      // index into hierarchical tree
      indexType newNodeID = nOldNodes + newNode;
      // retrieve the old parent superarc
      indexType oldSortIndex = mesh->SortIndex(oldNodeID);
      indexType oldSuperparent = contourTree->superparents[oldSortIndex];
      // and to a regular ID
      indexType oldSuperparentNewRegularID = hierarchicalRegularID[oldSuperparent];

      // Assuming that the new supernodes & hypernodes have been transferred, EVERY supernode in the old tree
      // now has hierarchicalRegularID set correctly.  Since every regular node belongs on a superarc in the old tree,
      // we can use the ends of the superarc to invoke a search in the hierarchical tree for the superparent.
      // This is therefore logically dependent on having the superstructure & hyperstructure updated first

      // supernodes will already have their superparent set in CopyNewSupernodes()
      if (noSuchElement(hierarchicalTree.superparents[newNodeID]))
        { // it's not a supernode
        // retrieve the end of the superarc & convert to hierarchical regular ID, plus identify whether it ascends
        indexType oldSupertargetSuperID = contourTree->superarcs[oldSuperparent];
        bool oldSuperarcAscends = isAscending(oldSupertargetSuperID);
        oldSupertargetSuperID = maskedIndex(oldSupertargetSuperID);
        indexType oldSupertargetOldSortID = contourTree->supernodes[oldSupertargetSuperID];
        indexType oldSupertargetOldRegularID = mesh->SortOrder(oldSupertargetOldSortID);
        indexType oldSupertargetNewRegularID = hierarchicalTreeID[oldSupertargetOldRegularID];

        // set up variables for our pruning search
        // collect the low end's values
        indexType lowEndRegularID = oldSuperarcAscends ? oldSuperparentNewRegularID : oldSupertargetNewRegularID;
        indexType highEndRegularID = oldSuperarcAscends ? oldSupertargetNewRegularID : oldSuperparentNewRegularID;

        // pull the data value at the node
        indexType nodeGlobalID = hierarchicalTree.regularNodeGlobalIDs[newNodeID];
        dataType nodeValue = hierarchicalTree.dataValues[newNodeID];

        // now ask the hierarchical tree for the correct superparent
        hierarchicalTree.superparents[newNodeID] = hierarchicalTree.FindSuperArcForUnknownNode(nodeGlobalID, nodeValue, highEndRegularID, lowEndRegularID);
        } // it's not a supernode
    } // per new node
    */
  } // operator ()

private:
  viskores::Id NumOldNodes;

}; // CopyNewHypernodes

} // namespace tree_grafter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
