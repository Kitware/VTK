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

#ifndef viskores_worklet_contourtree_distributed_hierarchical_augmenter_copy_base_regular_structure_worklet_h
#define viskores_worklet_contourtree_distributed_hierarchical_augmenter_copy_base_regular_structure_worklet_h

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

/// Worklet used in HierarchicalAugmenter::CopyBaseRegularStructure for
/// finding the superparent for each node needed
class CopyBaseRegularStructureWorklet : public viskores::worklet::WorkletMapField
{
public:
  /// Control signature for the worklet
  /// NOTE: We require the input arrays (aside form the input domain) to be permutted by the
  ///       regularNodesNeeded input domain so that we can use FieldIn instead of WholeArrayIn
  /// NOTE: We require ArrayHandleView for the output arrays of the range [numExistingRegular:end] so
  ///       that we can use FieldOut instead of requiring WholeArrayInOut
  using ControlSignature = void(
    FieldIn
      regularNodesNeededRange, // input domain ArrayHandleIndex of [0, regularNodesNeeded.GetNumberOfValues()]
    FieldIn
      baseTreeRegularNodeGlobalIdsPermuted, // input baseTree->regularNodeGlobalIds permuted by regularNodesNeeded
    FieldIn baseTreeDataValuesPermuted, // input baseTree->dataValues permuted by regularNodesNeeded
    FieldIn regularSuperparentsPermuted, // input regularSuperparents permuted by regularNodesNeeded
    FieldOut
      augmentedTreeRegularNodeGlobalIdsView, // output view of augmentedTree->regularNodeGlobalIds[numExistingRegular:]
    FieldOut
      augmentedTreeDataValuesView, // output view of augmentedTree->dataValues[numExistingRegular:]
    FieldOut
      augmentedTreeSuperparentsView, // output view of augmentedTree->superparents[numExistingRegular:]
    FieldOut
      augmentedTreeRegularNodeSortOrderView // output view of  augmentedTree->regularNodeSortOrder[numExistingRegular:]
  );
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8);
  using InputDomain = _1;

  /// Default Constructor
  VISKORES_EXEC_CONT
  CopyBaseRegularStructureWorklet(const viskores::Id& numExistingRegular)
    : NumExistingRegular(numExistingRegular)
  {
  }

  /// operator() of the workelt
  template <typename FieldType>
  VISKORES_EXEC void operator()(
    const viskores::Id& neededRegNode, // InputIndex in [0, regularNodesNeeded.GetNumberOfValues()]
    const viskores::Id&
      baseTreeRegularNodeGlobalId,      // same as baseTree->regularNodeGlobalIds[oldRegularID]
    const FieldType& baseTreeDataValue, // same as baseTree->dataValues[oldRegularID]
    const viskores::Id& regularSuperparentsValue, // same regularSuperparents[oldRegularID]
    viskores::Id&
      augmentedTreeRegularNodeGlobalIdValue, // same as augmentedTree->regularNodeGlobalIDs[NumExistingRegular + neededRegNode] = ...
    FieldType&
      augmentedTreeDataValue, // same as augmentedTree->dataValues[NumExistingRegular + neededRegNode] = ...
    viskores::Id&
      augmentedTreeSuperparentsValue, // same as augmentedTree->superparents[NumExistingRegular + neededRegNode] = ...
    viskores::Id&
      augmentedTreeRegularNodeSortOrderValue // same as augmentedTree->regularNodeSortOrder [NumExistingRegular + neededRegNode] = ...
  ) const
  {
    // per regular node needing addition
    // retrieve the existing index. oldRegularID is set on input
    viskores::Id newRegularId = this->NumExistingRegular +
      neededRegNode; // not needed since we do ArrayHandleViews on the outside

    // now use them to copy data
    augmentedTreeRegularNodeGlobalIdValue = baseTreeRegularNodeGlobalId;
    augmentedTreeDataValue = baseTreeDataValue;
    augmentedTreeSuperparentsValue = regularSuperparentsValue;

    // this one is special since we need to resort - set it to identity, leaving the sort order of old vertices alone
    // this *MAY* make certain sorts run faster
    augmentedTreeRegularNodeSortOrderValue = newRegularId;

    // NOTE: we can skip this step since Regular2supernode is already initalized with NO_SUCH_ELEMENT
    // since these are *ALL* only regular nodes, this one's easy:
    // augmentedTree->regular2supernode  [newRegularID]    =  NO_SUCH_ELEMENT;

    // In serial this worklet implements the following operation
    /*
    for (viskores::Id neededRegNode = 0; neededRegNode < nRegNeeded; neededRegNode++)
    { // per regular node needing addition
      // retrieve the existing index
      viskores::Id oldRegularID = regularNodesNeeded[neededRegNode];
      // and compute the new index
      viskores::Id newRegularID = nExistingRegular + neededRegNode;

      // now use them to copy data
      augmentedTree->regularNodeGlobalIDs  [newRegularID]    =  baseTree->regularNodeGlobalIDs  [oldRegularID];
      augmentedTree->dataValues      [newRegularID]    =  baseTree->dataValues      [oldRegularID];
      augmentedTree->superparents      [newRegularID]    =  regularSuperparents        [oldRegularID];

      // this one is special since we need to resort - set it to identity, leaving the sort order of old vertices alone
      // this *MAY* make certain sorts run faster
      augmentedTree->regularNodeSortOrder  [newRegularID]    =  newRegularID;

      // since these are *ALL* only regular nodes, this one's easy:
      augmentedTree->regular2supernode  [newRegularID]    =  NO_SUCH_ELEMENT;
    } // per regular node needing addition
    */
  } // operator()()
private:
  const viskores::Id NumExistingRegular;

}; // CopyBaseRegularStructureWorklet

} // namespace hierarchical_augmenter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
