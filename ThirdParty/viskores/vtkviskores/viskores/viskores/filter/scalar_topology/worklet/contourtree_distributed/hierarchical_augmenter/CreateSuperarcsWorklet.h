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
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#ifndef viskores_worklet_contourtree_distributed_hierarchical_augmenter_create_superarcs_worklet_h
#define viskores_worklet_contourtree_distributed_hierarchical_augmenter_create_superarcs_worklet_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{
namespace hierarchical_augmenter
{

/// Worklet used to implement the main part of HierarchicalAugmenter::CreateSuperarcs
/// Connect superarcs for the level & set hyperparents & superchildren count, whichRound,
/// whichIteration, super2hypernode
template <typename FieldType>
class CreateSuperarcsWorklet : public viskores::worklet::WorkletMapField
{
public:
  // TODO: Check if augmentedTreeFirstSupernodePerIteration could be changed to WholeArrayOut or if we need the In to preserve orignal values

  /// Control signature for the worklet
  /// @param[in] supernodeSorter input domain. We need access to InputIndex and InputIndex+1,
  ///                           therefore this is a WholeArrayIn transfer.
  /// @param[in] supernodeIdSetPermuted Field in of supernodeIdSet permuted by the supernodeSorter array to
  ///                                 allow us to use FieldIn
  /// @param[in] globalRegularIdSetPermuted Field in of globalRegularIdSet permuted by supernodeSorter array to
  ///                           allow use of FieldIn
  /// @param[in] dataValueSetPermuted Field in of dataValyeSet permuted by supernodeSorter array to
  ///                           allow use of FieldIn
  /// @param[in] ExecObject findSuperArcForUnknownNode Execute object in to find the superarc of arbitrary node
  /// @param[in] ExecObject createSuperarcsData Data object in, storing many BaseTree arrays
  /// @param[out, in] augmentedTreeSupernodes  this->AugmentedTree->Supernodes array
  /// @param[out] augmentedTreeSuperarcsView  output view of  this->AugmentedTree->Superarcs with
  ///                           viskores::cont::make_ArrayHandleView(this->AugmentedTree->Superarcs,
  ///                           numSupernodesAlready, this->SupernodeSorter.GetNumberOfValues()).
  ///                           By using this view allows us to do this one as a FieldOut and it effectively the
  ///                           same as accessing the array at the newSuppernodeId location.
  /// @param[out] augmentedTreeHyperparentsView  output view of  this->AugmentedTree->Hyperparents with
  ///                           viskores::cont::make_ArrayHandleView(this->AugmentedTree->Hyperparents,
  ///                           numSupernodesAlready, this->SupernodeSorter.GetNumberOfValues()).
  ///                           By using this view allows us to do this one as a FieldOut and it effectively the
  ///                           same as accessing the array at the newSuppernodeId location.
  /// @param[out] augmentedTreeSuper2HypernodeView  output view of  this->AugmentedTree->Super2Hypernode with
  ///                           viskores::cont::make_ArrayHandleView(this->AugmentedTree->Super2Hypernode,
  ///                           numSupernodesAlready, this->SupernodeSorter.GetNumberOfValues()).
  ///                           By using this view allows us to do this one as a FieldOut and it effectively the
  ///                           same as accessing the array at the newSuppernodeId location.
  /// @param[out] augmentedTreeWhichRoundView  output view of  this->AugmentedTree->WhichRound with
  ///                           viskores::cont::make_ArrayHandleView(this->AugmentedTree->WhichRound,
  ///                           numSupernodesAlready, this->SupernodeSorter.GetNumberOfValues()).
  ///                           By using this view allows us to do this one as a FieldOut and it effectively the
  ///                           same as accessing the array at the newSuppernodeId location.
  /// @param[out] augmentedTreeWhichIterationView  output view of  this->AugmentedTree->WhichIteration with
  ///                           viskores::cont::make_ArrayHandleView(this->AugmentedTree->WhichIteration,
  ///                           numSupernodesAlready, this->SupernodeSorter.GetNumberOfValues()).
  ///                           By using this view allows us to do this one as a FieldOut and it effectively the
  ///                           same as accessing the array at the newSuppernodeId location.
  /// @param[out] augmentedTreeRegularNodeGlobalIdsView  output view of  this->AugmentedTree->RegularNodeGlobalIds with
  ///                           viskores::cont::make_ArrayHandleView(this->AugmentedTree->RegularNodeGlobalIds,
  ///                           numSupernodesAlready, this->SupernodeSorter.GetNumberOfValues()).
  ///                           By using this view allows us to do this one as a FieldOut and it effectively the
  ///                           same as accessing the array at the newRegularId location.
  /// @param[out] augmentedTreeDataValuesView  output view of  this->AugmentedTree->DataValues with
  ///                           viskores::cont::make_ArrayHandleView(this->AugmentedTree->DataValues,
  ///                           numSupernodesAlready, this->SupernodeSorter.GetNumberOfValues()).
  ///                           By using this view allows us to do this one as a FieldOut and it effectively the
  ///                           same as accessing the array at the newRegularId location.
  /// @param[out] augmentedTreeRegular2SupernodeView  output view of  this->AugmentedTree->Regular2Supernode with
  ///                           viskores::cont::make_ArrayHandleView(this->AugmentedTree->Regular2Supernode,
  ///                           numSupernodesAlready, this->SupernodeSorter.GetNumberOfValues()).
  ///                           By using this view allows us to do this one as a FieldOut and it effectively the
  ///                           same as accessing the array at the newRegularId location.
  /// @param[out] augmentedTreeSuperparentsView  output view of  this->AugmentedTree->Superparents with
  ///                           viskores::cont::make_ArrayHandleView(this->AugmentedTree->Superparents,
  ///                           numSupernodesAlready, this->SupernodeSorter.GetNumberOfValues()).
  ///                           By using this view allows us to do this one as a FieldOut and it effectively the
  ///                           same as accessing the array at the newRegularId location.

  using ControlSignature = void(
    // Inputs
    WholeArrayIn supernodeSorter,
    FieldIn supernodeIdSetPermuted,
    FieldIn globalRegularIdSetPermuted,
    FieldIn dataValueSetPermuted,
    ExecObject findSuperArcForUnknownNode,
    ExecObject createSuperarcsData,
    // Outputs
    WholeArrayInOut augmentedTreeSupernodes,
    FieldOut augmentedTreeSuperarcsView,
    FieldOut augmentedTreeHyperparentsView,
    FieldOut augmentedTreeSuper2Hypernode,
    FieldOut augmentedTreeWhichRoundView,
    FieldOut augmentedTreeWhichIterationView,
    FieldOut augmentedTreeRegularNodeGlobalIdsView,
    FieldOut augmentedTreeDataValuesView,
    FieldOut augmentedTreeRegular2SupernodeView,
    FieldOut augmentedTreeSuperparentsViews);
  using ExecutionSignature =
    void(InputIndex, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16);

  using InputDomain = _1;

  /// Default Constructor
  /// @param[in] numSupernodesAlready Set to viskores::cont::ArrayGetValue(0, this->AugmentedTree->FirstSupernodePerIteration[roundNumber]);
  /// @param[in] baseTreeNumRounds Set to this->BaseTree->NumRounds
  /// @param[in] numInsertedSupernodes Set to  numInsertedSupernodes
  VISKORES_EXEC_CONT
  CreateSuperarcsWorklet(const viskores::Id& numSupernodesAlready,
                         const viskores::Id& baseTreeNumRounds,
                         const viskores::Id& numInsertedSupernodes,
                         const viskores::Id& roundNo)
    : NumSupernodesAlready(numSupernodesAlready)
    , BaseTreeNumRounds(baseTreeNumRounds)
    , NumInsertedSupernodes(numInsertedSupernodes)
    , RoundNo(roundNo)
  {
  }

  /// operator() of the workelt
  template <typename InFieldPortalType,
            typename ExecObjType,
            typename ExecObjectTypeData,
            typename InOutFieldPortalType>
  //typename InOutDataFieldPortalType>
  //typename InOutFieldPortalType,
  //typename ExecObjectTypeData,
  //typename ExecObjType>
  VISKORES_EXEC void operator()(
    // Inputs
    const viskores::Id& supernode, // InputIndex of supernodeSorter
    const InFieldPortalType& supernodeSorterPortal,
    const viskores::Id& oldSupernodeId, //  supernodeIdSet[supernodeSorterPortal.Get(supernode)]
    const viskores::Id&
      globalRegularIdSetValue, //  globalRegularIdSet[supernodeSorterPortal.Get(supernode)]];
    const FieldType& dataValueSetValue, //  dataValueSet[supernodeSorterPortal.Get(supernode)]];
    const ExecObjType&
      findSuperArcForUnknownNode, // Execution object to call FindSuperArcForUnknownNode
    const ExecObjectTypeData&
      createSuperarcsData, // Execution object of collect BaseTree data array
    // Outputs
    const InOutFieldPortalType& augmentedTreeSupernodesPortal,
    viskores::Id&
      augmentedTreeSuperarcsValue, // set value for AugmentedTree->Superarcs[newSupernodeId]
    viskores::Id&
      augmentedTreeHyperparentsValue, // set value for AugmentedTree->Hyperparents[newSupernodeId]
    viskores::Id&
      augmentedTreeSuper2HypernodeValue, // set value for AugmentedTree->Super2Hypernode[newSupernodeId]
    viskores::Id& augmentedTreeWhichRoundValue,     // AugmentedTree->WhichRound[newSupernodeId]
    viskores::Id& augmentedTreeWhichIterationValue, // AugmentedTree->WhichIteration[newSupernodeId]
    viskores::Id&
      augmentedTreeRegularNodeGlobalIdsValue, // AugmentedTree->RegularNodeGlobalIds[newRegularID]
    FieldType& augmentedTreeDataValuesValue,  // AugmentedTree->DataValues[newRegularID]
    viskores::Id&
      augmentedTreeRegular2SupernodeValue,       // AugmentedTree->Regular2Supernode[newRegularID]
    viskores::Id& augmentedTreeSuperparentsValue // AugmentedTree->Superparents[newRegularID]
  ) const
  {
    // per supernode in the set
    // retrieve the index from the sorting index array
    viskores::Id supernodeSetIndex = supernodeSorterPortal.Get(supernode);

    // work out the new supernode ID
    viskores::Id newSupernodeId = this->NumSupernodesAlready + supernode;

    // and the old supernode ID
    //  viskores::Id oldSupernodeId = supernodeIDSet[supernodeSetIndex];  Extracted on call and provides as input

    //	At all levels above 0, we used to keep regular vertices in case they are attachment points.
    //  After augmentation, we don't need to.
    //	Instead, at all levels above 0, the regular nodes in each round are identical to the supernodes
    //	In order to avoid confusion, we will copy the ID into a separate variable
    viskores::Id newRegularId = newSupernodeId;

    // setting the supernode's regular ID is now trivial
    augmentedTreeSupernodesPortal.Set(newSupernodeId, newRegularId);

    // retrieve the old superID of the superparent.  This is slightly tricky, as we have four classes of supernodes:
    // 1. the root of the entire tree
    // 2. attachment points not being inserted. In this case, the supernode ID is stored in the superparentSet
    //    array, not the superparent for insertion purposes
    // 3. attachment points being inserted.  In this case, the superparent is stored in the superparentSet array
    // 4. "ordinary" supernodes, where the superparent is the same as the supernode ID anyway
    //
    // Note that an attachment point gets inserted into a parent superarc.  But the attachment point itself has
    // a NULL superarc, because it's only a virtual insertion.
    // This means that such an attachment superarc cannot be the superparent of any other attachment point
    // It is therefore reasonable to deal with 1. & 2 separately. 3. & 4. then combine together

    // first we test for the root of the tree
    if ((this->RoundNo == BaseTreeNumRounds) &&
        (supernode == supernodeSorterPortal.GetNumberOfValues() - 1))
    { // root of the tree
      // note that oldSupernodeID is guaranteed not to be NO_SUCH_ELEMENT, as the root is in every tree
      // set the super arrays
      augmentedTreeSuperarcsValue = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
      // hyperstructure carries over, so we use the same hyperparent as before
      augmentedTreeHyperparentsValue = createSuperarcsData.BaseTreeHyperparents.Get(oldSupernodeId);
      // and set the hypernode ID
      augmentedTreeSuper2HypernodeValue =
        createSuperarcsData.BaseTreeSuper2Hypernode.Get(oldSupernodeId);
      // and the round and iteration
      augmentedTreeWhichRoundValue = createSuperarcsData.BaseTreeWhichRound.Get(oldSupernodeId);
      augmentedTreeWhichIterationValue =
        createSuperarcsData.BaseTreeWhichIteration.Get(oldSupernodeId);
      // and set the relevant regular arrays
      augmentedTreeRegularNodeGlobalIdsValue = globalRegularIdSetValue;
      augmentedTreeDataValuesValue = dataValueSetValue;
      // for the root, these always point to itself
      augmentedTreeRegular2SupernodeValue = newSupernodeId;
      augmentedTreeSuperparentsValue = newSupernodeId;
    } // root of the tree
    // now deal with unsimplified attachment points, which we can identify because they were in the "kept" batch, not the "inserted" batch,
    // and this is given away by the index into the set of supernodes to be added
    // and the fact that the superarc is NO_SUCH_ELEMENT
    else if ((supernodeSetIndex >= this->NumInsertedSupernodes) &&
             (viskores::worklet::contourtree_augmented::NoSuchElement(
               createSuperarcsData.BaseTreeSuperarcs.Get(oldSupernodeId))))
    { // preserved attachment point
      // note that oldSupernodeID is guaranteed not to be NO_SUCH_ELEMENT, as the supernode came from this block originally
      // set the superarc to NO_SUCH_ELEMENT, as before
      augmentedTreeSuperarcsValue = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
      // hyperstructure carries over, so we use the same hyperparent as before
      // the "if" clauses guarantee the oldSupernodeId not to be NO_SUCH_ELEMENT.
      // We cannot prepare the array permutation outside the worklet, or the guarantee does not hold.
      augmentedTreeHyperparentsValue = createSuperarcsData.BaseTreeHyperparents.Get(oldSupernodeId);
      // attachment points are never hypernodes anyway, so set it directly
      augmentedTreeSuper2HypernodeValue = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
      // and the round and iteration
      augmentedTreeWhichRoundValue = createSuperarcsData.BaseTreeWhichRound.Get(oldSupernodeId);
      augmentedTreeWhichIterationValue =
        createSuperarcsData.BaseTreeWhichIteration.Get(oldSupernodeId);
      // and set the relevant regular arrays
      augmentedTreeRegularNodeGlobalIdsValue = globalRegularIdSetValue;
      augmentedTreeDataValuesValue = dataValueSetValue;
      // for a preserved attachment point, this always points to itself
      augmentedTreeRegular2SupernodeValue = newSupernodeId;
      // the superparent is the tricky one, as the old one may have been broken up by insertions at a higher level

      // Here, what we need to do is a search in the augmented tree to find which superarc to attach to.  This is necessary
      // because the old superarc it attached to may have been broken up.
      // We are guaranteed that there is one, and that it only uses the higher levels of the augmented tree,
      // so the fact that we are partially constructed doesn't get in the way.  To do this, we need supernodes
      // known to be in the higher level that are above and below the supernode.
      // Since the point was an attachment point in the base tree, that means that there is a higher round superarc
      // it inserts into.  Moreover, the algorithm ALWAYS inserts a supernode at or above its original round, so
      // we can guarantee that both ends of the parent are in the higher levels.  Which means we only need to work
      // out which end is higher.

      // the "if" clauses guarantee the oldSupernodeId not to be NO_SUCH_ELEMENT.
      // However, we cannot prepare the array permutation outside the worklet, or the guarantee does not hold.
      //      indexType oldRegularID = baseTree->supernodes[oldSupernodeID];
      //      indexType oldSuperFrom = baseTree->superparents[oldRegularID];
      //      indexType oldSuperTo = baseTree->superarcs[oldSuperFrom];
      viskores::Id oldRegularId = createSuperarcsData.BaseTreeSupernodes.Get(oldSupernodeId);
      viskores::Id oldSuperFromValue = createSuperarcsData.BaseTreeSuperparents.Get(oldRegularId);
      viskores::Id oldSuperToValue = createSuperarcsData.BaseTreeSuperarcs.Get(oldSuperFromValue);

      // retrieve the ascending flag
      bool ascendingSuperarc =
        viskores::worklet::contourtree_augmented::IsAscending(oldSuperToValue);
      // and mask out the flags
      viskores::Id oldSuperToMaskedIndex =
        viskores::worklet::contourtree_augmented::MaskedIndex(oldSuperToValue);

      // since we haven't set up the regular search array yet, we can't use that
      // instead, we know that the two supernodes must be in the new tree, so we retrieve their new super IDs
      // and convert them to regular

      // retrieve their new super IDs
      viskores::Id newSuperFrom = createSuperarcsData.NewSupernodeIds.Get(oldSuperFromValue);
      viskores::Id newSuperTo = createSuperarcsData.NewSupernodeIds.Get(oldSuperToMaskedIndex);

      // convert to regular IDs (which is what the FindSuperArcForUnknownNode() routine assumes)
      viskores::Id newRegularFrom = augmentedTreeSupernodesPortal.Get(newSuperFrom);
      viskores::Id newRegularTo = augmentedTreeSupernodesPortal.Get(newSuperTo);

      // the new superparent after the search
      viskores::Id newSuperparentId = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;

      // depending on the ascending flag
      if (ascendingSuperarc)
      {
        newSuperparentId = findSuperArcForUnknownNode.FindSuperArcForUnknownNode(
          globalRegularIdSetValue, dataValueSetValue, newRegularTo, newRegularFrom);
      }
      else
      {
        newSuperparentId = findSuperArcForUnknownNode.FindSuperArcForUnknownNode(
          globalRegularIdSetValue, dataValueSetValue, newRegularFrom, newRegularTo);
      }

      // attachment points use the superparent to store the superarc they insert onto
      augmentedTreeSuperparentsValue = newSuperparentId;

    } // preserved attachment point
    else
    { // raised attachment point or "ordinary" supernodes
      // Since all of the superparents must be in the base tree, we can now retrieve the target
      viskores::Id superparentOldSuperId = viskores::worklet::contourtree_augmented::MaskedIndex(
        createSuperarcsData.SuperparentSet.Get(supernodeSetIndex));

      viskores::Id oldTargetSuperId =
        createSuperarcsData.BaseTreeSuperarcs.Get(superparentOldSuperId);

      // and break it into a target and flags
      bool ascendingSuperarc =
        viskores::worklet::contourtree_augmented::IsAscending(oldTargetSuperId);
      // NOTE: if the target was NO_SUCH_ELEMENT, this will hold 0
      oldTargetSuperId = viskores::worklet::contourtree_augmented::MaskedIndex(oldTargetSuperId);

      // and another boolean for whether we are the last element in a segment
      bool isLastInSegment = false;

      // end of the entire array counts as last in segment
      if (supernode == supernodeSorterPortal.GetNumberOfValues() - 1)
      {
        isLastInSegment = true;
      }
      // otherwise, check for a mismatch in the sorting superparent which indicates the end of a segment
      else if (viskores::worklet::contourtree_augmented::MaskedIndex(
                 createSuperarcsData.SuperparentSet.Get(supernodeSetIndex)) !=
               viskores::worklet::contourtree_augmented::MaskedIndex(
                 createSuperarcsData.SuperparentSet.Get(supernodeSorterPortal.Get(supernode + 1))))
      {
        isLastInSegment = true;
      }

      // setting the superarc is done the usual way.  Our sort routine has ended up with the supernodes arranged in either ascending or descending order
      // inwards along the parent superarc (as expressed by the superparent ID).  Each superarc except the last in the segment points to the next one:
      // the last one points to the target of the original superarc.
      if (isLastInSegment)
      { // last in segment
        // we take the old target of the superarc (in old supernode IDs) and convert it to a new supernode ID
        augmentedTreeSuperarcsValue = createSuperarcsData.NewSupernodeIds.Get(oldTargetSuperId) |
          (ascendingSuperarc ? viskores::worklet::contourtree_augmented::IS_ASCENDING : 0x00);
      } // last in segment
      else
      { // not last in segment
        // the target is always the next one, so just store it with the ascending flag
        augmentedTreeSuperarcsValue = (newSupernodeId + 1) |
          (ascendingSuperarc ? viskores::worklet::contourtree_augmented::IS_ASCENDING : 0x00);
      } // not last in segment

      // first we identify the hyperarc on which the superarc sits
      // this will be visible in the old base tree, since hyperstructure carries over
      viskores::Id oldHyperparent =
        createSuperarcsData.BaseTreeHyperparents.Get(superparentOldSuperId);

      // hyperstructure carries over, so we use the same hyperparent as the superparent
      augmentedTreeHyperparentsValue = oldHyperparent;

      // retrieve the hyperparent's old supernode ID & convert to a new one, then test it
      if (createSuperarcsData.NewSupernodeIds.Get(
            createSuperarcsData.BaseTreeHypernodes.Get(oldHyperparent)) == newSupernodeId)
      {
        augmentedTreeSuper2HypernodeValue = oldHyperparent;
      }
      else
      {
        augmentedTreeSuper2HypernodeValue =
          viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
      }

      // round and iteration are set from the superparent, since we are raising to its level
      augmentedTreeWhichRoundValue =
        createSuperarcsData.BaseTreeWhichRound.Get(superparentOldSuperId);
      augmentedTreeWhichIterationValue =
        createSuperarcsData.BaseTreeWhichIteration.Get(superparentOldSuperId);
      // and set the relevant regular arrays
      augmentedTreeRegularNodeGlobalIdsValue = globalRegularIdSetValue;
      augmentedTreeDataValuesValue = dataValueSetValue;
      // for all supernodes, this points to itself
      augmentedTreeRegular2SupernodeValue = newSupernodeId;
      // and since we're inserted, so does this
      augmentedTreeSuperparentsValue = newSupernodeId;
    } // raised attachment point or "ordinary" supernodes

    /*  Original PPP2 code
    for (indexType supernode = 0; supernode < supernodeSorter.size(); supernode++)
        { // per supernode in the set
        // retrieve the index from the sorting index array
        indexType supernodeSetIndex = supernodeSorter[supernode];

        // work out the new supernode ID
        indexType newSupernodeID = nSupernodesAlready + supernode;

        // and the old supernode ID
        // NB: May be NO_SUCH_ELEMENT if not already in the base tree
        indexType oldSupernodeID = supernodeIDSet[supernodeSetIndex];

        //	At all levels above 0, we used to keep regular vertices in case they are attachment points.  After augmentation, we don't need to.
        //	Instead, at all levels above 0, the regular nodes in each round are identical to the supernodes
        //	In order to avoid confusion, we will copy the ID into a separate variable
        indexType newRegularID = newSupernodeID;

        // setting the supernode's regular ID is now trivial
        augmentedTree->supernodes			[newSupernodeID] = newRegularID;

        // retrieve the old superID of the superparent.  This is slightly tricky, as we have four classes of supernodes:
        // 1. the root of the entire tree
        // 2. attachment points not being inserted. In this case, the supernode ID is stored in the superparentSet array, not the superparent for insertion purposes
        // 3. attachment points being inserted.  In this case, the superparent is stored in the superparentSet array
        // 4. "ordinary" supernodes, where the superparent is the same as the supernode ID anyway
        //
        // Note that an attachment point gets inserted into a parent superarc.  But the attachment point itself has a NULL superarc, because it's only a virtual insertion
        // This means that such an attachment superarc cannot be the superparent of any other attachment point
        // It is therefore reasonable to deal with 1. & 2 separately. 3. & 4. then combine together

        // first we test for the root of the tree
        if ((roundNo == baseTree->nRounds) && (supernode == supernodeSorter.size() - 1))
            { // root of the tree
            // note that oldSupernodeID is guaranteed not to be NO_SUCH_ELEMENT, as the root is in every tree
            // set the super arrays
            augmentedTree->superarcs			[newSupernodeID]	= NO_SUCH_ELEMENT;
            // hyperstructure carries over, so we use the same hyperparent as before
            augmentedTree->hyperparents			[newSupernodeID]	= baseTree->hyperparents[oldSupernodeID];
            // and set the hypernode ID
            augmentedTree->super2hypernode		[newSupernodeID]	= baseTree->super2hypernode[oldSupernodeID];
            // and the round and iteration
            augmentedTree->whichRound			[newSupernodeID]	= baseTree->whichRound[oldSupernodeID];
            augmentedTree->whichIteration		[newSupernodeID]	= baseTree->whichIteration[oldSupernodeID];
            // and set the relevant regular arrays
            augmentedTree->regularNodeGlobalIDs	[newRegularID]		= globalRegularIDSet[supernodeSetIndex];
            augmentedTree->dataValues			[newRegularID]		= dataValueSet[supernodeSetIndex];
            // for the root, these always point to itself
            augmentedTree->regular2supernode	[newRegularID]		= newSupernodeID;
            augmentedTree->superparents			[newRegularID]		= newSupernodeID;
            } // root of the tree
        // now deal with unsimplified attachment points, which we can identify because they were in the "kept" batch, not the "inserted" batch,
        // and this is given away by the index into the set of supernodes to be added
        // and the fact that the superarc is NO_SUCH_ELEMENT
        else if	((supernodeSetIndex >= nInsertedSupernodes) && (noSuchElement(baseTree->superarcs[oldSupernodeID])))
            { // preserved attachment point
            // note that oldSupernodeID is guaranteed not to be NO_SUCH_ELEMENT, as the supernode came from this block originally
            // set the superarc to NO_SUCH_ELEMENT, as before
            augmentedTree->superarcs			[newSupernodeID]	= NO_SUCH_ELEMENT;
            // hyperstructure carries over, so we use the same hyperparent as before
            augmentedTree->hyperparents			[newSupernodeID]	= baseTree->hyperparents[oldSupernodeID];
            // attachment points are never hypernodes anyway, so set it directly
            augmentedTree->super2hypernode		[newSupernodeID]	= NO_SUCH_ELEMENT;
            // and the round and iteration
            augmentedTree->whichRound			[newSupernodeID]	= baseTree->whichRound[oldSupernodeID];
            augmentedTree->whichIteration		[newSupernodeID]	= baseTree->whichIteration[oldSupernodeID];
            // and set the relevant regular arrays
            augmentedTree->regularNodeGlobalIDs	[newRegularID]		= globalRegularIDSet[supernodeSetIndex];
            augmentedTree->dataValues			[newRegularID]		= dataValueSet[supernodeSetIndex];
            // for a preserved attachment point, this always points to itself
            augmentedTree->regular2supernode	[newRegularID]		= newSupernodeID;
            // the superparent is the tricky one, as the old one may have been broken up by insertions at a higher level

            // Here, what we need to do is a search in the augmented tree to find which superarc to attach to.  This is necessary
            // because the old superarc it attached to may have been broken up.
            // We are guaranteed that there is one, and that it only uses the higher levels of the augmented tree,
            // so the fact that we are partially constructed doesn't get in the way.  To do this, we need supernodes
            // known to be in the higher level that are above and below the supernode.
            // Since the point was an attachment point in the base tree, that means that there is a higher round superarc
            // it inserts into.  Moreover, the algorithm ALWAYS inserts a supernode at or above its original round, so
            // we can guarantee that both ends of the parent are in the higher levels.  Which means we only need to work
            // out which end is higher.
            indexType oldRegularID = baseTree->supernodes[oldSupernodeID];
            indexType oldSuperFrom = baseTree->superparents[oldRegularID];
            indexType oldSuperTo = baseTree->superarcs[oldSuperFrom];
            // retrieve the ascending flag
            bool ascendingSuperarc = isAscending(oldSuperTo);
            // and mask out the flags
            oldSuperTo = maskedIndex(oldSuperTo);

            // since we haven't set up the regular search array yet, we can't use that
            // instead, we know that the two supernodes must be in the new tree, so we retrieve their new super IDs
            // and convert them to regular

            // retrieve their new super IDs
            indexType newSuperFrom = newSupernodeIDs[oldSuperFrom];
            indexType newSuperTo = newSupernodeIDs[oldSuperTo];

            // convert to regular IDs (which is what the FindSuperArcForUnknownNode() routine assumes)
            indexType newRegularFrom = augmentedTree->supernodes[newSuperFrom];
            indexType newRegularTo = augmentedTree->supernodes[newSuperTo];

            // the new superparent after the search
            indexType newSuperparentID = NO_SUCH_ELEMENT;

            // depending on the ascending flag
            if (ascendingSuperarc)
                newSuperparentID = augmentedTree->FindSuperArcForUnknownNode(globalRegularIDSet[supernodeSetIndex],dataValueSet[supernodeSetIndex], newRegularTo, newRegularFrom);
            else
                newSuperparentID = augmentedTree->FindSuperArcForUnknownNode(globalRegularIDSet[supernodeSetIndex],dataValueSet[supernodeSetIndex], newRegularFrom, newRegularTo);

            // attachment points use the superparent to store the superarc they insert onto
            augmentedTree->superparents			[newRegularID] 		= newSuperparentID;

            } // preserved attachment point
        else
            { // raised attachment point or "ordinary" supernodes
            // Since all of the superparents must be in the base tree, we can now retrieve the target
            indexType superparentOldSuperID = maskedIndex(superparentSet[supernodeSetIndex]);
            indexType oldTargetSuperID = baseTree->superarcs[superparentOldSuperID];
            // and break it into a target and flags
            bool ascendingSuperarc = isAscending(oldTargetSuperID);
            // NOTE: if the target was NO_SUCH_ELEMENT, this will hold 0
            oldTargetSuperID = maskedIndex(oldTargetSuperID);

            // and another boolean for whether we are the last element in a segment
            bool isLastInSegment = false;

            // end of the entire array counts as last in segment
            if (supernode == supernodeSorter.size() - 1)
                isLastInSegment = true;
            // otherwise, check for a mismatch in the sorting superparent which indicates the end of a segment
            else if (maskedIndex(superparentSet[supernodeSetIndex]) != maskedIndex(superparentSet[supernodeSorter[supernode+1]]))
                isLastInSegment = true;

            // setting the superarc is done the usual way.  Our sort routine has ended up with the supernodes arranged in either ascending or descending order
            // inwards along the parent superarc (as expressed by the superparent ID).  Each superarc except the last in the segment points to the next one:
            // the last one points to the target of the original superarc.
            if (isLastInSegment)
                { // last in segment
                // we take the old target of the superarc (in old supernode IDs) and convert it to a new supernode ID
                augmentedTree->superarcs[newSupernodeID] = newSupernodeIDs[oldTargetSuperID] | (ascendingSuperarc ? IS_ASCENDING : 0x00);
                } // last in segment
            else
                { // not last in segment
                // the target is always the next one, so just store it with the ascending flag
                augmentedTree->superarcs[newSupernodeID] = (newSupernodeID+1) | (ascendingSuperarc ? IS_ASCENDING : 0x00);
                } // not last in segment

            // first we identify the hyperarc on which the superarc sits
            // this will be visible in the old base tree, since hyperstructure carries over
            indexType oldHyperparent = baseTree->hyperparents[superparentOldSuperID];

            // hyperstructure carries over, so we use the same hyperparent as the superparent
            augmentedTree->hyperparents			[newSupernodeID]	= oldHyperparent;
            // retrieve the hyperparent's old supernode ID & convert to a new one, then test it
            if (newSupernodeIDs[baseTree->hypernodes[oldHyperparent]] == newSupernodeID)
                augmentedTree->super2hypernode	[newSupernodeID]	= oldHyperparent;
            else
                augmentedTree->super2hypernode	[newSupernodeID]	= NO_SUCH_ELEMENT;

            // round and iteration are set from the superparent, since we are raising to its level
            augmentedTree->whichRound			[newSupernodeID]	= baseTree->whichRound[superparentOldSuperID];
            augmentedTree->whichIteration		[newSupernodeID]	= baseTree->whichIteration[superparentOldSuperID];
            // and set the relevant regular arrays
            augmentedTree->regularNodeGlobalIDs	[newRegularID]		= globalRegularIDSet[supernodeSetIndex];
            augmentedTree->dataValues			[newRegularID]		= dataValueSet[supernodeSetIndex];
            // for all supernodes, this points to itself
            augmentedTree->regular2supernode	[newRegularID]		= newSupernodeID;
            // and since we're inserted, so does this
            augmentedTree->superparents			[newRegularID]		= newSupernodeID;
            } // raised attachment point or "ordinary" supernodes

        } // per supernode in the set
    */


  } // operator()()

private:
  const viskores::Id NumSupernodesAlready;
  const viskores::Id BaseTreeNumRounds;
  const viskores::Id NumInsertedSupernodes;
  const viskores::Id RoundNo;

}; // CreateSuperarcsWorklet

} // namespace hierarchical_augmenter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
