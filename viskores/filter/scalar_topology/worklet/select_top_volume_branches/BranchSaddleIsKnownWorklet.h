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

#ifndef viskores_filter_scalar_topology_worklet_select_top_volume_branches_BranchSaddleIsKnownWorklet_h
#define viskores_filter_scalar_topology_worklet_select_top_volume_branches_BranchSaddleIsKnownWorklet_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace scalar_topology
{
namespace select_top_volume_branches
{

using IdArrayType = viskores::worklet::contourtree_augmented::IdArrayType;

/// <summary>
/// worklet to check whether the saddle end of branch is known by the block
/// if true, we return the saddle end supernode id
/// if false (or main branch), we return NO_SUCH_ELEMENT
/// </summary>
class BranchSaddleIsKnownWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(
    FieldIn lowerEndGRId,      // (input) branch lower end global regular id
    FieldIn lowerLocalEnd,     // (input) branch local lower end
    FieldIn lowerLocalEndGRId, // (input) branch local lower end global regular id
    FieldIn upperEndGRId,      // (input) branch upper end global regular id
    FieldIn upperLocalEnd,     // (input) branch local upper end
    FieldIn upperLocalEndGRId, // (input) branch local upper end global regular id
    FieldIn branchSaddleEps,   // (input) branch saddle epsilon
    FieldOut branchSaddle      // (output) the branch saddle (if known by the block)
  );
  using ExecutionSignature = _8(_1, _2, _3, _4, _5, _6, _7);
  using InputDomain = _1;

  /// Constructor
  VISKORES_EXEC_CONT
  BranchSaddleIsKnownWorklet() {}

  /// The functor checks the direction of the branch
  VISKORES_EXEC viskores::Id operator()(const viskores::Id& lowerEndGRId,
                                        const viskores::Id& lowerLocalEnd,
                                        const viskores::Id& lowerLocalEndGRId,
                                        const viskores::Id& upperEndGRId,
                                        const viskores::Id& upperLocalEnd,
                                        const viskores::Id& upperLocalEndGRId,
                                        const viskores::Id& branchSaddleEps) const
  {
    // if main branch
    if (branchSaddleEps == 0)
      return viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;

    // if the branch is a maximum-saddle branch
    if (branchSaddleEps > 0)
      return lowerEndGRId == lowerLocalEndGRId
        ? lowerLocalEnd
        : viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;

    // if the branch is a minimum-saddle branch
    if (branchSaddleEps < 0)
      return upperEndGRId == upperLocalEndGRId
        ? upperLocalEnd
        : viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;

    // in case of fallout, should never reach
    return viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
  }

}; // BranchSaddleIsKnownWorklet

} // namespace select_top_volume_branches
} // namespace scalar_topology
} // namespace worklet
} // namespace viskores

#endif
