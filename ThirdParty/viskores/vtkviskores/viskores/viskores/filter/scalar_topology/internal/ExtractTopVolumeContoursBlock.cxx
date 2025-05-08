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
#include <viskores/filter/scalar_topology/internal/ExtractTopVolumeContoursBlock.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ArrayTransforms.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/DataSetMesh.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_contour_tree/FindSuperArcForUnknownNode.h>
#include <viskores/filter/scalar_topology/worklet/extract_top_volume_contours/CopyConstArraysWorklet.h>
#include <viskores/filter/scalar_topology/worklet/extract_top_volume_contours/GetCellCasesWorklet.h>
#include <viskores/filter/scalar_topology/worklet/extract_top_volume_contours/GetEdgesInCellWorklet.h>
#include <viskores/filter/scalar_topology/worklet/extract_top_volume_contours/GetSuperarcByIsoValueWorklet.h>
#include <viskores/filter/scalar_topology/worklet/extract_top_volume_contours/Types.h>
#include <viskores/filter/scalar_topology/worklet/select_top_volume_branches/AssignValueWorklet.h>
#include <viskores/filter/scalar_topology/worklet/select_top_volume_branches/BinarySearchWorklet.h>

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

ExtractTopVolumeContoursBlock::ExtractTopVolumeContoursBlock(viskores::Id localBlockNo,
                                                             int globalBlockId)
  : LocalBlockNo(localBlockNo)
  , GlobalBlockId(globalBlockId)
{
}

void ExtractTopVolumeContoursBlock::ExtractIsosurfaceOnSelectedBranch(
  const viskores::cont::DataSet& dataSet,
  const bool isMarchingCubes,
  const bool shiftIsovalueByEpsilon,
  const viskores::cont::LogLevel timingsLogLevel)
{
  using viskores::worklet::contourtree_augmented::IdArrayType;

  this->TopVolumeData.TopVolumeBranchRootGRId =
    dataSet.GetField("TopVolumeBranchGlobalRegularIds").GetData().AsArrayHandle<IdArrayType>();
  // if no branch to extract from
  if (this->TopVolumeData.TopVolumeBranchRootGRId.GetNumberOfValues() < 1)
    return;

  // branch root global regular ID
  // size: nBranches
  // usage: identifier of the branch
  viskores::cont::Algorithm::Copy(
    dataSet.GetField("BranchRootGRId").GetData().AsArrayHandle<IdArrayType>(),
    this->TopVolumeData.BranchRootGRId);

  // branch local upper end and lower end
  // size: nBranches
  // usage: search for the superarc of an arbitrary point (not necessarily on grid)
  auto upperEndLocalIds =
    dataSet.GetField("UpperEndLocalIds").GetData().AsArrayHandle<IdArrayType>();
  auto lowerEndLocalIds =
    dataSet.GetField("LowerEndLocalIds").GetData().AsArrayHandle<IdArrayType>();

  // global regular ids
  auto globalRegularIds =
    dataSet.GetField("RegularNodeGlobalIds").GetData().AsArrayHandle<IdArrayType>();

  // Extracting the mesh id information.
  // Because most data arrays include nodes in other blocks,
  // we need reference to the mesh id of nodes that are actually inside the block.
  viskores::Id3 globalPointDimensions;
  viskores::Id3 pointDimensions, globalPointIndexStart;

  dataSet.GetCellSet().CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>(
    viskores::worklet::contourtree_augmented::GetLocalAndGlobalPointDimensions(),
    pointDimensions,
    globalPointDimensions,
    globalPointIndexStart);

#ifdef DEBUG_PRINT
  VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Block size info");
  {
    std::stringstream rs;
    rs << "globalPointDimensions: " << globalPointDimensions << std::endl;
    rs << "pointDimensions: " << pointDimensions << std::endl;
    rs << "globalPointIndexStart: " << globalPointIndexStart << std::endl;
    rs << "globalRegularIDs: " << globalRegularIds.GetNumberOfValues() << std::endl;
    // ds.PrintSummary(rs);
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, rs.str());
  }
#endif

  // Tool to relabel local mesh ids to global ids
  auto localToGlobalIdRelabeler = viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler(
    globalPointIndexStart, pointDimensions, globalPointDimensions);
  IdArrayType globalIdsByMesh;

  // Note: the cell set is different from the mesh structure.
  // Here, we assume that the cell set is structured grid.
  // A more general way to do this is to use CellSet().GetCellPointIds(i)
  // to extract all the local ids and keep unique ones

  // local ids in the mesh
  IdArrayType localIdsByMesh;
  viskores::cont::Algorithm::Copy(
    viskores::cont::ArrayHandleIndex(pointDimensions[0] * pointDimensions[1] * pointDimensions[2]),
    localIdsByMesh);
  // then, we transform the local ids to global ids
  auto localTransformToGlobalId =
    viskores::cont::make_ArrayHandleTransform(localIdsByMesh, localToGlobalIdRelabeler);
  viskores::cont::ArrayCopyDevice(localTransformToGlobalId, globalIdsByMesh);

  // detect whether the element in globalRegularIds are in the block
  // globalIdsDiscard is just a filler for the worklet format. We do not use it.
  // The last slot for the worklet is useful in a later step.
  // Here we just reuse the worklet
  IdArrayType globalIdsWithinBlockStencil;
  viskores::cont::ArrayHandleDiscard<viskores::Id> globalIdsDiscard;

  viskores::cont::Invoker invoke;
  // stencil is 1 if the global regular id is within the block, 0 otherwise
  // TODO/FIXME: A way to do binary search using built-in algorithms is LowerBound+UpperBound -> Check if identical
  // Not sure if that is faster.
  auto binarySearchWorklet =
    viskores::worklet::scalar_topology::select_top_volume_branches::BinarySearchWorklet();
  invoke(binarySearchWorklet,
         globalRegularIds,
         globalIdsByMesh,
         globalIdsWithinBlockStencil,
         globalIdsDiscard);

  this->TopVolumeData.TopVolumeBranchSaddleIsoValue =
    dataSet.GetField("TopVolumeBranchSaddleIsoValue").GetData();

  auto resolveArray = [&](const auto& inArray)
  {
    using InArrayHandleType = std::decay_t<decltype(inArray)>;
    using ValueType = typename InArrayHandleType::ValueType;

    // we need to sort all values based on the global ids
    // and remove values of points that do not belong to the local block
    auto dataValues = dataSet.GetField("DataValues").GetData().AsArrayHandle<InArrayHandleType>();

    IdArrayType globalIdsWithinBlock;
    IdArrayType localIdsWithinBlock;
    InArrayHandleType dataValuesWithinBlock;

    // filter global regular ids, array ids, and data values
    viskores::cont::Algorithm::CopyIf(
      globalRegularIds, globalIdsWithinBlockStencil, globalIdsWithinBlock);
    viskores::cont::Algorithm::CopyIf(
      viskores::cont::ArrayHandleIndex(globalRegularIds.GetNumberOfValues()),
      globalIdsWithinBlockStencil,
      localIdsWithinBlock);
    viskores::cont::Algorithm::CopyIf(
      dataValues, globalIdsWithinBlockStencil, dataValuesWithinBlock);

    // sorted index based on global regular ids
    IdArrayType sortedGlobalIds;
    viskores::cont::Algorithm::Copy(
      viskores::cont::ArrayHandleIndex(globalIdsByMesh.GetNumberOfValues()), sortedGlobalIds);
    viskores::cont::Algorithm::SortByKey(globalIdsWithinBlock, sortedGlobalIds);

    // globalIdsWithinBlock (sorted) and globalIdsByMesh should be identical.
    // computing globalIdsWithinBlock ensures the input data is correct
    bool identical =
      globalIdsWithinBlock.GetNumberOfValues() == globalIdsByMesh.GetNumberOfValues();
    if (identical)
    {
      viskores::cont::ArrayHandle<bool> globalIdsIdentical;
      viskores::cont::Algorithm::Transform(
        globalIdsWithinBlock, globalIdsByMesh, globalIdsIdentical, viskores::Equal());
      identical =
        viskores::cont::Algorithm::Reduce(globalIdsIdentical, true, viskores::LogicalAnd());
    }
    if (!identical)
    {
      viskores::worklet::contourtree_augmented::PrintHeader(globalIdsByMesh.GetNumberOfValues());
      viskores::worklet::contourtree_augmented::PrintIndices("globalIdsByMesh", globalIdsByMesh);
      viskores::worklet::contourtree_augmented::PrintHeader(
        globalIdsWithinBlock.GetNumberOfValues());
      viskores::worklet::contourtree_augmented::PrintIndices("globalIdsWithinBlock",
                                                             globalIdsWithinBlock);
    }
    VISKORES_ASSERT(identical);

    // filtered and sorted local node info ids
    // i.e. index of global regular ids, data values, and superparents
    // Note: This is not local mesh id! Make sure to distinguish them
    IdArrayType sortedLocalNodeInfoIdsWithinBlock;
    viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id,
                                                                          IdArrayType>(
      localIdsWithinBlock, sortedGlobalIds, sortedLocalNodeInfoIdsWithinBlock);

    // sorted data values
    // for simulation of simplicity, we also need sorted global regular IDs in globalIdsWithinBlock
    InArrayHandleType sortedDataValuesWithinBlock;
    viskores::worklet::contourtree_augmented::PermuteArrayWithRawIndex<InArrayHandleType>(
      dataValuesWithinBlock, sortedGlobalIds, sortedDataValuesWithinBlock);

    // create an execution object to find the superarc for an arbitrary point within the mesh
    // all information below are required to initialize the execution object
    auto superparents = dataSet.GetField("Superparents").GetData().AsArrayHandle<IdArrayType>();
    auto supernodes = dataSet.GetField("Supernodes").GetData().AsArrayHandle<IdArrayType>();
    auto superarcs = dataSet.GetField("Superarcs").GetData().AsArrayHandle<IdArrayType>();
    auto superchildren = dataSet.GetField("Superchildren").GetData().AsArrayHandle<IdArrayType>();
    auto whichRound = dataSet.GetField("WhichRound").GetData().AsArrayHandle<IdArrayType>();
    auto whichIteration = dataSet.GetField("WhichIteration").GetData().AsArrayHandle<IdArrayType>();
    auto hyperparents = dataSet.GetField("Hyperparents").GetData().AsArrayHandle<IdArrayType>();
    auto hypernodes = dataSet.GetField("Hypernodes").GetData().AsArrayHandle<IdArrayType>();
    auto hyperarcs = dataSet.GetField("Hyperarcs").GetData().AsArrayHandle<IdArrayType>();

    // filtered + sorted superparents of nodes
    IdArrayType superparentsWithinBlock;
    viskores::cont::Algorithm::CopyIf(
      superparents, globalIdsWithinBlockStencil, superparentsWithinBlock);
    IdArrayType sortedSuperparentsWithinBlock;
    viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id,
                                                                          IdArrayType>(
      superparentsWithinBlock, sortedGlobalIds, sortedSuperparentsWithinBlock);

    // initialize the exec object
    // note: terms should include the contour tree as much as possible
    // should pass the full arrays to the object instead of the filtered ones
    auto findSuperarcForNode =
      viskores::worklet::contourtree_distributed::FindSuperArcForUnknownNode<ValueType>(
        superparents,
        supernodes,
        superarcs,
        superchildren,
        whichRound,
        whichIteration,
        hyperparents,
        hypernodes,
        hyperarcs,
        globalRegularIds,
        dataValues);

    // let's check which branches are known by the block
    // We check the branchGRId of top volume branches to see whether there are matches within the block
    viskores::Id nIsoValues = inArray.GetNumberOfValues();
    viskores::Id totalNumPoints =
      globalPointDimensions[0] * globalPointDimensions[1] * globalPointDimensions[2];

    // dropping out top-volume branches that are not known by the block
    this->TopVolumeData.TopVolBranchKnownByBlockStencil =
      dataSet.GetField("TopVolumeBranchKnownByBlockStencil").GetData().AsArrayHandle<IdArrayType>();
    // index of top-volume branches within the block among all top-volume branches
    IdArrayType topVolBranchWithinBlockId;
    viskores::cont::Algorithm::CopyIf(viskores::cont::ArrayHandleIndex(nIsoValues),
                                      this->TopVolumeData.TopVolBranchKnownByBlockStencil,
                                      topVolBranchWithinBlockId);
    auto topVolBranchWithinBlockIdPortal = topVolBranchWithinBlockId.ReadPortal();

    viskores::Id nTopVolBranchWithinBlock = topVolBranchWithinBlockId.GetNumberOfValues();

    // filtered branch saddle values
    InArrayHandleType isoValues;
    viskores::cont::Algorithm::CopyIf(
      inArray, this->TopVolumeData.TopVolBranchKnownByBlockStencil, isoValues);
    auto isoValuePortal = isoValues.ReadPortal();

    this->TopVolumeData.TopVolumeBranchSaddleEpsilon =
      dataSet.GetField("TopVolumeBranchSaddleEpsilon").GetData().AsArrayHandle<IdArrayType>();
    // filtered branch saddle epsilons
    IdArrayType topVolBranchSaddleEpsilons;
    viskores::cont::Algorithm::CopyIf(this->TopVolumeData.TopVolumeBranchSaddleEpsilon,
                                      this->TopVolumeData.TopVolBranchKnownByBlockStencil,
                                      topVolBranchSaddleEpsilons);
    auto topVolBranchSaddleEpsilonPortal = topVolBranchSaddleEpsilons.ReadPortal();

    this->TopVolumeData.TopVolBranchInfoActualIndex =
      dataSet.GetField("TopVolumeBranchInformationIndex").GetData().AsArrayHandle<IdArrayType>();
    this->TopVolumeData.IsParentBranch = dataSet.GetField("IsParentBranch")
                                           .GetData()
                                           .AsArrayHandle<viskores::cont::ArrayHandle<bool>>();
    // for each top-vol branch in the block
    // we get their upper end and lower end local ids
    IdArrayType topVolLocalBranchUpperEnd;
    IdArrayType topVolLocalBranchLowerEnd;
    viskores::cont::ArrayHandle<bool> topVolIsParent;
    viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id,
                                                                          IdArrayType>(
      upperEndLocalIds, this->TopVolumeData.TopVolBranchInfoActualIndex, topVolLocalBranchUpperEnd);
    viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id,
                                                                          IdArrayType>(
      lowerEndLocalIds, this->TopVolumeData.TopVolBranchInfoActualIndex, topVolLocalBranchLowerEnd);
    viskores::worklet::contourtree_augmented::PermuteArrayWithRawIndex<
      viskores::cont::ArrayHandle<bool>>(this->TopVolumeData.IsParentBranch,
                                         this->TopVolumeData.TopVolBranchInfoActualIndex,
                                         topVolIsParent);
    auto topVolIsParentPortal = topVolIsParent.ReadPortal();

    // We compute the global regular IDs of top-vol branch saddles.
    // We extract the contour right above/below the saddle.
    IdArrayType topVolLocalBranchSaddleGRId;
    {
      IdArrayType topVolLocalBranchSaddle;
      viskores::cont::Algorithm::Copy(topVolLocalBranchUpperEnd, topVolLocalBranchSaddle);
      invoke(
        viskores::worklet::scalar_topology::select_top_volume_branches::AssignValueByPositivity{},
        topVolBranchSaddleEpsilons,
        topVolLocalBranchLowerEnd,
        topVolLocalBranchSaddle);
      viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id,
                                                                            IdArrayType>(
        globalRegularIds, topVolLocalBranchSaddle, topVolLocalBranchSaddleGRId);
    }

    // We compute the superarc of the branch within the block
    // around the given isovalue
    IdArrayType branchIsoSuperarcs;
    branchIsoSuperarcs.Allocate(nTopVolBranchWithinBlock);

    viskores::worklet::scalar_topology::extract_top_volume_contours::GetSuperarcByIsoValueWorklet
      branchIsoSuperarcWorklet(totalNumPoints, shiftIsovalueByEpsilon);
    invoke(branchIsoSuperarcWorklet,
           topVolLocalBranchUpperEnd,
           topVolLocalBranchLowerEnd,
           isoValues,
           topVolLocalBranchSaddleGRId,
           topVolBranchSaddleEpsilons,
           branchIsoSuperarcs,
           findSuperarcForNode);
    auto branchIsoSuperarcsPortal = branchIsoSuperarcs.ReadPortal();

#ifdef DEBUG_PRINT
    std::stringstream branchStream;

    branchStream << "Debug for branch info, Block " << this->LocalBlockNo << std::endl;
    viskores::worklet::contourtree_augmented::PrintHeader(sortedBranchGRId.GetNumberOfValues(),
                                                          branchStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Raw Branch GR", this->TopVolumeData.BranchRootGRId, -1, branchStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Raw Upper End", upperLocalEndIds, -1, branchStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Raw Lower End", lowerLocalEndIds, -1, branchStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Sorted Branch GR", sortedBranchGRId, -1, branchStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Sorted Branch Id", sortedBranchOrder, -1, branchStream);

    viskores::worklet::contourtree_augmented::PrintHeader(nIsoValues, branchStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Top Branch GR", this->TopVolumeData.TopVolumeBranchRootGRId, -1, branchStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Top Branch Stencil", topVolBranchWithinBlockStencil, -1, branchStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Top Branch Idx", topVolBranchInfoIdx, -1, branchStream);

    viskores::worklet::contourtree_augmented::PrintHeader(nTopVolBranchKnownByBlock, branchStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Sorted Upper End", topVolBranchUpperLocalEnd, -1, branchStream);
    viskores::worklet::contourtree_augmented::PrintIndices(
      "Sorted Lower End", topVolBranchLowerLocalEnd, -1, branchStream);

    VISKORES_LOG_S(viskores::cont::LogLevel::Info, branchStream.str());
#endif
    this->TopVolumeData.ExtraMaximaBranchLowerEnd =
      dataSet.GetField("ExtraMaximaBranchLowerEnd").GetData().AsArrayHandle<IdArrayType>();
    this->TopVolumeData.ExtraMinimaBranchLowerEnd =
      dataSet.GetField("ExtraMinimaBranchLowerEnd").GetData().AsArrayHandle<IdArrayType>();
    this->TopVolumeData.ExtraMaximaBranchUpperEnd =
      dataSet.GetField("ExtraMaximaBranchUpperEnd").GetData().AsArrayHandle<IdArrayType>();
    this->TopVolumeData.ExtraMinimaBranchUpperEnd =
      dataSet.GetField("ExtraMinimaBranchUpperEnd").GetData().AsArrayHandle<IdArrayType>();
    this->TopVolumeData.ExtraMaximaBranchOrder =
      dataSet.GetField("ExtraMaximaBranchOrder").GetData().AsArrayHandle<IdArrayType>();
    this->TopVolumeData.ExtraMinimaBranchOrder =
      dataSet.GetField("ExtraMinimaBranchOrder").GetData().AsArrayHandle<IdArrayType>();

    const viskores::Id nExtraMaximaBranch =
      this->TopVolumeData.ExtraMaximaBranchLowerEnd.GetNumberOfValues();
    const viskores::Id nExtraMinimaBranch =
      this->TopVolumeData.ExtraMinimaBranchLowerEnd.GetNumberOfValues();
    InArrayHandleType extraMaximaBranchIsoValue;
    InArrayHandleType extraMinimaBranchIsoValue;

    IdArrayType extraMaximaBranchSuperarcs;
    IdArrayType extraMinimaBranchSuperarcs;
    extraMaximaBranchSuperarcs.Allocate(nExtraMaximaBranch);
    extraMinimaBranchSuperarcs.Allocate(nExtraMinimaBranch);

    if (nExtraMaximaBranch)
    {
      extraMaximaBranchIsoValue =
        dataSet.GetField("ExtraMaximaBranchIsoValue").GetData().AsArrayHandle<InArrayHandleType>();
      this->TopVolumeData.ExtraMaximaBranchSaddleGRId =
        dataSet.GetField("ExtraMaximaBranchSaddleGRId").GetData().AsArrayHandle<IdArrayType>();

      invoke(branchIsoSuperarcWorklet,
             this->TopVolumeData.ExtraMaximaBranchUpperEnd,
             this->TopVolumeData.ExtraMaximaBranchLowerEnd,
             extraMaximaBranchIsoValue,
             this->TopVolumeData.ExtraMaximaBranchSaddleGRId,
             viskores::cont::ArrayHandleConstant<viskores::Id>(1, nExtraMaximaBranch),
             extraMaximaBranchSuperarcs,
             findSuperarcForNode);

#ifdef DEBUG_PRINT
      std::stringstream extraMaxStream;
      extraMaxStream << "Debug for Extra Maxima Branch, Block " << this->LocalBlockNo << std::endl;
      viskores::worklet::contourtree_augmented::PrintHeader(nExtraMaximaBranch, extraMaxStream);
      viskores::worklet::contourtree_augmented::PrintIndices(
        "Max Branch Upper End", this->TopVolumeData.ExtraMaximaBranchUpperEnd, -1, extraMaxStream);
      viskores::worklet::contourtree_augmented::PrintIndices(
        "Max Branch Lower End", this->TopVolumeData.ExtraMaximaBranchLowerEnd, -1, extraMaxStream);
      viskores::worklet::contourtree_augmented::PrintValues<ValueType>(
        "Max Branch IsoValue", extraMaximaBranchIsoValue, -1, extraMaxStream);
      viskores::worklet::contourtree_augmented::PrintIndices(
        "Max Branch Superarc", extraMaximaBranchSuperarcs, -1, extraMaxStream);
      viskores::worklet::contourtree_augmented::PrintIndices(
        "Max Branch Order", this->TopVolumeData.ExtraMaximaBranchOrder, -1, extraMaxStream);
      VISKORES_LOG_S(viskores::cont::LogLevel::Info, extraMaxStream.str());
#endif // DEBUG_PRINT
    }

    if (nExtraMinimaBranch)
    {
      extraMinimaBranchIsoValue =
        dataSet.GetField("ExtraMinimaBranchIsoValue").GetData().AsArrayHandle<InArrayHandleType>();
      this->TopVolumeData.ExtraMinimaBranchSaddleGRId =
        dataSet.GetField("ExtraMinimaBranchSaddleGRId").GetData().AsArrayHandle<IdArrayType>();

      invoke(branchIsoSuperarcWorklet,
             this->TopVolumeData.ExtraMinimaBranchUpperEnd,
             this->TopVolumeData.ExtraMinimaBranchLowerEnd,
             extraMinimaBranchIsoValue,
             this->TopVolumeData.ExtraMinimaBranchSaddleGRId,
             viskores::cont::ArrayHandleConstant<viskores::Id>(-1, nExtraMinimaBranch),
             extraMinimaBranchSuperarcs,
             findSuperarcForNode);

#ifdef DEBUG_PRINT
      std::stringstream extraMinStream;
      extraMaxStream << "Debug for Extra Maxima Branch, Block " << this->LocalBlockNo << std::endl;
      viskores::worklet::contourtree_augmented::PrintHeader(nExtraMinimaBranch, extraMinStream);
      viskores::worklet::contourtree_augmented::PrintIndices(
        "Max Branch Upper End", this->TopVolumeData.ExtraMinimaBranchUpperEnd, -1, extraMinStream);
      viskores::worklet::contourtree_augmented::PrintIndices(
        "Max Branch Lower End", this->TopVolumeData.ExtraMinimaBranchLowerEnd, -1, extraMinStream);
      viskores::worklet::contourtree_augmented::PrintValues<ValueType>(
        "Max Branch IsoValue", extraMinimaBranchIsoValue, -1, extraMinStream);
      viskores::worklet::contourtree_augmented::PrintIndices(
        "Max Branch Superarc", extraMinimaBranchSuperarcs, -1, extraMinStream);
      viskores::worklet::contourtree_augmented::PrintIndices(
        "Max Branch Order", this->TopVolumeData.ExtraMinimaBranchOrder, -1, extraMinStream);
      VISKORES_LOG_S(viskores::cont::LogLevel::Info, extraMinStream.str());
#endif // DEBUG_PRINT
    }

    auto extraMaximaBranchSuperarcPortal = extraMaximaBranchSuperarcs.ReadPortal();
    auto extraMinimaBranchSuperarcPortal = extraMinimaBranchSuperarcs.ReadPortal();
    auto extraMaximaBranchIsoValuePortal = extraMaximaBranchIsoValue.ReadPortal();
    auto extraMinimaBranchIsoValuePortal = extraMinimaBranchIsoValue.ReadPortal();
    auto extraMaximaBranchOrderPortal = this->TopVolumeData.ExtraMaximaBranchOrder.ReadPortal();
    auto extraMinimaBranchOrderPortal = this->TopVolumeData.ExtraMinimaBranchOrder.ReadPortal();

    // Update 01/09/2025
    // Adding the branch saddle global regular ID portals for simulation of simplicity when
    // computing the cell polarity cases and draw contour lines.
    auto topVolLocalBranchSaddleGRIdPortal = topVolLocalBranchSaddleGRId.ReadPortal();
    auto extraMaximaBranchSaddleGRIdPortal =
      this->TopVolumeData.ExtraMaximaBranchSaddleGRId.ReadPortal();
    auto extraMinimaBranchSaddleGRIdPortal =
      this->TopVolumeData.ExtraMinimaBranchSaddleGRId.ReadPortal();

    const viskores::Id nContours =
      nTopVolBranchWithinBlock + nExtraMaximaBranch + nExtraMinimaBranch;
    this->IsosurfaceEdgesOffset.AllocateAndFill(nContours, 0);
    this->IsosurfaceEdgesLabels.AllocateAndFill(nContours, 0);
    this->IsosurfaceEdgesOrders.AllocateAndFill(nContours, 0);
    this->IsosurfaceGRIds.AllocateAndFill(nContours, 0);
    InArrayHandleType isosurfaceIsoValue;
    isosurfaceIsoValue.AllocateAndFill(nContours, static_cast<ValueType>(0));
    auto edgeOffsetWritePortal = this->IsosurfaceEdgesOffset.WritePortal();
    auto edgeLabelWritePortal = this->IsosurfaceEdgesLabels.WritePortal();
    auto edgeOrderWritePortal = this->IsosurfaceEdgesOrders.WritePortal();
    auto globalRegularIdsWritePortal = this->IsosurfaceGRIds.WritePortal();
    auto isosurfaceValuePortal = isosurfaceIsoValue.WritePortal();

    viskores::Id nContourCandidateMeshes = 0;
    // NOTE: nContours denotes the number of isosurfaces for visualization.
    // The number is usually small, so linear loop is not too costly.
    // NOTE update 06/16/2024: We always need the isovalue of the contour.
    // As a result, we iterate through nContours (=O(k)).
    // This may be parallelizable in future work
    for (viskores::Id branchIdx = 0; branchIdx < nContours; branchIdx++)
    {
      ValueType isoValue;
      viskores::Id currBranchSaddleEpsilon, branchSuperarc, branchOrder, branchSaddleGRId,
        branchLabel = 0;

      using viskores::worklet::scalar_topology::extract_top_volume_contours::BRANCH_SADDLE;
      using viskores::worklet::scalar_topology::extract_top_volume_contours::BRANCH_COVER;
      using viskores::worklet::scalar_topology::extract_top_volume_contours::MAXIMA_CONTOUR;

      if (branchIdx < nTopVolBranchWithinBlock)
      {
        isoValue = isoValuePortal.Get(branchIdx);
        currBranchSaddleEpsilon = topVolBranchSaddleEpsilonPortal.Get(branchIdx);
        branchSuperarc = branchIsoSuperarcsPortal.Get(branchIdx);
        branchSaddleGRId = topVolLocalBranchSaddleGRIdPortal.Get(branchIdx);
        branchOrder = topVolBranchWithinBlockIdPortal.Get(branchIdx) + 1;
        branchLabel |= BRANCH_SADDLE;
        branchLabel |= topVolIsParentPortal.Get(branchIdx) ? BRANCH_COVER : 0;
        branchLabel |= currBranchSaddleEpsilon > 0 ? MAXIMA_CONTOUR : 0;
      }
      else if (branchIdx < nTopVolBranchWithinBlock + nExtraMaximaBranch)
      {
        const viskores::Id idx = branchIdx - nTopVolBranchWithinBlock;
        isoValue = extraMaximaBranchIsoValuePortal.Get(idx);
        currBranchSaddleEpsilon = 1;
        branchSuperarc = extraMaximaBranchSuperarcPortal.Get(idx);
        branchSaddleGRId = extraMaximaBranchSaddleGRIdPortal.Get(idx);
        branchOrder = extraMaximaBranchOrderPortal.Get(idx);
        branchLabel |= MAXIMA_CONTOUR;
      }
      else
      {
        const viskores::Id idx = branchIdx - nTopVolBranchWithinBlock - nExtraMaximaBranch;
        VISKORES_ASSERT(idx < nExtraMinimaBranch);
        isoValue = extraMinimaBranchIsoValuePortal.Get(idx);
        currBranchSaddleEpsilon = -1;
        branchSuperarc = extraMinimaBranchSuperarcPortal.Get(idx);
        branchSaddleGRId = extraMinimaBranchSaddleGRIdPortal.Get(idx);
        branchOrder = extraMinimaBranchOrderPortal.Get(idx);
      }

      edgeOffsetWritePortal.Set(branchIdx, this->IsosurfaceEdgesFrom.GetNumberOfValues());
      edgeLabelWritePortal.Set(branchIdx, branchLabel);
      edgeOrderWritePortal.Set(branchIdx, branchOrder);
      globalRegularIdsWritePortal.Set(branchIdx, branchSaddleGRId);
      isosurfaceValuePortal.Set(branchIdx, isoValue);

      if (viskores::worklet::contourtree_augmented::NoSuchElement(branchSuperarc))
      {
        continue;
      }

      // Note: by concept, there is no 3D cell if pointDimensions[2] <= 1
      const bool isData2D = globalPointDimensions[2] <= 1;
      viskores::Id nCells = isData2D
        ? (pointDimensions[0] - 1) * (pointDimensions[1] - 1)
        : (pointDimensions[0] - 1) * (pointDimensions[1] - 1) * (pointDimensions[2] - 1);

      // we get the polarity cases of cells
      // we use lookup tables to for fast processing
      using viskores::worklet::scalar_topology::extract_top_volume_contours::
        CopyConstArraysForMarchingCubesDataTablesWorklet;
      using viskores::worklet::scalar_topology::extract_top_volume_contours::COPY_VERTEXOFFSET;
      using viskores::worklet::scalar_topology::extract_top_volume_contours::COPY_EDGETABLE;
      using viskores::worklet::scalar_topology::extract_top_volume_contours::COPY_NUMBOUNDTABLE;
      using viskores::worklet::scalar_topology::extract_top_volume_contours::COPY_BOUNDARYTABLE;
      using viskores::worklet::scalar_topology::extract_top_volume_contours::COPY_LABELEDGETABLE;

      using viskores::worklet::scalar_topology::extract_top_volume_contours::nVertices2d;
      using viskores::worklet::scalar_topology::extract_top_volume_contours::nVertices3d;

      IdArrayType vertexOffSetTable;
      const viskores::Id vertexOffsetSize = isData2D ? nVertices2d * 2 : nVertices3d * 3;

      CopyConstArraysForMarchingCubesDataTablesWorklet copyVertexOffset(
        isData2D, isMarchingCubes, COPY_VERTEXOFFSET);
      invoke(
        copyVertexOffset, viskores::cont::ArrayHandleIndex(vertexOffsetSize), vertexOffSetTable);

      using viskores::worklet::scalar_topology::extract_top_volume_contours::nEdges2d;
      using viskores::worklet::scalar_topology::extract_top_volume_contours::nEdgesMC3d;
      using viskores::worklet::scalar_topology::extract_top_volume_contours::nEdgesLT3d;

      IdArrayType edgeTable;
      const viskores::Id edgeTableSize =
        isData2D ? nEdges2d * 2 : (isMarchingCubes ? nEdgesMC3d * 2 : nEdgesLT3d * 2);
      CopyConstArraysForMarchingCubesDataTablesWorklet copyEdgeTable(
        isData2D, isMarchingCubes, COPY_EDGETABLE);
      invoke(copyEdgeTable, viskores::cont::ArrayHandleIndex(edgeTableSize), edgeTable);

      using viskores::worklet::scalar_topology::extract_top_volume_contours::nCases2d;
      using viskores::worklet::scalar_topology::extract_top_volume_contours::nCasesMC3d;
      using viskores::worklet::scalar_topology::extract_top_volume_contours::nCasesLT3d;

      IdArrayType numBoundTable;
      const viskores::Id numBoundTableSize =
        isData2D ? (nCases2d) : (isMarchingCubes ? (nCasesMC3d) : (nCasesLT3d));
      CopyConstArraysForMarchingCubesDataTablesWorklet copyNumBoundTable(
        isData2D, isMarchingCubes, COPY_NUMBOUNDTABLE);
      invoke(copyNumBoundTable, viskores::cont::ArrayHandleIndex(numBoundTableSize), numBoundTable);

      using viskores::worklet::scalar_topology::extract_top_volume_contours::nLineTableElemSize2d;
      using viskores::worklet::scalar_topology::extract_top_volume_contours::nTriTableMC3dElemSize;
      using viskores::worklet::scalar_topology::extract_top_volume_contours::nTriTableLT3dElemSize;

      IdArrayType boundaryTable;
      const viskores::Id boundaryTableSize = isData2D
        ? (nCases2d * nLineTableElemSize2d)
        : (isMarchingCubes ? (nCasesMC3d * nTriTableMC3dElemSize)
                           : (nCasesLT3d * nTriTableLT3dElemSize));
      CopyConstArraysForMarchingCubesDataTablesWorklet copyBoundaryTable(
        isData2D, isMarchingCubes, COPY_BOUNDARYTABLE);
      invoke(copyBoundaryTable, viskores::cont::ArrayHandleIndex(boundaryTableSize), boundaryTable);

      using viskores::worklet::scalar_topology::extract_top_volume_contours::
        nLabelEdgeTableMC3dElemSize;
      using viskores::worklet::scalar_topology::extract_top_volume_contours::
        nLabelEdgeTableLT3dElemSize;

      IdArrayType labelEdgeTable;
      const viskores::Id labelEdgeTableSize = isData2D
        ? 0
        : (isMarchingCubes ? nCasesMC3d * nLabelEdgeTableMC3dElemSize
                           : nCasesLT3d * nLabelEdgeTableLT3dElemSize);
      labelEdgeTable.Allocate(labelEdgeTableSize);
      if (!isData2D)
      {
        CopyConstArraysForMarchingCubesDataTablesWorklet copyLabelEdgeTable(
          false, isMarchingCubes, COPY_LABELEDGETABLE);
        invoke(
          copyLabelEdgeTable, viskores::cont::ArrayHandleIndex(labelEdgeTableSize), labelEdgeTable);
      }

      //IdArrayType vertexOffset = globalPointDimensions[2] <= 1
      //  ? viskores::worklet::scalar_topology::extract_top_volume_contours::vertexOffset2d
      //  : viskores::worklet::scalar_topology::extract_top_volume_contours::vertexOffset3d;
      //IdArrayType edgeTable = globalPointDimensions[2] <= 1
      //  ? viskores::worklet::scalar_topology::extract_top_volume_contours::edgeTable2d
      //  : (isMarchingCubes
      //       ? viskores::worklet::scalar_topology::extract_top_volume_contours::edgeTableMC3d
      //       : viskores::worklet::scalar_topology::extract_top_volume_contours::edgeTableLT3d);
      //IdArrayType numBoundTable = globalPointDimensions[2] <= 1
      //  ? viskores::worklet::scalar_topology::extract_top_volume_contours::numLinesTable2d
      //  : (isMarchingCubes
      //       ? viskores::worklet::scalar_topology::extract_top_volume_contours::numTrianglesTableMC3d
      //       : viskores::worklet::scalar_topology::extract_top_volume_contours::numTrianglesTableLT3d);
      //IdArrayType boundaryTable = globalPointDimensions[2] <= 1
      //  ? viskores::worklet::scalar_topology::extract_top_volume_contours::lineTable2d
      //  : (isMarchingCubes
      //       ? viskores::worklet::scalar_topology::extract_top_volume_contours::triTableMC3d
      //       : viskores::worklet::scalar_topology::extract_top_volume_contours::triTableLT3d);
      //IdArrayType labelEdgeTable = isMarchingCubes
      //  ? viskores::worklet::scalar_topology::extract_top_volume_contours::labelEdgeTableMC3d
      //  : viskores::worklet::scalar_topology::extract_top_volume_contours::labelEdgeTableLT3d;

      IdArrayType caseCells;

      caseCells.Allocate(nCells);
      IdArrayType numEdgesInCell;
      numEdgesInCell.Allocate(nCells);
      auto caseCellsWorklet =
        viskores::worklet::scalar_topology::extract_top_volume_contours::GetCellCasesWorklet<
          ValueType>(pointDimensions,
                     currBranchSaddleEpsilon,
                     isoValue,
                     shiftIsovalueByEpsilon,
                     branchSaddleGRId);

      invoke(caseCellsWorklet,
             viskores::cont::ArrayHandleIndex(nCells),
             sortedDataValuesWithinBlock,
             globalIdsWithinBlock,
             vertexOffSetTable,
             caseCells);

      // we compute the number of edges for each cell
      // to initialize the array size of edges
      IdArrayType numBoundariesInCell;
      viskores::worklet::contourtree_augmented::PermuteArrayWithMaskedIndex<viskores::Id,
                                                                            IdArrayType>(
        numBoundTable, caseCells, numBoundariesInCell);

      viskores::cont::Algorithm::Transform(
        numBoundariesInCell,
        globalPointDimensions[2] <= 1
          ? viskores::cont::ArrayHandleConstant<viskores::Id>(1, nCells)
          : viskores::cont::ArrayHandleConstant<viskores::Id>(3, nCells),
        numEdgesInCell,
        viskores::Multiply());

      // use prefix sum to get the offset of the starting edge in each cell/cube
      viskores::Id nEdges = viskores::cont::Algorithm::Reduce<viskores::Id, viskores::Id>(
        numEdgesInCell, viskores::Id(0));
      nContourCandidateMeshes += globalPointDimensions[2] <= 1 ? nEdges : nEdges / 3;
      IdArrayType edgesOffset;
      viskores::cont::Algorithm::ScanExclusive(numEdgesInCell, edgesOffset);

      viskores::cont::ArrayHandle<viskores::Vec3f_64> isosurfaceEdgesFrom;
      viskores::cont::ArrayHandle<viskores::Vec3f_64> isosurfaceEdgesTo;
      IdArrayType isValidEdges;
      isosurfaceEdgesFrom.Allocate(nEdges);
      isosurfaceEdgesTo.Allocate(nEdges);
      isValidEdges.Allocate(nEdges);

      // draw isosurface
      auto getEdgesInCellWorklet =
        viskores::worklet::scalar_topology::extract_top_volume_contours::GetEdgesInCellWorklet<
          ValueType>(pointDimensions,
                     globalPointIndexStart,
                     isoValue,
                     branchSaddleGRId,
                     branchSuperarc,
                     currBranchSaddleEpsilon,
                     totalNumPoints,
                     isMarchingCubes,
                     shiftIsovalueByEpsilon);

      invoke(getEdgesInCellWorklet,
             edgesOffset,
             caseCells,
             sortedLocalNodeInfoIdsWithinBlock,
             sortedDataValuesWithinBlock,
             globalIdsWithinBlock,
             vertexOffSetTable,
             edgeTable,
             numBoundTable,
             boundaryTable,
             labelEdgeTable,
             isosurfaceEdgesFrom,
             isosurfaceEdgesTo,
             isValidEdges,
             findSuperarcForNode);

      // isValidEdges: stencil about whether the edge is on the desired superarc
      viskores::cont::ArrayHandle<viskores::Vec3f_64> validEdgesFrom;
      viskores::cont::ArrayHandle<viskores::Vec3f_64> validEdgesTo;

      // we remove edges that are not on the desired superarc
      viskores::cont::Algorithm::CopyIf(isosurfaceEdgesFrom, isValidEdges, validEdgesFrom);
      viskores::cont::Algorithm::CopyIf(isosurfaceEdgesTo, isValidEdges, validEdgesTo);

      // append edges into the result array
      viskores::Id nValidEdges = validEdgesFrom.GetNumberOfValues();
      viskores::Id nExistEdges = branchIdx == 0 ? 0 : this->IsosurfaceEdgesFrom.GetNumberOfValues();

      this->IsosurfaceEdgesFrom.Allocate(nValidEdges + nExistEdges, viskores::CopyFlag::On);
      this->IsosurfaceEdgesTo.Allocate(nValidEdges + nExistEdges, viskores::CopyFlag::On);

#ifdef DEBUG_PRINT
      std::stringstream edgeInfoStream;
      contourStream << "Debug for Contour Info, Block " << this->LocalBlockNo << std::endl;
      viskores::worklet::contourtree_augmented::PrintHeader(nContours, edgeInfoStream);
      viskores::worklet::contourtree_augmented::PrintValues<viskores::Id>(
        "edgeOffset", this->IsosurfaceEdgesOffset, -1, edgeInfoStream);
      viskores::worklet::contourtree_augmented::PrintValues<viskores::Id>(
        "edgeOrders", this->IsosurfaceEdgesOrders, -1, edgeInfoStream);
      viskores::worklet::contourtree_augmented::PrintValues<viskores::Id>(
        "edgeLabels", this->IsosurfaceEdgesLabels, -1, edgeInfoStream);

      VISKORES_LOG_S(viskores::cont::LogLevel::Info, edgeInfoStream.str());
#endif // DEBUG_PRINT

      viskores::cont::Algorithm::CopySubRange(
        validEdgesFrom, 0, nValidEdges, this->IsosurfaceEdgesFrom, nExistEdges);
      viskores::cont::Algorithm::CopySubRange(
        validEdgesTo, 0, nValidEdges, this->IsosurfaceEdgesTo, nExistEdges);

#ifdef DEBUG_PRINT
      std::stringstream contourStream;
      contourStream << "Debug for Contour, Block " << this->LocalBlockNo << std::endl;
      contourStream << "Branch Superarc = " << branchSuperarc << ", isoValue = " << isoValue
                    << std::endl;

      viskores::worklet::contourtree_augmented::PrintHeader(nCells, contourStream);
      viskores::worklet::contourtree_augmented::PrintIndices(
        "Case of Cells", caseCells, -1, contourStream);
      viskores::worklet::contourtree_augmented::PrintIndices(
        "# of Edges", numEdgesInCell, -1, contourStream);

      viskores::worklet::contourtree_augmented::PrintHeader(nEdges, contourStream);
      viskores::worklet::contourtree_augmented::PrintIndices(
        "Edge Stencil", isValidEdges, -1, contourStream);

      viskores::worklet::contourtree_augmented::PrintHeader(nValidEdges, contourStream);
      viskores::worklet::contourtree_augmented::PrintValues<viskores::Vec3f_64>(
        "EdgesFrom", validEdgesFrom, -1, contourStream);
      viskores::worklet::contourtree_augmented::PrintValues<viskores::Vec3f_64>(
        "EdgesTo", validEdgesTo, -1, contourStream);
      viskores::worklet::contourtree_augmented::PrintIndices(
        "EdgesLabels", validEdgesLabels, -1, contourStream);

      VISKORES_LOG_S(viskores::cont::LogLevel::Info, contourStream.str());
#endif // DEBUG_PRINT
    }
    this->IsosurfaceIsoValue = isosurfaceIsoValue;

    const viskores::Id nMeshesOnBranches = globalPointDimensions[2] <= 1
      ? this->IsosurfaceEdgesFrom.GetNumberOfValues()
      : this->IsosurfaceEdgesFrom.GetNumberOfValues() / 3;
    VISKORES_LOG_S(timingsLogLevel,
                   std::endl
                     << "-----------  Draw Isosurface (block=" << this->LocalBlockNo
                     << ")------------" << std::endl
                     << "    " << std::setw(60) << std::left << "Number of Contours: " << nContours
                     << std::endl
                     << "    " << std::setw(60) << std::left
                     << "Number of Isosurface Meshes: " << nContourCandidateMeshes << std::endl
                     << "    " << std::setw(60) << std::left
                     << "Number of Meshes On Branches: " << nMeshesOnBranches << std::endl);
  };
  this->TopVolumeData.TopVolumeBranchSaddleIsoValue
    .CastAndCallForTypes<viskores::TypeListScalarAll, viskores::cont::StorageListBasic>(
      resolveArray);
}

} // namespace internal
} // namespace scalar_topology
} // namespace filter
} // namespace viskores
