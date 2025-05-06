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

#ifndef viskores_worklet_contourtree_augmented_contourtree_maker_inc_set_new_hyper_nodes_and_arcs_h
#define viskores_worklet_contourtree_augmented_contourtree_maker_inc_set_new_hyper_nodes_and_arcs_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{
namespace contourtree_maker_inc
{

// Worklet for settubg the super/hyperarcs fromm the permuted super/hyperarcs vector
class ComputeHyperAndSuperStructure_SetNewHypernodesAndArcs
  : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn contourTreeSupernodes, // (input) active super/hyperarcs
                                WholeArrayIn contourTreeWhenTransferred, // (input)
                                WholeArrayIn contourTreeHypernodes,      // (input)
                                WholeArrayIn contourTreeHyperarcs,       // (input)
                                WholeArrayIn newHypernodePosition,       // (input)
                                WholeArrayOut newHypernodes,             // (output)
                                WholeArrayOut newHyperarcs               // (output)
  );                                                                     // (output)
  typedef void ExecutionSignature(_1, InputIndex, _2, _3, _4, _5, _6, _7);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  ComputeHyperAndSuperStructure_SetNewHypernodesAndArcs() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(
    const viskores::Id& /*supernodeID*/, // FIXME: Remove unused parameter?
    const viskores::Id supernode,
    const InFieldPortalType& contourTreeWhenTransferredPortal,
    const InFieldPortalType& contourTreeHypernodesPortal,
    const InFieldPortalType& contourTreeHyperarcsPortal,
    const InFieldPortalType& newHypernodePositionPortal,
    const OutFieldPortalType& newHypernodesPortal,
    const OutFieldPortalType& newHyperarcsPortal) const
  {
    bool isAHypernode = IsHypernode(contourTreeWhenTransferredPortal.Get(supernode));
    // ignore non-hypernodes
    // all others (including the root hypernode) are kept
    if (isAHypernode)
    {
      newHypernodesPortal.Set(newHypernodePositionPortal.Get(supernode),
                              contourTreeHypernodesPortal.Get(supernode));
      newHyperarcsPortal.Set(newHypernodePositionPortal.Get(supernode),
                             contourTreeHyperarcsPortal.Get(supernode));
    }

    // In serial this worklet implements the following operation
    /*
      for (viskores::Id supernode = 0; supernode < contourTree.Supernodes.size(); supernode++)
        { // per supernode
          bool isAHypernode = IsHypernode(contourTree.WhenTransferred[supernode]);

          // ignore non-hypernodes
          // all others (including the root hypernode) are kept
          if (isAHypernode)
            {
              newHypernodes[newHypernodePosition[supernode]] = contourTree.hypernodes[supernode];
              newHyperarcs[newHypernodePosition[supernode]] = contourTree.hyperarcs[supernode];
            }
        } // per supernode
      */
  }

}; // AugmentMergeTrees_InitNewJoinSplitIDAndSuperparents.h

} // namespace contourtree_maker_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
