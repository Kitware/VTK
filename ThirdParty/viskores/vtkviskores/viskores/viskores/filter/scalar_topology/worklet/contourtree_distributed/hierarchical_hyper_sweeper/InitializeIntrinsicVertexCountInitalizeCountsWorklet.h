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

#ifndef viskores_worklet_contourtree_distributed_hierarchical_hyper_sweeper_initialize_intrinsic_vertex_count_initialize_counts_worklet_h
#define viskores_worklet_contourtree_distributed_hierarchical_hyper_sweeper_initialize_intrinsic_vertex_count_initialize_counts_worklet_h

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
/// set the count to the Id one off the high end of each range
class InitializeIntrinsicVertexCountInitalizeCountsWorklet
  : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn superparents, WholeArrayInOut superarcRegularCounts);
  using ExecutionSignature = void(InputIndex, _1, _2);
  using InputDomain = _1;

  // Default Constructor
  VISKORES_EXEC_CONT
  InitializeIntrinsicVertexCountInitalizeCountsWorklet() {}

  template <typename InFieldPortalType, typename InOutFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& vertex,
                                const InFieldPortalType superparentsPortal,
                                const InOutFieldPortalType superarcRegularCountsPortal) const
  {
    // per vertex
    // retrieve the superparent
    viskores::Id superparent = superparentsPortal.Get(vertex);

    // if it's NSE, ignore (should never happen, but . . . )
    if (viskores::worklet::contourtree_augmented::NoSuchElement(superparent))
    {
      return;
    }
    // if its the last element, always write
    if (vertex == superparentsPortal.GetNumberOfValues() - 1)
    {
      superarcRegularCountsPortal.Set(superparent, vertex + 1);
    }
    // otherwise, only write if different from next one
    else
    {
      if (superparentsPortal.Get(vertex + 1) != superparent)
      {
        superarcRegularCountsPortal.Set(superparent, vertex + 1);
      }
    }

    // In serial this worklet implements the following operation
    /*
    for (viskores::Id vertex = 0; vertex < superparents.size(); vertex++)
    { // per vertex
      // retrieve the superparent
      viskores::Id superparent = superparents[vertex];

      // if it's NSE, ignore (should never happen, but . . . )
      if (noSuchElement(superparent))
        continue;

      // if its the last element, always write
      if (vertex == superparents.size() - 1)
        superarcRegularCounts[superparent] = vertex+1;
      // otherwise, only write if different from next one
      else
        if (superparents[vertex+1] != superparent)
          superarcRegularCounts[superparent] = vertex+1;
    } // per vertex
    */
  } // operator()()

}; // InitializeIntrinsicVertexCountInitalizeCountsWorklet

} // namespace hierarchical_hyper_sweeper
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
