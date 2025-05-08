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

#ifndef viskores_worklet_contourtree_distributed_hierarchical_augmenter_update_hyperstructure_set_superchildren_worklet_h
#define viskores_worklet_contourtree_distributed_hierarchical_augmenter_update_hyperstructure_set_superchildren_worklet_h

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

/// Worklet used in HierarchicalAugmenter::UpdateHyperstructure to set the superchildren
/// The worklet  finds the number of superchildren as the delta between the super Id
/// and the next hypernode's super Id
class UpdateHyperstructureSetSuperchildrenWorklet : public viskores::worklet::WorkletMapField
{
public:
  /// Control signature for the worklet
  using ControlSignature = void(
    WholeArrayIn augmentedTreeHypernodes,      // input (we need both this and the next value)
    FieldIn augmentedTreeSuperarcs,            // input
    WholeArrayIn augmentedTreeHyperparents,    // input
    WholeArrayInOut augmentedTreeSuperchildren // output
  );
  using ExecutionSignature = void(InputIndex, _1, _2, _3, _4);
  using InputDomain = _2;

  // Default Constructor
  VISKORES_EXEC_CONT
  UpdateHyperstructureSetSuperchildrenWorklet(const viskores::Id& augmentedTreeNumSupernodes,
                                              const viskores::Id& supernodeStartIndex)
    : AugmentedTreeNumSupernodes(augmentedTreeNumSupernodes)
    , AugmentedTreeSupernodeStartIndex(supernodeStartIndex)
  {
  }


  template <typename InFieldPortalType1, typename InFieldPortalType2, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& supernode,
                                const InFieldPortalType1& augmentedTreeHypernodesPortal,
                                const viskores::Id& augmentedTreeSuperarcsValue,
                                const InFieldPortalType2& augmentedTreeHyperparentsPortal,
                                const OutFieldPortalType& augmentedTreeSuperchildrenPortal) const
  {
    // per supernode
    // attachment points have NULL superarcs and are skipped
    if (viskores::worklet::contourtree_augmented::NoSuchElement(augmentedTreeSuperarcsValue))
    {
      return;
    }
    // we are now guaranteed to have a valid hyperparent
    viskores::Id hyperparent = augmentedTreeHyperparentsPortal.Get(supernode);
    viskores::Id hyperparentSuperId = augmentedTreeHypernodesPortal.Get(hyperparent);

    // we could be at the end of the array, so test explicitly
    if (this->AugmentedTreeSupernodeStartIndex + supernode == this->AugmentedTreeNumSupernodes - 1)
    {
      // this means that we are the end of the segment and can subtract the hyperparent's super ID to get the number of superchildren
      augmentedTreeSuperchildrenPortal.Set(hyperparent,
                                           this->AugmentedTreeNumSupernodes - hyperparentSuperId);
    }
    // otherwise, if our hyperparent is different from our neighbor's, we are the end of the segment
    else if (hyperparent != augmentedTreeHyperparentsPortal.Get(supernode + 1))
    {
      // again, subtract to get the number
      augmentedTreeSuperchildrenPortal.Set(
        hyperparent, supernode + this->AugmentedTreeSupernodeStartIndex + 1 - hyperparentSuperId);
    }
    // per supernode
    // In serial this worklet implements the following operation
    /*
      for (indexType supernode = augmentedTree->firstSupernodePerIteration[roundNo][0]; supernode < augmentedTree->firstSupernodePerIteration[roundNo][augmentedTree->nIterations[roundNo]]; supernode++)
      { // per supernode
        // attachment points have NULL superarcs and are skipped
       if (noSuchElement(augmentedTree->superarcs[supernode]))
          continue;
       // we are now guaranteed to have a valid hyperparent
       indexType hyperparent = augmentedTree->hyperparents[supernode];
       indexType hyperparentSuperID = augmentedTree->hypernodes[hyperparent];

       // we could be at the end of the array, so test explicitly
      if (supernode == augmentedTree->supernodes.size() - 1)
        // this means that we are the end of the segment and can subtract the hyperparent's super ID to get the number of superchildren
        augmentedTree->superchildren[hyperparent] = augmentedTree->supernodes.size() - hyperparentSuperID;
      // otherwise, if our hyperparent is different from our neighbor's, we are the end of the segment
      else if (hyperparent != augmentedTree->hyperparents[supernode + 1])
        // again, subtract to get the number
        augmentedTree->superchildren[hyperparent] = supernode + 1 - hyperparentSuperID;
      } // per supernode
    */
  } // operator()()

private:
  const viskores::Id AugmentedTreeNumSupernodes;
  const viskores::Id AugmentedTreeSupernodeStartIndex;
}; // UpdateHyperstructureSetSuperchildrenWorklet

} // namespace hierarchical_augmenter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
