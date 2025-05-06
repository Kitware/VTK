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

#ifndef viskores_worklet_contourtree_distributed_tree_grafter_copy_new_supernodes_set_superchildren_worklet_h
#define viskores_worklet_contourtree_distributed_tree_grafter_copy_new_supernodes_set_superchildren_worklet_h


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

/// Worklet to loop to set the number of superchildren per hyperarc as part of TreeGrafter::CopyNewSupernodes
class CopyNewSupernodesSetSuperchildrenWorklet : public viskores::worklet::WorkletMapField
{
public:
  // TODO: Access to  hierarchicalTreeSuperarcs and hierarchicalTreeHyperparents could potentially be imporved by using an ArrayView instead
  using ControlSignature = void(
    FieldIn newSupernodeIndex, // input array starting at 0 to NewSupernodes.GetNumberOfValues();
    WholeArrayIn hierarchicalTreeSuperarcs,     //input
    WholeArrayIn hierarchicalTreeHyperparents,  // input
    WholeArrayIn hierarchicalTreeHypernodes,    //input
    WholeArrayOut hierarchicalTreeSuperchildren // output
  );

  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  /// Default Constructor
  /// @param[in] numHierarchicalTreeSupernodes should be set to hierarchicalTree.Supernodes.GetNumberOfValues()
  VISKORES_EXEC_CONT
  CopyNewSupernodesSetSuperchildrenWorklet(viskores::Id numHierarchicalTreeSupernodes)
    : NumHierarchicalTreeSupernodes(numHierarchicalTreeSupernodes)
  {
  }

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& newSupernodeIndex,
                                const InFieldPortalType& hierarchicalTreeSuperarcsPortal,
                                const InFieldPortalType& hierarchicalTreeHyperparentsPortal,
                                const InFieldPortalType& hierarchicalTreeHypernodesPortal,
                                const OutFieldPortalType hierarchicalTreeSuperchildrenPortal

  ) const
  { // operator ()
    // convert from 0...NumNewSupernodes index to [hierarchicalTree.supernodes.size() - newSupernodes.size(), hierarchicalTree.supernodes.size())
    viskores::Id newSupernode = (this->NumHierarchicalTreeSupernodes - 1) - newSupernodeIndex;

    // per new supernode
    // attachment points have NULL superarcs and can be ignored
    if (viskores::worklet::contourtree_augmented::NoSuchElement(
          hierarchicalTreeSuperarcsPortal.Get(newSupernode)))
    {
      return;
    }
    // OK: we are now guaranteed to have a valid hyperparent
    viskores::Id hyperparent = hierarchicalTreeHyperparentsPortal.Get(newSupernode);

    // we could still be at the end of the array, so we have to test explicitly
    if (newSupernode == NumHierarchicalTreeSupernodes - 1)
    {
      // compute the delta and store it
      hierarchicalTreeSuperchildrenPortal.Set(hyperparent,
                                              NumHierarchicalTreeSupernodes -
                                                hierarchicalTreeHypernodesPortal.Get(hyperparent));
    }
    else if (hyperparent != hierarchicalTreeHyperparentsPortal.Get(newSupernode + 1))
    {
      hierarchicalTreeSuperchildrenPortal.Set(
        hyperparent, newSupernode + 1 - hierarchicalTreeHypernodesPortal.Get(hyperparent));
    }

    // In serial this worklet implements the following operation
    /*
    for (indexType newSupernode = hierarchicalTree.supernodes.size() - newSupernodes.size(); newSupernode < hierarchicalTree.supernodes.size(); newSupernode++)
    { // per new supernode
      // attachment points have NULL superarcs and can be ignored
      if (noSuchElement(hierarchicalTree.superarcs[newSupernode]))
        continue;

      // OK: we are now guaranteed to have a valid hyperparent
      indexType hyperparent = hierarchicalTree.hyperparents[newSupernode];

      // we could still be at the end of the array, so we have to test explicitly
      if (newSupernode == hierarchicalTree.supernodes.size() - 1)
        // compute the delta and store it
        hierarchicalTree.superchildren[hyperparent] = hierarchicalTree.supernodes.size() - hierarchicalTree.hypernodes[hyperparent];
      else if (hyperparent != hierarchicalTree.hyperparents[newSupernode + 1])
        hierarchicalTree.superchildren[hyperparent] = newSupernode + 1 - hierarchicalTree.hypernodes[hyperparent];
    } // per new supernode

    */
  } // operator ()

private:
  viskores::Id NumHierarchicalTreeSupernodes; /// hierarchicalTree.supernodes.size()

}; // CopyNewHypernodes

} // namespace tree_grafter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
