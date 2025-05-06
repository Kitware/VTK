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

#ifndef viskores_filter_scalar_topology_worklet_select_top_volume_branches_CollectOuterSaddleWorklet_h
#define viskores_filter_scalar_topology_worklet_select_top_volume_branches_CollectOuterSaddleWorklet_h

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

/// <summary>
/// worklet to get the outer saddles of parent branches from branch-decomposition tree
/// This is to visualize the isosurface belong to the parent branch
/// that is symmetrical to the outer-most child branch
/// we collect the first saddle isovalue if branchSaddleEpsilon(parent) < 0
/// or the last saddle isovalue if branchSaddleEpsilon(parent) > 0
/// or both if branchSaddleEpsilon(parent) == 0
/// </summary>
class CollectOuterSaddle : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(
    FieldIn parentSaddleEpsilon, // parent saddle epsilon
    WholeArrayIn branchParent,   // (array input) parent branch root ID (local)
    FieldOut IsOuterSaddle       // (output) whether the branch is an outer saddle of the parent
  );
  using ExecutionSignature = _3(InputIndex, _1, _2);
  using InputDomain = _1;

  using IdArrayPortalType = typename IdArrayType::ReadPortalType;

  /// Constructor
  VISKORES_EXEC_CONT
  CollectOuterSaddle() {}

  VISKORES_EXEC viskores::Id operator()(const viskores::Id& inputIndex,
                                        const viskores::Id& parentSaddleEpsilon,
                                        const IdArrayPortalType& branchParent) const
  {
    const viskores::Id selfParent = branchParent.Get(inputIndex);
    viskores::Id isOuterSaddle = 0;
    if (viskores::worklet::contourtree_augmented::NoSuchElement(selfParent))
    {
      return isOuterSaddle;
    }
    const bool isFirst = (inputIndex == 0) || (branchParent.Get(inputIndex - 1) != selfParent);
    const bool isLast = (inputIndex == branchParent.GetNumberOfValues() - 1) ||
      (branchParent.Get(inputIndex + 1) != selfParent);
    if (isFirst && parentSaddleEpsilon <= 0)
    {
      isOuterSaddle |= 1;
    }
    if (isLast && parentSaddleEpsilon >= 0)
    {
      isOuterSaddle |= 2;
    }
    return isOuterSaddle;
  }
}; // CollectOuterSaddle

} // namespace select_top_volume_branches
} // namespace scalar_topology
} // namespace worklet
} // namespace viskores

#endif
