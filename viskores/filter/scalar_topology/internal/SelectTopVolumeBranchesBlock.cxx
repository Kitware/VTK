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

#include <viskores/cont/ArrayCopy.h>
#include <viskores/filter/scalar_topology/internal/SelectTopVolumeBranchesBlock.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ArrayTransforms.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/DataSetMesh.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_contour_tree/FindSuperArcForUnknownNode.h>
#include <viskores/filter/scalar_topology/worklet/select_top_volume_branches/AboveThresholdWorklet.h>
#include <viskores/filter/scalar_topology/worklet/select_top_volume_branches/BranchParentComparator.h>
#include <viskores/filter/scalar_topology/worklet/select_top_volume_branches/ClarifyBranchEndSupernodeTypeWorklet.h>
#include <viskores/filter/scalar_topology/worklet/select_top_volume_branches/GetBranchHierarchyWorklet.h>
#include <viskores/filter/scalar_topology/worklet/select_top_volume_branches/GetBranchVolumeWorklet.h>
#include <viskores/filter/scalar_topology/worklet/select_top_volume_branches/Predicates.h>
#include <viskores/filter/scalar_topology/worklet/select_top_volume_branches/UpdateInfoByBranchDirectionWorklet.h>

#ifdef DEBUG_PRINT
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/PrintVectors.h>
#endif

namespace viskores
{
namespace filter
{
namespace scalar_topology
{
namespace internal
{

SelectTopVolumeBranchesBlock::SelectTopVolumeBranchesBlock(viskores::Id localBlockNo,
                                                           int globalBlockId)
  : LocalBlockNo(localBlockNo)
  , GlobalBlockId(globalBlockId)
{
}

void SelectTopVolumeBranchesBlock::SortBranchByVolume(const viskores::cont::DataSet& bdDataSet,
                                                      const viskores::Id totalVolume)
{
  /// Pipeline to compute the branch volume
  /// 1. check both ends of the branch. If both leaves, then main branch, volume = totalVolume
  /// 2. for other branches, check the direction of the inner superarc
  ///    branch volume = (inner superarc points to the senior-most node) ?
  ///                     dependentVolume[innerSuperarc] :
  ///                     reverseVolume[innerSuperarc]
  /// NOTE: reverseVolume = totalVolume - dependentVolume + intrinsicVolume

  // Generally, if ending superarc has intrinsicVol == dependentVol, then it is a leaf node
  viskores::cont::ArrayHandle<bool> isLowerLeaf;
  viskores::cont::ArrayHandle<bool> isUpperLeaf;

  auto upperEndIntrinsicVolume = bdDataSet.GetField("UpperEndIntrinsicVolume")
                                   .GetData()
                                   .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();
  auto upperEndDependentVolume = bdDataSet.GetField("UpperEndDependentVolume")
                                   .GetData()
                                   .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();
  auto lowerEndIntrinsicVolume = bdDataSet.GetField("LowerEndIntrinsicVolume")
                                   .GetData()
                                   .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();
  auto lowerEndDependentVolume = bdDataSet.GetField("LowerEndDependentVolume")
                                   .GetData()
                                   .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();

  auto lowerEndSuperarcId = bdDataSet.GetField("LowerEndSuperarcId")
                              .GetData()
                              .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();
  auto upperEndSuperarcId = bdDataSet.GetField("UpperEndSuperarcId")
                              .GetData()
                              .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();
  auto branchRoot = bdDataSet.GetField("BranchRootByBranch")
                      .GetData()
                      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();

  viskores::cont::Algorithm::Transform(
    upperEndIntrinsicVolume, upperEndDependentVolume, isUpperLeaf, viskores::Equal());
  viskores::cont::Algorithm::Transform(
    lowerEndIntrinsicVolume, lowerEndDependentVolume, isLowerLeaf, viskores::Equal());

  // NOTE: special cases (one-superarc branches) exist
  // if the upper end superarc == lower end superarc == branch root superarc
  // then it's probably not a leaf-leaf branch (Both equalities have to be satisfied!)
  // exception: the entire domain has only one superarc (intrinsic == dependent == total - 1)
  // then it is a leaf-leaf branch
  viskores::cont::Invoker invoke;

  viskores::worklet::scalar_topology::select_top_volume_branches::
    ClarifyBranchEndSupernodeTypeWorklet clarifyNodeTypeWorklet(totalVolume);

  invoke(clarifyNodeTypeWorklet,
         lowerEndSuperarcId,
         lowerEndIntrinsicVolume,
         upperEndSuperarcId,
         upperEndIntrinsicVolume,
         branchRoot,
         isLowerLeaf,
         isUpperLeaf);

  viskores::cont::UnknownArrayHandle upperEndValue = bdDataSet.GetField("UpperEndValue").GetData();

  // Based on the direction info of the branch, store epsilon direction and isovalue of the saddle
  auto resolveArray = [&](const auto& inArray)
  {
    using InArrayHandleType = std::decay_t<decltype(inArray)>;
    using ValueType = typename InArrayHandleType::ValueType;

    viskores::cont::ArrayHandle<ValueType> branchSaddleIsoValue;
    branchSaddleIsoValue.Allocate(isLowerLeaf.GetNumberOfValues());
    this->TopVolumeData.BranchSaddleEpsilon.Allocate(isLowerLeaf.GetNumberOfValues());

    viskores::worklet::scalar_topology::select_top_volume_branches::
      UpdateInfoByBranchDirectionWorklet<ValueType>
        updateInfoWorklet;
    auto lowerEndValue = bdDataSet.GetField("LowerEndValue")
                           .GetData()
                           .AsArrayHandle<viskores::cont::ArrayHandle<ValueType>>();

    invoke(updateInfoWorklet,
           isLowerLeaf,
           isUpperLeaf,
           inArray,
           lowerEndValue,
           this->TopVolumeData.BranchSaddleEpsilon,
           branchSaddleIsoValue);
    this->TopVolumeData.BranchSaddleIsoValue = branchSaddleIsoValue;
  };

  upperEndValue.CastAndCallForTypes<viskores::TypeListScalarAll, viskores::cont::StorageListBasic>(
    resolveArray);

  // Compute the branch volume based on the upper/lower end superarc volumes
  viskores::worklet::contourtree_augmented::IdArrayType branchVolume;
  viskores::worklet::scalar_topology::select_top_volume_branches::GetBranchVolumeWorklet
    getBranchVolumeWorklet(totalVolume);

  invoke(getBranchVolumeWorklet,  // worklet
         lowerEndSuperarcId,      // input
         lowerEndIntrinsicVolume, // input
         lowerEndDependentVolume, // input
         upperEndSuperarcId,      // input
         upperEndIntrinsicVolume, // input
         upperEndDependentVolume, // input
         isLowerLeaf,
         isUpperLeaf,
         branchVolume); // output

#ifdef DEBUG_PRINT
  std::stringstream resultStream;
  resultStream << "Branch Volume In The Block" << std::endl;
  const viskores::Id nVolume = branchVolume.GetNumberOfValues();
  viskores::worklet::contourtree_augmented::PrintHeader(nVolume, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "BranchVolume", branchVolume, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "isLowerLeaf", isLowerLeaf, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "isUpperLeaf", isUpperLeaf, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "LowerEndIntrinsicVol", lowerEndIntrinsicVolume, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "LowerEndDependentVol", lowerEndDependentVolume, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "UpperEndIntrinsicVol", upperEndIntrinsicVolume, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "UpperEndDependentVol", upperEndDependentVolume, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "LowerEndSuperarc", lowerEndSuperarcId, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "UpperEndSuperarc", upperEndSuperarcId, -1, resultStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "BranchRoot", branchRoot, -1, resultStream);
  resultStream << std::endl;
  VISKORES_LOG_S(viskores::cont::LogLevel::Info, resultStream.str());
#endif

  viskores::cont::Algorithm::Copy(branchVolume, this->TopVolumeData.BranchVolume);

  const viskores::Id nBranches = lowerEndSuperarcId.GetNumberOfValues();
  viskores::cont::ArrayHandleIndex branchesIdx(nBranches);
  viskores::worklet::contourtree_augmented::IdArrayType sortedBranches;
  viskores::cont::Algorithm::Copy(branchesIdx, sortedBranches);

  // sort the branch volume
  viskores::cont::Algorithm::SortByKey(branchVolume, sortedBranches, viskores::SortGreater());
  viskores::cont::Algorithm::Copy(sortedBranches, this->TopVolumeData.SortedBranchByVolume);
}

// Select the local top K branches by volume
void SelectTopVolumeBranchesBlock::SelectLocalTopVolumeBranches(
  const viskores::cont::DataSet& bdDataset,
  const viskores::Id nSavedBranches)
{
  using viskores::worklet::contourtree_augmented::IdArrayType;
  // copy the top volume branches into a smaller array
  // we skip index 0 because it must be the main branch (which has the highest volume)
  viskores::Id nActualSavedBranches =
    std::min(nSavedBranches, this->TopVolumeData.SortedBranchByVolume.GetNumberOfValues() - 1);

  viskores::worklet::contourtree_augmented::IdArrayType topVolumeBranch;
  viskores::cont::Algorithm::CopySubRange(
    this->TopVolumeData.SortedBranchByVolume, 1, nActualSavedBranches, topVolumeBranch);

  auto branchRootByBranch = bdDataset.GetField("BranchRootByBranch")
                              .GetData()
                              .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();

  const viskores::Id nBranches = branchRootByBranch.GetNumberOfValues();

  auto branchRootGRId = bdDataset.GetField("BranchRootGRId")
                          .GetData()
                          .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();

  auto upperEndGRId = bdDataset.GetField("UpperEndGlobalRegularIds")
                        .GetData()
                        .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();

  auto lowerEndGRId = bdDataset.GetField("LowerEndGlobalRegularIds")
                        .GetData()
                        .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();

  viskores::cont::Algorithm::Copy(branchRootByBranch, this->TopVolumeData.BranchRootByBranch);
  viskores::cont::Algorithm::Copy(branchRootGRId, this->TopVolumeData.BranchRootGRId);

  // This seems weird, but we temporarily put the initialization of computing the branch decomposition tree here
  this->TopVolumeData.IsParentBranch.AllocateAndFill(nBranches, false);

  // we permute all branch information to align with the order by volume
  viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id, IdArrayType>(
    branchRootGRId, topVolumeBranch, this->TopVolumeData.TopVolumeBranchRootGRId);

  viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id, IdArrayType>(
    upperEndGRId, topVolumeBranch, this->TopVolumeData.TopVolumeBranchUpperEndGRId);

  viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id, IdArrayType>(
    lowerEndGRId, topVolumeBranch, this->TopVolumeData.TopVolumeBranchLowerEndGRId);

  viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id, IdArrayType>(
    this->TopVolumeData.BranchVolume, topVolumeBranch, this->TopVolumeData.TopVolumeBranchVolume);

  viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id, IdArrayType>(
    this->TopVolumeData.BranchSaddleEpsilon,
    topVolumeBranch,
    this->TopVolumeData.TopVolumeBranchSaddleEpsilon);

  auto resolveArray = [&](const auto& inArray)
  {
    using InArrayHandleType = std::decay_t<decltype(inArray)>;
    InArrayHandleType topVolBranchSaddleIsoValue;
    viskores::worklet::contourtree_augmented::PermuteArrayWithRawIndex<InArrayHandleType>(
      inArray, topVolumeBranch, topVolBranchSaddleIsoValue);
    this->TopVolumeData.TopVolumeBranchSaddleIsoValue = topVolBranchSaddleIsoValue;
  };

  this->TopVolumeData.BranchSaddleIsoValue
    .CastAndCallForTypes<viskores::TypeListScalarAll, viskores::cont::StorageListBasic>(
      resolveArray);
}

void SelectTopVolumeBranchesBlock::ComputeTopVolumeBranchHierarchy(
  const viskores::cont::DataSet& bdDataSet)
{
  this->BDTMaker.ComputeTopVolumeBranchHierarchy(bdDataSet, this->TopVolumeData);
}

viskores::Id SelectTopVolumeBranchesBlock::ExcludeTopVolumeBranchByThreshold(
  const viskores::Id presimplifyThreshold)
{
  using viskores::worklet::contourtree_augmented::IdArrayType;

  // the stencil for top-volume branches of whether passing the threshold
  viskores::cont::ArrayHandle<bool> topVolumeAboveThreshold;
  topVolumeAboveThreshold.AllocateAndFill(
    this->TopVolumeData.TopVolumeBranchVolume.GetNumberOfValues(), true);

  viskores::cont::Invoker invoke;
  viskores::worklet::scalar_topology::select_top_volume_branches::AboveThresholdWorklet
    aboveThresholdWorklet(presimplifyThreshold);
  invoke(aboveThresholdWorklet, this->TopVolumeData.TopVolumeBranchVolume, topVolumeAboveThreshold);

  // using the stencil to filter the top-volume branch information
  IdArrayType filteredTopVolumeBranchRootGRId;
  viskores::cont::Algorithm::CopyIf(this->TopVolumeData.TopVolumeBranchRootGRId,
                                    topVolumeAboveThreshold,
                                    filteredTopVolumeBranchRootGRId);
  viskores::cont::Algorithm::Copy(filteredTopVolumeBranchRootGRId,
                                  this->TopVolumeData.TopVolumeBranchRootGRId);

  IdArrayType filteredTopVolumeBranchVolume;
  viskores::cont::Algorithm::CopyIf(this->TopVolumeData.TopVolumeBranchVolume,
                                    topVolumeAboveThreshold,
                                    filteredTopVolumeBranchVolume);
  viskores::cont::Algorithm::Copy(filteredTopVolumeBranchVolume,
                                  this->TopVolumeData.TopVolumeBranchVolume);

  auto resolveArray = [&](auto& inArray)
  {
    using InArrayHandleType = std::decay_t<decltype(inArray)>;
    InArrayHandleType filteredTopVolumeBranchSaddleIsoValue;
    viskores::cont::Algorithm::CopyIf(
      inArray, topVolumeAboveThreshold, filteredTopVolumeBranchSaddleIsoValue);

    inArray.Allocate(filteredTopVolumeBranchVolume.GetNumberOfValues());
    viskores::cont::Algorithm::Copy(filteredTopVolumeBranchSaddleIsoValue, inArray);
  };
  this->TopVolumeData.TopVolumeBranchSaddleIsoValue
    .CastAndCallForTypes<viskores::TypeListScalarAll, viskores::cont::StorageListBasic>(
      resolveArray);

  IdArrayType filteredTopVolumeBranchSaddleEpsilon;
  viskores::cont::Algorithm::CopyIf(this->TopVolumeData.TopVolumeBranchSaddleEpsilon,
                                    topVolumeAboveThreshold,
                                    filteredTopVolumeBranchSaddleEpsilon);
  viskores::cont::Algorithm::Copy(filteredTopVolumeBranchSaddleEpsilon,
                                  this->TopVolumeData.TopVolumeBranchSaddleEpsilon);

  IdArrayType filteredTopVolumeBranchUpperEndGRId;
  viskores::cont::Algorithm::CopyIf(this->TopVolumeData.TopVolumeBranchUpperEndGRId,
                                    topVolumeAboveThreshold,
                                    filteredTopVolumeBranchUpperEndGRId);
  viskores::cont::Algorithm::Copy(filteredTopVolumeBranchUpperEndGRId,
                                  this->TopVolumeData.TopVolumeBranchUpperEndGRId);

  IdArrayType filteredTopVolumeBranchLowerEndGRId;
  viskores::cont::Algorithm::CopyIf(this->TopVolumeData.TopVolumeBranchLowerEndGRId,
                                    topVolumeAboveThreshold,
                                    filteredTopVolumeBranchLowerEndGRId);
  viskores::cont::Algorithm::Copy(filteredTopVolumeBranchLowerEndGRId,
                                  this->TopVolumeData.TopVolumeBranchLowerEndGRId);

  return filteredTopVolumeBranchVolume.GetNumberOfValues();
}

} // namespace internal
} // namespace scalar_topology
} // namespace filter
} // namespace viskores
