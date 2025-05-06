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

#ifndef viskores_worklet_contourtree_distributed_tree_grafter_copy_new_hypernodes_worklet_h
#define viskores_worklet_contourtree_distributed_tree_grafter_copy_new_hypernodes_worklet_h


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

// Worklet implementing the inner parallel loop to collaps the regular chains in TreeGrafter.CollapseRegularChains
class CopyNewHypernodesWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn newHypernodes,                    // input iteration index.
                                WholeArrayIn hierarchicalSuperId,         // input
                                WholeArrayIn hierarchicalHyperarc,        // input
                                WholeArrayOut hierarchicalTreeHypernodes, // output
                                WholeArrayOut hierarchicalTreeHyperarcs   // output
  );

  using ExecutionSignature = void(InputIndex, _1, _2, _3, _4, _5);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  CopyNewHypernodesWorklet(viskores::Id numOldHypernodes)
    : NumOldHypernodes(numOldHypernodes)
  {
  }

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(
    const viskores::Id& newHypernode, // position in newHypernodes arrays
    const viskores::Id&
      oldSupernodeId, // old hypernode ID & super ID, i.e., newHypernodes[newHypernode]
    const InFieldPortalType& hierarchicalSuperIdPortal,         // input
    const InFieldPortalType& hierarchicalHyperarcPortal,        // input
    const OutFieldPortalType& hierarchicalTreeHypernodesPortal, // output
    const OutFieldPortalType& hierarchicalTreeHyperarcsPortal   // output
  ) const
  { // operator ()
    // per new hypernode
    // retrieve the old hypernode ID & super ID (DONE as part of the worklet invocation)
    // convert into new IDs
    viskores::Id newHypernodeId = this->NumOldHypernodes + newHypernode;
    viskores::Id newHypernodeSuperId = hierarchicalSuperIdPortal.Get(oldSupernodeId);

    // store the new hypernode ID
    hierarchicalTreeHypernodesPortal.Set(newHypernodeId, newHypernodeSuperId);

    // retrieve the hyperarc and convert, carrying masking bits
    viskores::Id newHyperarcOldSuperId = hierarchicalHyperarcPortal.Get(oldSupernodeId);
    viskores::Id isAscendingHyperarc =
      viskores::worklet::contourtree_augmented::IsAscending(newHyperarcOldSuperId)
      ? viskores::worklet::contourtree_augmented::IS_ASCENDING
      : 0x0;
    newHyperarcOldSuperId =
      viskores::worklet::contourtree_augmented::MaskedIndex(newHyperarcOldSuperId);
    viskores::Id newHyperarcNewSuperId =
      hierarchicalSuperIdPortal.Get(newHyperarcOldSuperId) | isAscendingHyperarc;

    // and store it
    hierarchicalTreeHyperarcsPortal.Set(newHypernodeId, newHyperarcNewSuperId);

    // In serial this worklet implements the following operation
    /*
    // B.  Copy in the hypernodes & hyperarcs
    for (indexType newHypernode = 0; newHypernode < nNewHypernodes; newHypernode++)
      { // per new hypernode
      // retrieve the old hypernode ID & super ID
      indexType oldSupernodeID = newHypernodes[newHypernode];

      // convert into new IDs
      indexType newHypernodeID = nOldHypernodes + newHypernode;
      indexType newHypernodeSuperID = hierarchicalSuperID[oldSupernodeID];

      // store the new hypernode ID
      hierarchicalTree.hypernodes[newHypernodeID] = newHypernodeSuperID;

      // retrieve the hyperarc and convert, carrying masking bits
      indexType newHyperarcOldSuperID = hierarchicalHyperarc[oldSupernodeID];
      indexType isAscendingHyperarc = isAscending(newHyperarcOldSuperID) ? IS_ASCENDING : 0x0;
      newHyperarcOldSuperID = maskedIndex(newHyperarcOldSuperID);
      indexType newHyperarcNewSuperID = hierarchicalSuperID[newHyperarcOldSuperID] | isAscendingHyperarc;

      // and store it
      hierarchicalTree.hyperarcs[newHypernodeID] = newHyperarcNewSuperID;
      } // per new hypernode
    */
  } // operator ()

private:
  viskores::Id NumOldHypernodes;

}; // CopyNewHypernodes

} // namespace tree_grafter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
