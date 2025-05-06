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

#ifndef viskores_worklet_contourtree_distributed_hierarchical_hyper_sweeper_transfer_weights_update_rhe_worklet_round2_h
#define viskores_worklet_contourtree_distributed_hierarchical_hyper_sweeper_transfer_weights_update_rhe_worklet_round2_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{
namespace hierarchical_hyper_sweeper
{

/// Worklet used in HierarchicalHyperSweeper.TransferWeights(...) to implement
/// step 7a. Find the RHE of each group and transfer the prefix sum weight.
/// Note that we do not compute the transfer weight separately, we add it in place instead
class TransferWeightsUpdateRHEWorkletRound2 : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature =
    void(FieldIn supernodeIndex, // input counting array [firstSupernode, lastSupernode)
         WholeArrayIn sortedTransferTarget,
         FieldIn valuePrefixSumView, // input view of valuePrefixSum[firstSupernode, lastSupernode)
         WholeArrayInOut intrinsicValuesPortal,
         WholeArrayInOut dependentValuesPortal);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);

  // Default Constructor
  VISKORES_EXEC_CONT
  TransferWeightsUpdateRHEWorkletRound2(const viskores::Id& lastSupernode)
    : LastSupernode(lastSupernode)
  {
  }

  template <typename InPortalType, typename OutPortalType>
  VISKORES_EXEC void operator()(
    const viskores::Id& supernode,
    const InPortalType& sortedTransferTargetPortal,
    const viskores::Id& valuePrefixSum, // same as valuePrefixSum[supernode]
    OutPortalType& intrinsicValuesPortal,
    OutPortalType& dependentValuesPortal) const
  {
    // per supernode
    // ignore any that point at NO_SUCH_ELEMENT
    viskores::Id transferTarget = sortedTransferTargetPortal.Get(supernode);
    if (!viskores::worklet::contourtree_augmented::NoSuchElement(transferTarget))
    {
      // the RHE of each segment transfers its weight (including all irrelevant prefixes)
      if ((supernode == this->LastSupernode - 1) ||
          (transferTarget != sortedTransferTargetPortal.Get(supernode + 1)))
      { // RHE of segment
        // we need to separate out the flag for attachment points
        bool superarcTransfer =
          viskores::worklet::contourtree_augmented::TransferToSuperarc(transferTarget);
        viskores::Id superarcOrNodeId =
          viskores::worklet::contourtree_augmented::MaskedIndex(transferTarget);
        // we ignore attachment points
        if (!superarcTransfer)
        {
          return;
        }
        // we modify both intrinsic and dependent values
        auto originalIntrinsicValue = intrinsicValuesPortal.Get(superarcOrNodeId);
        intrinsicValuesPortal.Set(superarcOrNodeId, originalIntrinsicValue + valuePrefixSum);
        auto originalDependentValue = dependentValuesPortal.Get(superarcOrNodeId);
        dependentValuesPortal.Set(superarcOrNodeId, originalDependentValue + valuePrefixSum);
      } // RHE of segment
    }

    // In serial this worklet implements the following operation
    /*
    for (indexType supernode = firstSupernode; supernode < lastSupernode; supernode++)
    { // per supernode
      // ignore any that point at NO_SUCH_ELEMENT
      if (noSuchElement(sortedTransferTarget[supernode]))
        continue;

      // the RHE of each segment transfers its weight (including all irrelevant prefixes)
      if ((supernode == lastSupernode - 1) || (sortedTransferTarget[supernode] != sortedTransferTarget[supernode+1]))
      { // RHE of segment
        // we need to separate out the flag for attachment points
        bool superarcTransfer = transferToSuperarc(sortedTransferTarget[supernode]);
        indexType superarcOrNodeID = maskedIndex(sortedTransferTarget[supernode]);
        // ignore the transfers for non-attachment points
        if (!superarcTransfer)
          continue;

        // we modify both intrinsic and dependent values
        intrinsicValues[superarcOrNodeID] += valuePrefixSum[supernode];
        dependentValues[superarcOrNodeID] += valuePrefixSum[supernode];
      } // RHE of segment

    } // per supernode
    */
  } // operator()()

private:
  const viskores::Id LastSupernode;

}; // TransferWeightsUpdateRHEWorklet

} // namespace hierarchical_hyper_sweeper
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
