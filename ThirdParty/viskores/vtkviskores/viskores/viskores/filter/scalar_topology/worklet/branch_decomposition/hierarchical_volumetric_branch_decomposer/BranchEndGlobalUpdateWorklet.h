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

#ifndef viskores_filter_scalar_topology_worklet_branch_decomposition_hierarchical_volumetric_branch_decomposer_branch_end_global_update_h
#define viskores_filter_scalar_topology_worklet_branch_decomposition_hierarchical_volumetric_branch_decomposer_branch_end_global_update_h

#include <viskores/worklet/WorkletMapField.h>


namespace viskores
{
namespace worklet
{
namespace scalar_topology
{
namespace hierarchical_volumetric_branch_decomposer
{

/// Worklet to update the information of branch end on a local block
/// by comparing to the same branch in neighbor blocks
template <typename ValueType, bool isLower>
class UpdateBranchEndByExchangeWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn sharedBranchID,             // (input)
                                FieldInOut selfEndGRID,             // (input/output)
                                FieldIn incomingEndGRID,            // (input)
                                FieldInOut selfEndValue,            // (input/output)
                                FieldIn incomingEndValue,           // (input)
                                FieldInOut selfEndSuperarcID,       // (input/output)
                                FieldIn incomingEndSuperarcID,      // (input)
                                FieldInOut selfEndIntrinsicVolume,  // (input/output)
                                FieldIn incomingEndIntrinsicVolume, // (input)
                                FieldInOut selfEndDependentVolume,  // (input/output)
                                FieldIn incomingEndDependentVolume  // (input)
  );

  /// Constructor, empty
  VISKORES_EXEC_CONT
  UpdateBranchEndByExchangeWorklet() {}

  /// The functor checks whether the incomingEnd is a better one than selfEnd
  /// If yes, update all information for the selfEnd
  /// Otherwise, do nothing
  VISKORES_EXEC void operator()(const viskores::Id& sharedBranchID,
                                viskores::Id& selfEndGRID,
                                const viskores::Id& incomingEndGRID,
                                ValueType& selfEndValue,
                                const ValueType& incomingEndValue,
                                viskores::Id& selfEndSuperarcID,
                                const viskores::Id& incomingEndSuperarcID,
                                viskores::Id& selfEndIntrinsicVolume,
                                const viskores::Id& incomingEndIntrinsicVolume,
                                viskores::Id& selfEndDependentVolume,
                                const viskores::Id& incomingEndDependentVolume) const
  {
    // sharedBranchID is only used as an index anchor for all shared branches.
    // We don't use its content.

    if (selfEndGRID == incomingEndGRID)
      return;

    // isLower == True: if self is lower than incoming, do nothing
    // isLower == False: if self is higher than incoming, do nothing
    if (isLower &&
        (selfEndValue < incomingEndValue ||
         (selfEndValue == incomingEndValue && selfEndGRID < incomingEndGRID)))
      return;
    if (!isLower &&
        (selfEndValue > incomingEndValue ||
         (selfEndValue == incomingEndValue && selfEndGRID > incomingEndGRID)))
      return;

    // Time to update
    selfEndGRID = incomingEndGRID;
    selfEndValue = incomingEndValue;
    selfEndSuperarcID = incomingEndSuperarcID;
    selfEndIntrinsicVolume = incomingEndIntrinsicVolume;
    selfEndDependentVolume = incomingEndDependentVolume;

    // Prevent unused parameter warning
    (void)sharedBranchID;
  }

}; // UpdateBranchEndByExchangeWorklet

} // namespace hierarchical_volumetric_branch_decomposer
} // namespace scalar_topology
} // namespace worklet
} // namespace viskores

#endif
