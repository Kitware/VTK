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

#ifndef viskores_worklet_contourtree_distributed_hierarchical_hyper_sweeper_compute_superarc_transfer_weights_worklet_h
#define viskores_worklet_contourtree_distributed_hierarchical_hyper_sweeper_compute_superarc_transfer_weights_worklet_h

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

/// Worklet used in HierarchicalHyperSweeper.InitializeRegularVertexCount(...) to
/// subtract out the low end from the superarc regular counts
class ComputeSuperarcTransferWeightsWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(
    FieldIn supernodeIndex, // counting array [firstSupernode, lastSupernode)
    FieldIn
      hierarchicalTreeSupernodesView, // view of hierarchicalTree.supernodes[firstSupernode, lastSupernode)
    WholeArrayIn hierarchicalTreeSuperparents, // whole array of hierarchicalTree.superparents
    WholeArrayIn hierarchicalTreeHyperparents, // whole array of hierarchicalTree.hyperparents
    FieldIn
      hierarchicalTreeSuperarcsView, // view of hierarchicalTree.superarcs[firstSupernode, lastSupernode)
    FieldOut transferTargetView      // view of transferTarget[firstSupernode, lastSupernode)
  );
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);

  // Default Constructor
  VISKORES_EXEC_CONT
  ComputeSuperarcTransferWeightsWorklet(const viskores::Id& round,
                                        const viskores::Id& hierarchicalTreeNumRounds,
                                        const viskores::Id& lastSupernode)
    : Round(round)
    , HierarchicalTreeNumRounds(hierarchicalTreeNumRounds)
    , LastSupernode(lastSupernode)
  {
  }

  template <typename InFieldPortalType>
  VISKORES_EXEC void operator()(
    const viskores::Id& supernode,
    const viskores::Id& supernodeRegularId, // same as  hierarchicalTree.supernodes[supernode];
    const InFieldPortalType& hierarchicalTreeSuperparentsPortal,
    const InFieldPortalType& hierarchicalTreeHyperparentsPortal,
    const viskores::Id& superarcTo, // same as hierarchicalTree.superarcs[supernode];
    viskores::Id& transferTarget    // same as transferTarget[supernode]
  ) const
  {
    // per supernode
    // if there is no superarc, it is either the root of the tree or an attachment point
    if (viskores::worklet::contourtree_augmented::NoSuchElement(superarcTo))
    { // null superarc
      // next we test for whether it is the global root
      if (this->Round == HierarchicalTreeNumRounds)
      { // global root
        // no transfer, so no target
        transferTarget = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
      } // global root
      else
      { // attachment point
        // set the transfer target
        transferTarget = hierarchicalTreeSuperparentsPortal.Get(supernodeRegularId) |
          viskores::worklet::contourtree_augmented::TRANSFER_TO_SUPERARC;
      } // attachment point
    }   // null superarc
    else
    { // actual superarc
      // test for the last in the subrange / last on the hyperarc
      if ((supernode != this->LastSupernode - 1) &&
          (hierarchicalTreeHyperparentsPortal.Get(supernode) ==
           hierarchicalTreeHyperparentsPortal.Get(supernode + 1)))
      { // not a superarc we care about
        transferTarget = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
      } // not a superarc we care about
      else
      { // a superarc we care about
        // strip off the flag bits and set the weight & target
        transferTarget = viskores::worklet::contourtree_augmented::MaskedIndex(superarcTo);
      } // a superarc we care about
    }   // actual superarc

    // In serial this worklet implements the following operation
    /*
    for (viskores::Id supernode = firstSupernode; supernode < lastSupernode; supernode++)
    { // per supernode
      // we need to know the superarc
      viskores::Id superarcTo = hierarchicalTree.superarcs[supernode];
      viskores::Id supernodeRegularId = hierarchicalTree.supernodes[supernode];

      // if there is no superarc, it is either the root of the tree or an attachment point
      if (noSuchElement(superarcTo))
      { // null superarc
        // next we test for whether it is the global root
        if (round == hierarchicalTree.nRounds)
        { // global root
          // no transfer, so no target
          transferTarget[supernode] = NO_SUCH_ELEMENT;
        } // global root
        else
        { // attachment point
          // set the transfer target
          transferTarget[supernode] = hierarchicalTree.superparents[supernodeRegularId];
        } // attachment point
      } // null superarc
      else
      { // actual superarc
        // test for the last in the subrange / last on the hyperarc
        if ((supernode != lastSupernode - 1) && (hierarchicalTree.hyperparents[supernode] == hierarchicalTree.hyperparents[supernode+1]))
        { // not a superarc we care about
          transferTarget[supernode] = NO_SUCH_ELEMENT;
        } // not a superarc we care about
        else
        { // a superarc we care about
          // strip off the flag bits
          superarcTo = maskedIndex(superarcTo);

          // and set the weight & target
          transferTarget[supernode] = superarcTo;
        } // a superarc we care about
      } // actual superarc
    } // per supernode
    */
  } // operator()()

private:
  const viskores::Id Round;
  const viskores::Id HierarchicalTreeNumRounds;
  const viskores::Id LastSupernode;

}; // ComputeSuperarcTransferWeightsWorklet

} // namespace hierarchical_hyper_sweeper
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
