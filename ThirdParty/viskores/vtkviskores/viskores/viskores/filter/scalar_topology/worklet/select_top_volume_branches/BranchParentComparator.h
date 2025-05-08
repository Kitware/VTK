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

#ifndef viskores_filter_scalar_topology_worklet_branch_decomposition_select_top_volume_branches_BranchParentComparator_h
#define viskores_filter_scalar_topology_worklet_branch_decomposition_select_top_volume_branches_BranchParentComparator_h

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

// Implementation of BranchParentComparator
template <typename ValueType>
class BranchParentComparatorImpl
{
public:
  using ValueArrayType = typename viskores::cont::ArrayHandle<ValueType>;
  using IdPortalType = typename IdArrayType::ReadPortalType;
  using ValuePortalType = typename ValueArrayType::ReadPortalType;

  // constructor
  VISKORES_CONT
  BranchParentComparatorImpl(const IdArrayType& branchParent,
                             const ValueArrayType& saddleIsoValue,
                             const IdArrayType& branchRootGRId,
                             viskores::cont::DeviceAdapterId device,
                             viskores::cont::Token& token)
    : branchParentPortal(branchParent.PrepareForInput(device, token))
    , saddleIsoValuePortal(saddleIsoValue.PrepareForInput(device, token))
    , branchRootGRIdPortal(branchRootGRId.PrepareForInput(device, token))
  { // constructor
  } // constructor

  // () operator - gets called to do comparison
  VISKORES_EXEC
  bool operator()(const viskores::Id& i, const viskores::Id& j) const
  { // operator()
    viskores::Id parentI = this->branchParentPortal.Get(i);
    viskores::Id parentJ = this->branchParentPortal.Get(j);

    // primary sort on branch parent
    if (parentI < parentJ)
      return true;
    if (parentI > parentJ)
      return false;

    ValueType valueI = this->saddleIsoValuePortal.Get(i);
    ValueType valueJ = this->saddleIsoValuePortal.Get(j);

    // secondary sort on branch saddle isovalue
    if (valueI < valueJ)
      return true;
    if (valueI > valueJ)
      return false;

    viskores::Id rootI = this->branchRootGRIdPortal.Get(i);
    viskores::Id rootJ = this->branchRootGRIdPortal.Get(j);

    return (rootI < rootJ);
  } // operator()

private:
  IdPortalType branchParentPortal;
  ValuePortalType saddleIsoValuePortal;
  IdPortalType branchRootGRIdPortal;

}; // BranchParentComparatorImpl

/// <summary>
/// Comparator of branch parent. Lower parent comes first
/// </summary>
template <typename ValueType>
class BranchParentComparator : public viskores::cont::ExecutionObjectBase
{
  using ValueArrayType = typename viskores::cont::ArrayHandle<ValueType>;

public:
  // constructor
  VISKORES_CONT
  BranchParentComparator(const IdArrayType& branchParent,
                         const ValueArrayType& saddleIsoValue,
                         const IdArrayType& branchRootGRId)
    : BranchParent(branchParent)
    , SaddleIsoValue(saddleIsoValue)
    , BranchRootGRId(branchRootGRId)
  {
  }

  VISKORES_CONT BranchParentComparatorImpl<ValueType> PrepareForExecution(
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token) const
  {
    return BranchParentComparatorImpl<ValueType>(
      this->BranchParent, this->SaddleIsoValue, this->BranchRootGRId, device, token);
  }

private:
  IdArrayType BranchParent;
  ValueArrayType SaddleIsoValue;
  IdArrayType BranchRootGRId;
}; // BranchParentComparator


// Implementation of SuperarcTargetComparator
class SuperarcTargetComparatorImpl
{
public:
  using IdPortalType = typename IdArrayType::ReadPortalType;

  // constructor
  VISKORES_CONT
  SuperarcTargetComparatorImpl(const IdArrayType& superarcTarget,
                               viskores::cont::DeviceAdapterId device,
                               viskores::cont::Token& token)
    : superarcPortal(superarcTarget.PrepareForInput(device, token))
  { // constructor
  } // constructor

  // () operator - gets called to do comparison
  VISKORES_EXEC
  bool operator()(const viskores::Id& i, const viskores::Id& j) const
  { // operator()
    VISKORES_ASSERT(i < superarcPortal.GetNumberOfValues() && i >= 0);
    VISKORES_ASSERT(j < superarcPortal.GetNumberOfValues() && j >= 0);
    viskores::Id superarcI = this->superarcPortal.Get(i);
    viskores::Id superarcJ = this->superarcPortal.Get(j);

    bool isNullI = viskores::worklet::contourtree_augmented::NoSuchElement(superarcI);
    bool isNullJ = viskores::worklet::contourtree_augmented::NoSuchElement(superarcJ);

    // let the NULL superarc always go first
    if (isNullI && !isNullJ)
      return true;
    if (isNullJ && !isNullI)
      return false;

    viskores::Id targetI = viskores::worklet::contourtree_augmented::MaskedIndex(superarcI);
    viskores::Id targetJ = viskores::worklet::contourtree_augmented::MaskedIndex(superarcJ);

    // primary sort on the superarc target
    return (targetI < targetJ);
  } // operator()

private:
  IdPortalType superarcPortal;

}; // SuperarcTargetComparatorImpl

/// <summary>
/// Comparator of superarc target. The NULL superarc always comes first.
/// </summary>
class SuperarcTargetComparator : public viskores::cont::ExecutionObjectBase
{

public:
  // constructor
  VISKORES_CONT
  SuperarcTargetComparator(const IdArrayType& superarcTarget)
    : SuperarcTarget(superarcTarget)
  {
  }

  VISKORES_CONT SuperarcTargetComparatorImpl
  PrepareForExecution(viskores::cont::DeviceAdapterId device, viskores::cont::Token& token) const
  {
    return SuperarcTargetComparatorImpl(this->SuperarcTarget, device, token);
  }

private:
  IdArrayType SuperarcTarget;
}; // SuperarcTargetComparator


} // namespace select_top_volume_branches
} // namespace scalar_topology
} // namespace worklet
} // namespace viskores

#endif
