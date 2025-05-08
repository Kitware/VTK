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

#ifndef viskores_filter_scalar_topology_worklet_branch_decomposition_hierarchical_volumetric_branch_decomposer_get_outer_end_worklet_h
#define viskores_filter_scalar_topology_worklet_branch_decomposition_hierarchical_volumetric_branch_decomposer_get_outer_end_worklet_h

#include <viskores/worklet/WorkletMapField.h>


namespace viskores
{
namespace worklet
{
namespace scalar_topology
{
namespace hierarchical_volumetric_branch_decomposer
{

/// Worklet for getting the outer node ID of a superarc
template <bool isLower>
class GetSuperarcOuterNodeWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(
    FieldIn superarcId, // (input) superarc ID without flag bits
    FieldIn superarcTo, // (input) target node ID of the superarc with flag bits
    FieldOut endNodeId  // (output) end node ID of the superarc
  );
  using ExecutionSignature = _3(_1, _2);
  using InputDomain = _1;

  /// Constructor
  /// isLower determines whether to find the upper end or the lower end of the superarc
  VISKORES_EXEC_CONT
  GetSuperarcOuterNodeWorklet() {}

  /// The functor checks the direction of the superarc based on the flag information
  /// and returns the outer end supernode of the superarc (without flag information)
  VISKORES_EXEC viskores::Id operator()(const viskores::Id& superarcId,
                                        const viskores::Id& superarcTo) const
  {
    if (contourtree_augmented::NoSuchElement(superarcTo))
    {
      return contourtree_augmented::NO_SUCH_ELEMENT;
    }
    bool ascendingSuperarc = contourtree_augmented::IsAscending(superarcTo);
    if (isLower ^ ascendingSuperarc)
    {
      return contourtree_augmented::MaskedIndex(superarcTo);
    }
    else
    {
      return superarcId;
    }
  }
}; // GetSuperarcOuterNodeWorklet


/// Worklet for getting the outer node ID of a branch
class OneIfBranchEndWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(
    FieldIn superarcId,         // (input) actual ID of superarc
    WholeArrayIn branchRoots,   // (array input) branch root (superarc) IDs of all superarcs
    FieldOut branchEndIndicator // (output) 1 if the superarc is the last of a branch in the array
  );
  using ExecutionSignature = _3(_1, _2);
  using InputDomain = _1;

  /// Constructor
  /// isLower determines whether to find the upper end or the lower end of the branch
  VISKORES_EXEC_CONT
  OneIfBranchEndWorklet() {}

  template <typename InIdPortalType>
  VISKORES_EXEC viskores::Id operator()(const viskores::Id superarcId,
                                        const InIdPortalType& branchRootsPortal) const
  {
    const viskores::Id superarcSize = branchRootsPortal.GetNumberOfValues();

    // handle illegal situations
    if (superarcId < 0 || superarcId >= superarcSize)
      return 0;

    // if last element
    if (superarcSize == superarcId + 1)
      return 1;
    return branchRootsPortal.Get(superarcId) != branchRootsPortal.Get(superarcId + 1) ? 1 : 0;
  }

}; // OneIfBranchEndWorklet


class CopyArcDirectionWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(
    FieldIn superarcId,         // (input) superarc ID
    FieldIn superarcTo,         // (input) target of superarc, including ascending flag
    FieldOut DirectedSuperarcId // (output) superarc ID with ascending flag
  );

  using ExecutionSignature = _3(_1, _2);
  using InputDomain = _1;

  VISKORES_EXEC_CONT
  CopyArcDirectionWorklet() {}

  VISKORES_EXEC viskores::Id operator()(const viskores::Id superarcId,
                                        const viskores::Id superarcTo) const
  {
    if (contourtree_augmented::IsAscending(superarcTo))
    {
      return superarcId | (contourtree_augmented::IS_ASCENDING);
    }
    else
    {
      return superarcId;
    }
  }
}; // CopyArcDirectionWorklet

} // namespace hierarchical_volumetric_branch_decomposer
} // namespace scalar_topology
} // namespace worklet
} // namespace viskores

#endif
