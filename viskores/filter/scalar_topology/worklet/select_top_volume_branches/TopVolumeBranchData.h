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
//=======================================================================================
//
//  Parallel Peak Pruning v. 2.0
//
//  Started June 15, 2017
//
// Copyright Hamish Carr, University of Leeds
//
// BranchDecompositionTreeMaker.h
//
//=======================================================================================
//
// COMMENTS:
//
//      This class computes the branch decomposition tree of top-volume branches
//
//=======================================================================================


#ifndef viskores_filter_scalar_topology_worklet_TopVolumeBranchData_h
#define viskores_filter_scalar_topology_worklet_TopVolumeBranchData_h

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

#ifdef DEBUG_PRINT
#define DEBUG_BRANCH_DECOMPOSITION_TREE_MAKER
#endif

namespace viskores
{
namespace filter
{
namespace scalar_topology
{

/// Data to store all information about top branches by volume
struct TopVolumeBranchData
{ // struct TopVolumeBranchData

  // Data from DHCT. Size: nBranches
  viskores::worklet::contourtree_augmented::IdArrayType BranchRootByBranch;
  viskores::worklet::contourtree_augmented::IdArrayType BranchRootGRId;
  viskores::worklet::contourtree_augmented::IdArrayType BranchVolume;
  viskores::worklet::contourtree_augmented::IdArrayType BranchSaddleEpsilon;
  viskores::worklet::contourtree_augmented::IdArrayType SortedBranchByVolume;
  viskores::cont::UnknownArrayHandle BranchSaddleIsoValue;

  // True if it is the parent of a branch in the branch decomposition tree
  viskores::cont::ArrayHandle<bool> IsParentBranch;

  // Output Datasets. Information of top branches by volume.
  // Size: nTopVolBranches
  viskores::worklet::contourtree_augmented::IdArrayType TopVolumeBranchRootGRId;
  viskores::worklet::contourtree_augmented::IdArrayType TopVolumeBranchVolume;
  // the parent branch of top branches (if no parent branch, then NO_SUCH_ELEMENT)
  viskores::worklet::contourtree_augmented::IdArrayType TopVolumeBranchParent;
  viskores::cont::UnknownArrayHandle TopVolumeBranchSaddleIsoValue;
  viskores::worklet::contourtree_augmented::IdArrayType TopVolumeBranchSaddleEpsilon;
  viskores::worklet::contourtree_augmented::IdArrayType TopVolumeBranchUpperEndGRId;
  viskores::worklet::contourtree_augmented::IdArrayType TopVolumeBranchLowerEndGRId;

  // Other top-volume branch information

  // Whether the top volume branch is known by the block.
  // size: nTopVolBranches
  // value range: [0, 1]
  viskores::worklet::contourtree_augmented::IdArrayType TopVolBranchKnownByBlockStencil;

  // Branch order (among all branches) by branch root global regular ids.
  // size: nTopVolBranches
  // value range: {NO_SUCH_ELEMENT} U [0, nBranches - 1]
  viskores::worklet::contourtree_augmented::IdArrayType TopVolBranchGROrder;

  // Branch information index (among all branches) of the top volume branch.
  // Top-volume branches outside the block are excluded.
  // size: nTopVolBranchKnownByBlock
  // value range: [0, nBranches - 1]
  viskores::worklet::contourtree_augmented::IdArrayType TopVolBranchInfoActualIndex;

  // Information to extract extra contours
  // For each top-volume branch, we extract a contour on the branch near the saddle,
  // and an extra contour on the parent branch near the same saddle
  // The extra contour is dependent on the saddle of top-volume branches

  // For top-volume branches with a maximum as the end,
  // the isovalue of the extra contour will be higher than the saddle end of the top-volume branch
  viskores::worklet::contourtree_augmented::IdArrayType ExtraMaximaBranchUpperEnd;
  viskores::worklet::contourtree_augmented::IdArrayType ExtraMaximaBranchLowerEnd;
  // We copy the branch order of the top-volume branch
  viskores::worklet::contourtree_augmented::IdArrayType ExtraMaximaBranchOrder;
  // We record the saddle end global regular IDs
  viskores::worklet::contourtree_augmented::IdArrayType ExtraMaximaBranchSaddleGRId;
  viskores::cont::UnknownArrayHandle ExtraMaximaBranchIsoValue;

  // For top-volume branches with a minimum as the end,
  // the isovalue of the extra contour will be lower than the saddle end of the top-volume branch
  viskores::worklet::contourtree_augmented::IdArrayType ExtraMinimaBranchUpperEnd;
  viskores::worklet::contourtree_augmented::IdArrayType ExtraMinimaBranchLowerEnd;
  // We copy the branch order of the top-volume branch
  viskores::worklet::contourtree_augmented::IdArrayType ExtraMinimaBranchOrder;
  // We record the saddle end global regular IDs
  viskores::worklet::contourtree_augmented::IdArrayType ExtraMinimaBranchSaddleGRId;
  viskores::cont::UnknownArrayHandle ExtraMinimaBranchIsoValue;
}; // class TopVolumeBranchData



} // namespace scalar_topology
} // namespace filter
} // namespace viskores


#endif
