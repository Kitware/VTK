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

#ifndef viskores_worklet_contourtree_distributed_bract_maker_propagate_boundary_counts_compute_group_totals_worklet_h
#define viskores_worklet_contourtree_distributed_bract_maker_propagate_boundary_counts_compute_group_totals_worklet_h

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

/// Worklet to subtract the beginning of the group to get the total for each group
/// Part of the BoundaryRestrictedAugmentedContourTree.PropagateBoundaryCounts function
///
/// 5. Finally, we subtract the beginning of the group to get the total for each group
class PropagateBoundaryCountsComputeGroupTotalsWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(
    WholeArrayIn hyperarcTargetSortPermutation,          // (input)
    WholeArrayIn hyperarcs,                              // (input) contour tree hyperarcs
    WholeArrayIn accumulatedBoundaryCountPortal,         // (input)
    WholeArrayInOut supernodeTransferBoundaryCountPortal // (input/output)
  );
  using ExecutionSignature = void(InputIndex, _1, _2, _3, _4);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  PropagateBoundaryCountsComputeGroupTotalsWorklet() {}

  template <typename InFieldPortalType, typename InOutFieldPortalType>
  VISKORES_EXEC void operator()(
    const viskores::Id& hyperarc,
    const InFieldPortalType& hyperarcTargetSortPermutationPortal,
    const InFieldPortalType& hyperarcsPortal,
    const InFieldPortalType& accumulatedBoundaryCountPortal,
    const InOutFieldPortalType& supernodeTransferBoundaryCountPortal) const
  {
    // the prefix sum of the first element is the correct value anyway so we can ignore it
    if (hyperarc == 0)
    {
      return;
    }
    // retrieve the hypertarget & the previous one as well
    viskores::Id hyperarcTarget = viskores::worklet::contourtree_augmented::MaskedIndex(
      hyperarcsPortal.Get(hyperarcTargetSortPermutationPortal.Get(hyperarc)));
    viskores::Id prevHyperarcTarget = viskores::worklet::contourtree_augmented::MaskedIndex(
      hyperarcsPortal.Get(hyperarcTargetSortPermutationPortal.Get(hyperarc - 1)));

    // if they differ, write
    if (hyperarcTarget != prevHyperarcTarget)
    { // they're different
      // if it's not the last one, transfer it to the supernode at the far end
      if (!viskores::worklet::contourtree_augmented::NoSuchElement(hyperarcTarget))
      { // valid target
        supernodeTransferBoundaryCountPortal.Set(
          hyperarcTarget,
          supernodeTransferBoundaryCountPortal.Get(hyperarcTarget) -
            accumulatedBoundaryCountPortal.Get(hyperarc - 1));
      } // valid target
    }   // they're different

    // In serial this worklet implements the following operation
    /*
    // NOTE: in contrast to the original imlementation this worklet starts the iteration at
    //       0 for simplicity, hence, we need to check this case separately here
    for (viskores::Id hyperarc = 1; hyperarc < hyperarcTargetSortPermutation.size(); hyperarc++)
    { // per hyperarc
        // retrieve the hypertarget & the previous one as well
        viskores::Id hyperarcTarget = maskedIndex(contourTree->hyperarcs[hyperarcTargetSortPermutation[hyperarc]]);
        viskores::Id prevHyperarcTarget = maskedIndex(contourTree->hyperarcs[hyperarcTargetSortPermutation[hyperarc-1]]);

        // if they differ, write
        if (hyperarcTarget != prevHyperarcTarget)
            { // they're different
            // if it's not the last one, transfer it to the supernode at the far end
            if (! noSuchElement(hyperarcTarget))
                    { // valid target
                    supernodeTransferBoundaryCount[hyperarcTarget] -= accumulatedBoundaryCount[hyperarc-1];
                    } // valid target
            } // they're different
    } // per hyperarc
    */
  }
}; // PropagateBoundaryCountsComputeGroupTotalsWorklet


} // namespace bract_maker
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
