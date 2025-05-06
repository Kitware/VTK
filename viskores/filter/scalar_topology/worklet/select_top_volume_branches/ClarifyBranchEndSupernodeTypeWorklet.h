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

#ifndef viskores_filter_scalar_topology_worklet_select_top_volume_branches_ClarifyBranchEndSupernodeTypeWorklet_h
#define viskores_filter_scalar_topology_worklet_select_top_volume_branches_ClarifyBranchEndSupernodeTypeWorklet_h

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

// For special branches that only have one superarc
// Clarify which end is leaf and which is saddle
class ClarifyBranchEndSupernodeTypeWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(
    FieldIn lowerSuperarcId, // (input) lower end superarc ID
    FieldIn lowerIntrinsic,  // (input) lower end superarc intrisic volume
    FieldIn upperSuperarcId, // (input) upper end superarc ID
    FieldIn upperIntrinsic,  // (input) upper end superarc intrisic volume
    FieldIn branchRoot,      // (input) branch root superarc ID
    FieldInOut isLowerLeaf,  // (input/output) bool, whether the lower end is a leaf
    FieldInOut isUpperLeaf   // (input/output) bool, whether the upper end is a leaf
  );
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7);
  using InputDomain = _1;

  /// Constructor
  VISKORES_EXEC_CONT
  ClarifyBranchEndSupernodeTypeWorklet(const viskores::Id tVol)
    : totalVolume(tVol)
  {
  }

  /// The functor checks the direction of the branch
  VISKORES_EXEC void operator()(const viskores::Id& lowerSuperarcId,
                                const viskores::Id& lowerIntrinsic,
                                const viskores::Id& upperSuperarcId,
                                const viskores::Id& upperIntrinsic,
                                const viskores::Id& branchRoot,
                                bool& isLowerLeaf,
                                bool& isUpperLeaf) const
  {
    // do nothing: not a "leaf-leaf" branch
    if (!isLowerLeaf || !isUpperLeaf)
      return;

    // do nothing: actual leaf-leaf branch
    if (lowerIntrinsic == totalVolume - 1 && lowerIntrinsic == upperIntrinsic)
      return;

    // do something: fake leaf-leaf branch
    // we already exclude the case of only one superarc above
    viskores::Id maskedLowerId =
      viskores::worklet::contourtree_augmented::MaskedIndex(lowerSuperarcId);
    viskores::Id maskedUpperId =
      viskores::worklet::contourtree_augmented::MaskedIndex(upperSuperarcId);
    viskores::Id maskedRoot = viskores::worklet::contourtree_augmented::MaskedIndex(branchRoot);

    if (maskedLowerId == maskedRoot && maskedUpperId == maskedRoot)
    {
      const bool isAscending =
        viskores::worklet::contourtree_augmented::IsAscending(lowerSuperarcId);
      if (isAscending)
        isUpperLeaf = false;
      else
        isLowerLeaf = false;
    }
  }

private:
  const viskores::Id totalVolume;
}; // ClarifyBranchEndSupernodeTypeWorklet

} // namespace select_top_volume_branches
} // namespace scalar_topology
} // namespace worklet
} // namespace viskores

#endif
