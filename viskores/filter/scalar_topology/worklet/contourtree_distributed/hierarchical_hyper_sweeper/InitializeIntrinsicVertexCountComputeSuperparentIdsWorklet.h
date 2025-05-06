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

#ifndef viskores_worklet_contourtree_distributed_hierarchical_hyper_sweeper_initialize_intrinsic_vertex_count_compute_superparent_ids_worklet_h
#define viskores_worklet_contourtree_distributed_hierarchical_hyper_sweeper_initialize_intrinsic_vertex_count_compute_superparent_ids_worklet_h

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

/// Worklet used in HierarchicalHyperSweeper.InitializeIntrinsicVertexCount(...) to
/// Look up the global Ids in the hierarchical tree & convert to superparent Ids
class InitializeIntrinsicVertexCountComputeSuperparentIdsWorklet
  : public viskores::worklet::WorkletMapField
{
public:
  // TODO: We could avoid the need for WholeArrayIn if we did the findRegularByGlobal mapping outside of the worklet first and then use the mapped
  using ControlSignature = void(FieldIn globalIds,                              // input
                                ExecObject findRegularByGlobal,                 // input
                                WholeArrayIn hierarchicalTreeRegular2Supernode, // input
                                WholeArrayIn hierarchicalTreeSuperparents,      // input
                                FieldOut superparents                           // output

  );
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  InitializeIntrinsicVertexCountComputeSuperparentIdsWorklet() {}

  template <typename ExecObjType, typename InFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& globalId,
                                const ExecObjType& findRegularByGlobal,
                                const InFieldPortalType& hierarchicalTreeRegular2SupernodePortal,
                                const InFieldPortalType& hierarchicalTreeSuperparentsPortal,
                                viskores::Id& superparent) const
  {
    // per vertex
    // retrieve the regular Id (should ALWAYS exist)
    viskores::Id hierarchicalRegularId = findRegularByGlobal(globalId);
    // be paranoid
    if (viskores::worklet::contourtree_augmented::NoSuchElement(hierarchicalRegularId))
    {
      superparent = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
    }
    else
    {
      // Attachment points cause a minor problem - they are supernodes, but can have a different
      // superparent than themselves (or the same one).  We therefore test explicitly whether we
      // are a supernode, and use either supernodeId or superparent depending on this test

      // retrieve the super Id
      viskores::Id superId = hierarchicalTreeRegular2SupernodePortal.Get(hierarchicalRegularId);

      // if it doesn't have one, use it's superparent
      if (viskores::worklet::contourtree_augmented::NoSuchElement(superId))
      {
        superparent = hierarchicalTreeSuperparentsPortal.Get(hierarchicalRegularId);
      }
      else
      {
        // if it does have a superId, use it
        superparent = superId;
      }
    }
    // In serial this worklet implements the following operation
    /*
    for (viskores::Id vertex = 0; vertex < globalIds.GetNumberOfValues(); vertex++)
    { // per vertex
      // retrieve the regular Id (should ALWAYS exist)
      viskores::Id hierarchicalRegularId = hierarchicalTree.FindRegularByGlobal(globalIds[vertex]);
      // be paranoid
      if (noSuchElement(hierarchicalRegularId))
        superparents[vertex] = NO_SUCH_ELEMENT;
      else
      { // found a regular Id
        // Attachment points cause a minor problem - they are supernodes, but can have a different
        // superparent than themselves (or the same one).  We therefore test explicitly whether we
        // are a supernode, and use either supernodeId or superparent depending on this test

        // retrieve the super Id
        viskores::Id superId = hierarchicalTree.regular2supernode[hierarchicalRegularId];

        // if it doesn't have one, use it's superparent
        if (noSuchElement(superId))
          superparents[vertex] = hierarchicalTree.superparents[hierarchicalRegularId];
        else
          // if it does have a superId, use it
          superparents[vertex] = superId;
      } // found a regular Id
    } // per vertex
    */
  } // operator()()

}; // InitializeIntrinsicVertexCountComputeSuperparentIdsWorklet

} // namespace hierarchical_hyper_sweeper
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
