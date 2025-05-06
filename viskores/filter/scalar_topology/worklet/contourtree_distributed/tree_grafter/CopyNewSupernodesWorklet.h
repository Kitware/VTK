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

#ifndef viskores_worklet_contourtree_distributed_tree_grafter_copy_new_supernodes_worklet_h
#define viskores_worklet_contourtree_distributed_tree_grafter_copy_new_supernodes_worklet_h


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

// Worklet implementing TreeGrafter.CopyNewSupernodes
class CopyNewSupernodesWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn newSupernode,            // input and iteration index
                                WholeArrayIn contourTreeSupernodes,   // input
                                WholeArrayIn meshSorterOrder,         // input
                                WholeArrayIn hierarchicalTreeId,      // input
                                WholeArrayIn whenTransferred,         // input
                                WholeArrayIn hierarchicalSuperparent, // input
                                WholeArrayIn hierarchicalHyperparent, //input
                                WholeArrayIn hierarchicalSuperId,     // input
                                WholeArrayIn hierarchicalHyperId,     // input
                                WholeArrayIn hierarchicalHyperarc,    // input
                                WholeArrayOut hierarchicalTreeSupernodes,     // output
                                WholeArrayOut hierarchicalTreeWhichRound,     // output
                                WholeArrayOut hierarchicalTreeWhichIteration, // output
                                WholeArrayOut hierarchicalTreeSuperarcs,      // output
                                WholeArrayInOut hierarchicalRegularId,        // input/output
                                WholeArrayInOut hierarchicalTreeHyperparents, // input/output
                                WholeArrayInOut hierarchicalTreeSuperparents  // input/output
  );

  using ExecutionSignature =
    void(InputIndex, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  CopyNewSupernodesWorklet(viskores::Id theRound, viskores::Id numOldSupernodes)
    : TheRound(theRound)
    , NumOldSupernodes(numOldSupernodes)
  {
  }

  template <typename InFieldPortalType,
            typename SortOrderPortalType,
            typename OutFieldPortalType,
            typename InOutFieldPortalType>
  VISKORES_EXEC void operator()(
    const viskores::Id& newSupernode,
    const InFieldPortalType& newSupernodesPortal,
    const InFieldPortalType& contourTreeSupernodesPortal,
    const SortOrderPortalType&
      meshSortOrderPortal, // depending on the mesh type these may be differnt fancy arrays
    const InFieldPortalType& hierarchicalTreeIdPortal,
    const InFieldPortalType& whenTransferredPortal,
    const InFieldPortalType& hierarchicalSuperparentPortal,
    const InFieldPortalType& hierarchicalHyperparentPortal,
    const InFieldPortalType& hierarchicalSuperIdPortal,
    const InFieldPortalType& hierarchicalHyperIdPortal,
    const InFieldPortalType& hierarchicalHyperarcPortal,
    const OutFieldPortalType& hierarchicalTreeSupernodesPortal,
    const OutFieldPortalType& hierarchicalTreeWhichRoundPortal,
    const OutFieldPortalType& hierarchicalTreeWhichIterationPortal,
    const OutFieldPortalType& hierarchicalTreeSuperarcsPortal,
    const InOutFieldPortalType& hierarchicalRegularIdPortal,
    const InOutFieldPortalType& hierarchicalTreeHyperparentsPortal,
    const InOutFieldPortalType& hierarchicalTreeSuperparentsPortal

  ) const
  { // operator ()
    // per new supernode
    // retrieve the old supernode & regular node Ids
    viskores::Id oldSupernodeId = newSupernodesPortal.Get(newSupernode);
    viskores::Id oldSortId = contourTreeSupernodesPortal.Get(oldSupernodeId);
    viskores::Id oldRegularId = meshSortOrderPortal.Get(oldSortId);

    // convert to new Ids
    viskores::Id newRegularId = hierarchicalTreeIdPortal.Get(oldRegularId);
    viskores::Id newSupernodeId = this->NumOldSupernodes + newSupernode;

    // set the supernode accordingly
    hierarchicalTreeSupernodesPortal.Set(newSupernodeId, newRegularId);

    // and set the round and iteration
    hierarchicalTreeWhichRoundPortal.Set(newSupernodeId, this->TheRound);
    hierarchicalTreeWhichIterationPortal.Set(newSupernodeId,
                                             whenTransferredPortal.Get(oldSupernodeId));

    // We want to set the superarc and hyperparent.  At this point, supernodes fall into four groups:
    //  I.    Present in the hierarchical tree as supernodes        No work required (not in newSupernodes)
    //  II.    Present in the hierarchical tree as regular nodes only    Added as supernode. Hyperparent only needs to be set.
    //  III.  Not present in the hierarchical tree: attachment point    Super/hyper parent stored in hierarchical Ids
    //  IV.    Not present, and not an attachment point          Super/hyper parent stored in local Ids
    // Note that I. is already taken care of, so we test whether the supernode was previously in the hierarchical tree at all
    viskores::Id storedRegularId = hierarchicalRegularIdPortal.Get(oldSupernodeId);

    // and set the regular Id in the hierarchical tree (even if it is already set)
    hierarchicalRegularIdPortal.Set(oldSupernodeId, newRegularId);

    // now we sort out hyperparent
    if (!viskores::worklet::contourtree_augmented::NoSuchElement(storedRegularId))
    { // present: II
      // if it isn't already a supernode, it is "only" a regular node
      if (newSupernodeId >= this->NumOldSupernodes)
      { // regular but not super
        // in this case, it already has a superparent (because it is already present in the tree as a regular node)
        // so we look up the relevant hyperparent
        hierarchicalTreeHyperparentsPortal.Set(
          newSupernodeId,
          hierarchicalTreeHyperparentsPortal.Get(
            hierarchicalTreeSuperparentsPortal.Get(storedRegularId)));
        // we set this to indicate that it's an attachment point
        hierarchicalTreeSuperarcsPortal.Set(
          newSupernodeId, (viskores::Id)viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT);
      } // regular but not super
    }   // present: actually II
    else
    { // not present: III or IV
      // attachment point (III) or free point (IV)
      if (!viskores::worklet::contourtree_augmented::NoSuchElement(
            hierarchicalSuperparentPortal.Get(oldSupernodeId)))
      { // attachment point
        // we've already captured the super-/hyper- parent in an earlier stage
        hierarchicalTreeSuperparentsPortal.Set(newRegularId,
                                               hierarchicalSuperparentPortal.Get(oldSupernodeId));
        hierarchicalTreeHyperparentsPortal.Set(newSupernodeId,
                                               hierarchicalHyperparentPortal.Get(oldSupernodeId));
        // and the superarc should indicate an attachment point
        hierarchicalTreeSuperarcsPortal.Set(
          newSupernodeId, (viskores::Id)viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT);
      } // attachment point
      // otherwise, we have a brand new free supernode, which is it's own superparent
      else
      { // free point
        // this is a supernode that was never in the hierarchical tree in the first place
        // it is its own superparent, and has a new hyperparent in old supernode Ids (often itself)
        // and can use that to look up the new hyperId
        viskores::Id hierarchicalHyperparentOldSuperId =
          hierarchicalHyperparentPortal.Get(oldSupernodeId);
        viskores::Id hierarchicalHyperparentNewHyperId =
          hierarchicalHyperIdPortal.Get(hierarchicalHyperparentOldSuperId);
        hierarchicalTreeHyperparentsPortal.Set(newSupernodeId, hierarchicalHyperparentNewHyperId);
        // since it is its own superparent, this is easy . . .
        hierarchicalTreeSuperparentsPortal.Set(newRegularId, newSupernodeId);

        // now the hard part: fill in the superarc
        viskores::Id hierarchicalHyperarcOldSuperId =
          hierarchicalHyperarcPortal.Get(hierarchicalHyperparentOldSuperId);
        viskores::Id isAscendingHyperarc =
          viskores::worklet::contourtree_augmented::IsAscending(hierarchicalHyperarcOldSuperId)
          ? (viskores::Id)viskores::worklet::contourtree_augmented::IS_ASCENDING
          : 0x0;
        hierarchicalHyperarcOldSuperId =
          viskores::worklet::contourtree_augmented::MaskedIndex(hierarchicalHyperarcOldSuperId);
        viskores::Id hierarchicalHyperarcNewSuperId =
          hierarchicalSuperIdPortal.Get(hierarchicalHyperarcOldSuperId);

        // we have located each supernode on a hyperarc
        // and we have to work out the supernode each connects to
        // unfortunately, the attachment points complicate this compared to the old code
        // for sweeping later, we will set the # of superchildren, but we don't have that yet

        // So the test will have to be the following:
        //  i.    the "neighbour" is the +1 index
        //  ii.    if the neighbour is off the end, we take the end of the hyperarc
        //  iii.  if the neighbour has flagged as an attachment point, we take the end of the hyperarc
        //  iv.    in all other cases, we take the neighbour
        //  Note that we are saved some trouble by the fact that this code only applies to free points

        // the superarc is now set by checking to see if the neighbour has the same hyperparent:
        // if it does, our superarc goes to the next element
        // if not (or we're at array end), we go to the hyperarc's target
        // NOTE: we will store the OLD superarc Id at this stage, since we need it to sort out regular arcs
        // this means we will have to add a final loop to reset to hierarchical Ids
        viskores::Id neighbour = newSupernode + 1;

        // special case at end of array: map the old hyperarc Id to a new one
        if (neighbour >= newSupernodesPortal.GetNumberOfValues())
        { // end of array
          hierarchicalTreeSuperarcsPortal.Set(newSupernodeId,
                                              hierarchicalHyperarcNewSuperId | isAscendingHyperarc);
        } // end of array
        else
        { // not at the end of the array
          viskores::Id nbrSuperId = newSupernodesPortal.Get(neighbour);

          // immediately check for case iii. by looking at the hierarchicalSuperparent of the neighbour's old Id
          // if it's already set, it's because it's an attachment point
          if (!viskores::worklet::contourtree_augmented::NoSuchElement(
                hierarchicalSuperparentPortal.Get(nbrSuperId)))
          { // attachment point
            hierarchicalTreeSuperarcsPortal.Set(
              newSupernodeId, hierarchicalHyperarcNewSuperId | isAscendingHyperarc);
          } // attachment point
          else
          { // not attachment point
            viskores::Id nbrHyperparent = hierarchicalHyperparentPortal.Get(nbrSuperId);

            // if they share a hyperparent, just take the neighbour
            if (nbrHyperparent == hierarchicalHyperparentOldSuperId)
            { // shared hyperparent
              hierarchicalTreeSuperarcsPortal.Set(
                newSupernodeId, hierarchicalSuperIdPortal.Get(nbrSuperId) | isAscendingHyperarc);
            } // shared hyperparent
            // if not, take the target of the hyperarc
            else
            { // not shared hyperparent
              hierarchicalTreeSuperarcsPortal.Set(
                newSupernodeId, hierarchicalHyperarcNewSuperId | isAscendingHyperarc);
            } // not shared hyperparent
          }   // not attachment point
        }     // not at the end of the array
      }       // free point
    }         // attachment point (III) or free point (IV)


    // In serial this worklet implements the following operation
    /*
    for (indexType newSupernode = 0; newSupernode < nNewSupernodes; newSupernode++)
    { // per new supernode
      // retrieve the old supernode & regular node IDs
      indexType oldSupernodeID = newSupernodes[newSupernode];
      indexType oldSortID = contourTree->supernodes[oldSupernodeID];
      indexType oldRegularID = mesh->SortOrder(oldSortID);

      // convert to new IDs
      indexType newRegularID = hierarchicalTreeID[oldRegularID];
      indexType newSupernodeID = nOldSupernodes + newSupernode;

      // set the supernode accordingly
      hierarchicalTree.supernodes[newSupernodeID] = newRegularID;

      // and set the round and iteration
      hierarchicalTree.whichRound[newSupernodeID] = theRound;
      hierarchicalTree.whichIteration[newSupernodeID] = whenTransferred[oldSupernodeID];

      // We want to set the superarc and hyperparent.  At this point, supernodes fall into four groups:
      //  I.    Present in the hierarchical tree as supernodes        No work required (not in newSupernodes)
      //  II.    Present in the hierarchical tree as regular nodes only    Added as supernode. Hyperparent only needs to be set.
      //  III.  Not present in the hierarchical tree: attachment point    Super/hyper parent stored in hierarchical IDs
      //  IV.    Not present, and not an attachment point          Super/hyper parent stored in local IDs
      // Note that I. is already taken care of, so we test whether the supernode was previously in the hierarchical tree at all
      indexType storedRegularID = hierarchicalRegularID[oldSupernodeID];

      // and set the regular ID in the hierarchical tree (even if it is already set)
      hierarchicalRegularID[oldSupernodeID] = newRegularID;

      // now we sort out hyperparent
      if (!noSuchElement(storedRegularID))
        { // present: II
        // if it isn't already a supernode, it is "only" a regular node
        if (newSupernodeID >= nOldSupernodes)
          { // regular but not super
          // in this case, it already has a superparent (because it is already present in the tree as a regular node)
          // so we look up the relevant hyperparent
          hierarchicalTree.hyperparents[newSupernodeID] = hierarchicalTree.hyperparents[hierarchicalTree.superparents[storedRegularID]];
          // we set this to indicate that it's an attachment point
          hierarchicalTree.superarcs[newSupernodeID] = NO_SUCH_ELEMENT;
          } // regular but not super
        } // present: actually II
      else
        { // not present: III or IV
        // attachment point (III) or free point (IV)
        if (!noSuchElement(hierarchicalSuperparent[oldSupernodeID]))
          { // attachment point
          // we've already captured the super-/hyper- parent in an earlier stage
          hierarchicalTree.superparents[newRegularID] = hierarchicalSuperparent[oldSupernodeID];
          hierarchicalTree.hyperparents[newSupernodeID] = hierarchicalHyperparent[oldSupernodeID];
          // and the superarc should indicate an attachment point
          hierarchicalTree.superarcs[newSupernodeID] = NO_SUCH_ELEMENT;
          } // attachment point
        // otherwise, we have a brand new free supernode, which is it's own superparent
        else
          { // free point
          // this is a supernode that was never in the hierarchical tree in the first place
          // it is its own superparent, and has a new hyperparent in old supernode IDs (often itself)
          // and can use that to look up the new hyperID
          indexType hierarchicalHyperparentOldSuperID = hierarchicalHyperparent[oldSupernodeID];
          indexType hierarchicalHyperparentNewHyperID = hierarchicalHyperID[hierarchicalHyperparentOldSuperID];
          hierarchicalTree.hyperparents[newSupernodeID] = hierarchicalHyperparentNewHyperID;
          // since it is its own superparent, this is easy . . .
          hierarchicalTree.superparents[newRegularID] = newSupernodeID;

          // now the hard part: fill in the superarc
          indexType hierarchicalHyperarcOldSuperID = hierarchicalHyperarc[hierarchicalHyperparentOldSuperID];
          indexType isAscendingHyperarc = isAscending(hierarchicalHyperarcOldSuperID) ? IS_ASCENDING : 0x0;
          hierarchicalHyperarcOldSuperID = maskedIndex(hierarchicalHyperarcOldSuperID);
          indexType hierarchicalHyperarcNewSuperID = hierarchicalSuperID[hierarchicalHyperarcOldSuperID];

          // we have located each supernode on a hyperarc
          // and we have to work out the supernode each connects to
          // unfortunately, the attachment points complicate this compared to the old code
          // for sweeping later, we will set the # of superchildren, but we don't have that yet

          // So the test will have to be the following:
          //  i.    the "neighbour" is the +1 index
          //  ii.    if the neighbour is off the end, we take the end of the hyperarc
          //  iii.  if the neighbour has flagged as an attachment point, we take the end of the hyperarc
          //  iv.    in all other cases, we take the neighbour
          //  Note that we are saved some trouble by the fact that this code only applies to free points

          // the superarc is now set by checking to see if the neighbour has the same hyperparent:
          // if it does, our superarc goes to the next element
          // if not (or we're at array end), we go to the hyperarc's target
          // NOTE: we will store the OLD superarc ID at this stage, since we need it to sort out regular arcs
          // this means we will have to add a final loop to reset to hierarchical IDs
          indexType neighbour = newSupernode+1;

          // special case at end of array: map the old hyperarc ID to a new one
          if (neighbour >= newSupernodes.size())
            { // end of array
            hierarchicalTree.superarcs[newSupernodeID] = hierarchicalHyperarcNewSuperID | isAscendingHyperarc;
            } // end of array
          else
            { // not at the end of the array
            indexType nbrSuperID = newSupernodes[neighbour];

            // immediately check for case iii. by looking at the hierarchicalSuperparent of the neighbour's old ID
            // if it's already set, it's because it's an attachment point
            if (!noSuchElement(hierarchicalSuperparent[nbrSuperID]))
              { // attachment point
              hierarchicalTree.superarcs[newSupernodeID] = hierarchicalHyperarcNewSuperID | isAscendingHyperarc;
              } // attachment point
            else
              { // not attachment point
              indexType nbrHyperparent = hierarchicalHyperparent[nbrSuperID];

              // if they share a hyperparent, just take the neighbour
              if (nbrHyperparent == hierarchicalHyperparentOldSuperID)
                { // shared hyperparent
                hierarchicalTree.superarcs[newSupernodeID] =  hierarchicalSuperID[nbrSuperID] | isAscendingHyperarc;
                } // shared hyperparent
              // if not, take the target of the hyperarc
              else
                { // not shared hyperparent
                hierarchicalTree.superarcs[newSupernodeID] =  hierarchicalHyperarcNewSuperID | isAscendingHyperarc;
                } // not shared hyperparent
              } // not attachment point
            } // not at the end of the array
          } // free point
        } // attachment point (III) or free point (IV)
    } // per new supernode
  */
  } // operator ()

private:
  viskores::Id TheRound;
  viskores::Id NumOldSupernodes;

}; // CopyNewHypernodes

} // namespace tree_grafter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
