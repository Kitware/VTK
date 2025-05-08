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
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#ifndef viskores_filter_scalar_topology_worklet_branch_decomposition_hierarchical_volumetric_branch_decomposer_CollapseBranchesPointerDoublingWorklet_h
#define viskores_filter_scalar_topology_worklet_branch_decomposition_hierarchical_volumetric_branch_decomposer_CollapseBranchesPointerDoublingWorklet_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace filter
{
namespace scalar_topology
{
namespace hierarchical_volumetric_branch_decomposer
{

class CollapseBranchesPointerDoublingWorklet : public viskores::worklet::WorkletMapField
{
public:
  /// Control signature for the worklet
  using ControlSignature = void(WholeArrayInOut branchRoot);
  using ExecutionSignature = void(InputIndex, _1);
  using InputDomain = _1;

  /// Default Constructor
  VISKORES_EXEC_CONT
  CollapseBranchesPointerDoublingWorklet() {}

  /// operator() of the workelt
  template <typename InOutFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id superarc,
                                const InOutFieldPortalType& branchRootPortal) const
  {
    viskores::Id branchRootVal1 = branchRootPortal.Get(superarc);
    viskores::Id branchRootVal2 = branchRootPortal.Get(branchRootPortal.Get(superarc));
    if (branchRootVal1 != branchRootVal2)
    {
      branchRootPortal.Set(superarc, branchRootVal2);
    }
  } // operator()()


}; // CollapseBranchesPointerDoublingWorklet

} // namespace hierarchical_volumetric_branch_decomposer
} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
