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

#ifndef _viskores_filter_testing_TestingContourTreeUniformDistributedFilter_h_
#define _viskores_filter_testing_TestingContourTreeUniformDistributedFilter_h_

#include <viskores/Types.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/cont/Serialization.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/filter/scalar_topology/ContourTreeUniformDistributed.h>
#include <viskores/filter/scalar_topology/DistributedBranchDecompositionFilter.h>
#include <viskores/filter/scalar_topology/ExtractTopVolumeContoursFilter.h>
#include <viskores/filter/scalar_topology/SelectTopVolumeBranchesFilter.h>
#include <viskores/filter/scalar_topology/testing/SuperArcHelper.h>
#include <viskores/filter/scalar_topology/testing/VolumeHelper.h>
#include <viskores/filter/scalar_topology/worklet/branch_decomposition/HierarchicalVolumetricBranchDecomposer.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/BranchCompiler.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/TreeCompiler.h>
#include <viskores/io/ErrorIO.h>
#include <viskores/io/VTKDataSetReader.h>

namespace viskores
{
namespace filter
{
namespace testing
{
namespace contourtree_uniform_distributed
{
inline viskores::IdComponent FindSplitAxis(viskores::Id3 globalSize)
{
  viskores::IdComponent splitAxis = 0;
  for (viskores::IdComponent d = 1; d < 3; ++d)
  {
    if (globalSize[d] > globalSize[splitAxis])
    {
      splitAxis = d;
    }
  }
  return splitAxis;
}

inline viskores::Id3 ComputeNumberOfBlocksPerAxis(viskores::Id3 globalSize,
                                                  viskores::Id numberOfBlocks)
{
  // Split numberOfBlocks into a power of two and a remainder
  viskores::Id powerOfTwoPortion = 1;
  while (numberOfBlocks % 2 == 0)
  {
    powerOfTwoPortion *= 2;
    numberOfBlocks /= 2;
  }

  viskores::Id3 blocksPerAxis{ 1, 1, 1 };
  if (numberOfBlocks > 1)
  {
    // Split the longest axis according to remainder
    viskores::IdComponent splitAxis = FindSplitAxis(globalSize);
    blocksPerAxis[splitAxis] = numberOfBlocks;
    globalSize[splitAxis] /= numberOfBlocks;
  }

  // Now perform splits for the power of two remainder of numberOfBlocks
  while (powerOfTwoPortion > 1)
  {
    viskores::IdComponent splitAxis = FindSplitAxis(globalSize);
    VISKORES_ASSERT(globalSize[splitAxis] > 1);
    blocksPerAxis[splitAxis] *= 2;
    globalSize[splitAxis] /= 2;
    powerOfTwoPortion /= 2;
  }

  return blocksPerAxis;
}

inline std::tuple<viskores::Id3, viskores::Id3, viskores::Id3>
ComputeBlockExtents(viskores::Id3 globalSize, viskores::Id3 blocksPerAxis, viskores::Id blockNo)
{
  // DEBUG: std::cout << "ComputeBlockExtents("<<globalSize <<", " << blocksPerAxis << ", " << blockNo << ")" << std::endl;
  // DEBUG: std::cout << "Block " << blockNo;

  viskores::Id3 blockIndex, blockOrigin, blockSize;
  for (viskores::IdComponent d = 0; d < 3; ++d)
  {
    blockIndex[d] = blockNo % blocksPerAxis[d];
    blockNo /= blocksPerAxis[d];

    float dx = float(globalSize[d] - 1) / float(blocksPerAxis[d]);
    blockOrigin[d] = viskores::Id(blockIndex[d] * dx);
    viskores::Id maxIdx = blockIndex[d] < blocksPerAxis[d] - 1
      ? viskores::Id((blockIndex[d] + 1) * dx)
      : globalSize[d] - 1;
    blockSize[d] = maxIdx - blockOrigin[d] + 1;
    // DEBUG: std::cout << " " << blockIndex[d] <<  dx << " " << blockOrigin[d] << " " << maxIdx << " " << blockSize[d] << "; ";
  }
  // DEBUG: std::cout << " -> " << blockIndex << " "  << blockOrigin << " " << blockSize << std::endl;
  return std::make_tuple(blockIndex, blockOrigin, blockSize);
}

inline viskores::cont::DataSet CreateSubDataSet(const viskores::cont::DataSet& ds,
                                                viskores::Id3 blockOrigin,
                                                viskores::Id3 blockSize,
                                                const std::string& fieldName)
{
  viskores::Id3 globalSize;
  viskores::cont::CastAndCall(
    ds.GetCellSet(), viskores::worklet::contourtree_augmented::GetPointDimensions(), globalSize);
  const viskores::Id nOutValues = blockSize[0] * blockSize[1] * blockSize[2];

  const auto inDataArrayHandle = ds.GetPointField(fieldName).GetData();

  viskores::cont::ArrayHandle<viskores::Id> copyIdsArray;
  copyIdsArray.Allocate(nOutValues);
  auto copyIdsPortal = copyIdsArray.WritePortal();

  viskores::Id3 outArrIdx;
  for (outArrIdx[2] = 0; outArrIdx[2] < blockSize[2]; ++outArrIdx[2])
    for (outArrIdx[1] = 0; outArrIdx[1] < blockSize[1]; ++outArrIdx[1])
      for (outArrIdx[0] = 0; outArrIdx[0] < blockSize[0]; ++outArrIdx[0])
      {
        viskores::Id3 inArrIdx = outArrIdx + blockOrigin;
        viskores::Id inIdx =
          (inArrIdx[2] * globalSize[1] + inArrIdx[1]) * globalSize[0] + inArrIdx[0];
        viskores::Id outIdx =
          (outArrIdx[2] * blockSize[1] + outArrIdx[1]) * blockSize[0] + outArrIdx[0];
        VISKORES_ASSERT(inIdx >= 0 && inIdx < inDataArrayHandle.GetNumberOfValues());
        VISKORES_ASSERT(outIdx >= 0 && outIdx < nOutValues);
        copyIdsPortal.Set(outIdx, inIdx);
      }
  // DEBUG: std::cout << copyIdsPortal.GetNumberOfValues() << std::endl;

  viskores::cont::Field permutedField;
  bool success =
    viskores::filter::MapFieldPermutation(ds.GetPointField(fieldName), copyIdsArray, permutedField);
  if (!success)
    throw viskores::cont::ErrorBadType("Field copy failed (probably due to invalid type)");

  viskores::cont::DataSetBuilderUniform dsb;
  if (globalSize[2] <= 1) // 2D Data Set
  {
    viskores::Id2 dimensions{ blockSize[0], blockSize[1] };
    viskores::cont::DataSet dataSet = dsb.Create(dimensions);
    viskores::cont::CellSetStructured<2> cellSet;
    cellSet.SetPointDimensions(dimensions);
    cellSet.SetGlobalPointDimensions(viskores::Id2{ globalSize[0], globalSize[1] });
    cellSet.SetGlobalPointIndexStart(viskores::Id2{ blockOrigin[0], blockOrigin[1] });
    dataSet.SetCellSet(cellSet);
    dataSet.AddField(permutedField);
    return dataSet;
  }
  else
  {
    viskores::cont::DataSet dataSet = dsb.Create(blockSize);
    viskores::cont::CellSetStructured<3> cellSet;
    cellSet.SetPointDimensions(blockSize);
    cellSet.SetGlobalPointDimensions(globalSize);
    cellSet.SetGlobalPointIndexStart(blockOrigin);
    dataSet.SetCellSet(cellSet);
    dataSet.AddField(permutedField);
    return dataSet;
  }
}

inline std::vector<viskores::worklet::contourtree_distributed::Edge> ReadGroundTruthContourTree(
  std::string filename)
{
  std::ifstream ct_file(filename);
  if (!ct_file.is_open())
  {
    throw viskores::io::ErrorIO("Unable to open data file: " + filename);
  }
  viskores::Id val1, val2;
  std::vector<viskores::worklet::contourtree_distributed::Edge> result;
  while (ct_file >> val1 >> val2)
  {
    result.push_back(viskores::worklet::contourtree_distributed::Edge(val1, val2));
  }
  std::sort(result.begin(), result.end());
  return result;
}

inline void ReadGroundTruthBranchVolume(std::string filename,
                                        std::vector<viskores::Id>& branchDirections,
                                        std::vector<viskores::Id>& branchInnerEnds,
                                        std::vector<viskores::Id>& branchVolumes)
{
  std::ifstream ct_file(filename);
  if (!ct_file.is_open())
  {
    throw viskores::io::ErrorIO("Unable to open ground truth data file: " + filename);
  }
  // read the branch information line by line
  viskores::Id branchDirection;
  while (ct_file >> branchDirection)
  {
    branchDirections.push_back(branchDirection);
    viskores::Id branchInnerEnd, branchVolume;
    if (branchDirection != 0)
    {
      ct_file >> branchInnerEnd >> branchVolume;
      branchInnerEnds.push_back(branchInnerEnd);
      branchVolumes.push_back(branchVolume);
    }
    else
    {
      viskores::Id branchLowerEnd, branchUpperEnd;
      ct_file >> branchLowerEnd >> branchUpperEnd >> branchVolume;
      // we do not store the main branch in the current check
    }
  }
}

inline viskores::cont::PartitionedDataSet RunContourTreeDUniformDistributed(
  const viskores::cont::DataSet& ds,
  std::string fieldName,
  bool useMarchingCubes,
  int numberOfBlocks,
  int rank,
  int numberOfRanks,
  bool augmentHierarchicalTree,
  bool computeHierarchicalVolumetricBranchDecomposition,
  viskores::Id3& globalSize,
  bool passBlockIndices = true,
  const viskores::Id presimplifyThreshold = 0)
{
  // Get dimensions of data set
  viskores::cont::CastAndCall(
    ds.GetCellSet(), viskores::worklet::contourtree_augmented::GetPointDimensions(), globalSize);

  // Determine split
  viskores::Id3 blocksPerAxis = ComputeNumberOfBlocksPerAxis(globalSize, numberOfBlocks);
  viskores::Id blocksPerRank = numberOfBlocks / numberOfRanks;
  viskores::Id numRanksWithExtraBlock = numberOfBlocks % numberOfRanks;
  viskores::Id blocksOnThisRank, startBlockNo;
  if (rank < numRanksWithExtraBlock)
  {
    blocksOnThisRank = blocksPerRank + 1;
    startBlockNo = (blocksPerRank + 1) * rank;
  }
  else
  {
    blocksOnThisRank = blocksPerRank;
    startBlockNo = numRanksWithExtraBlock * (blocksPerRank + 1) +
      (rank - numRanksWithExtraBlock) * blocksPerRank;
  }

  // Created partitioned (split) data set
  viskores::cont::PartitionedDataSet pds;
  viskores::cont::ArrayHandle<viskores::Id3> localBlockIndices;
  localBlockIndices.Allocate(blocksOnThisRank);

  auto localBlockIndicesPortal = localBlockIndices.WritePortal();

  for (viskores::Id blockNo = 0; blockNo < blocksOnThisRank; ++blockNo)
  {
    viskores::Id3 blockOrigin, blockSize, blockIndex;
    std::tie(blockIndex, blockOrigin, blockSize) =
      ComputeBlockExtents(globalSize, blocksPerAxis, startBlockNo + blockNo);
    pds.AppendPartition(CreateSubDataSet(ds, blockOrigin, blockSize, fieldName));
    localBlockIndicesPortal.Set(blockNo, blockIndex);
  }

  // Run the contour tree analysis
  viskores::filter::scalar_topology::ContourTreeUniformDistributed filter(
    viskores::cont::LogLevel::UserVerboseLast, viskores::cont::LogLevel::UserVerboseLast);

  if (passBlockIndices)
  {
    filter.SetBlockIndices(blocksPerAxis, localBlockIndices);
  }

  filter.SetUseMarchingCubes(useMarchingCubes);
  filter.SetUseBoundaryExtremaOnly(true);
  filter.SetAugmentHierarchicalTree(augmentHierarchicalTree);
  filter.SetActiveField(fieldName);
  if (presimplifyThreshold > 0)
  {
    filter.SetPresimplifyThreshold(presimplifyThreshold);
  }
  auto result = filter.Execute(pds);

  if (computeHierarchicalVolumetricBranchDecomposition)
  {
    using viskores::filter::scalar_topology::DistributedBranchDecompositionFilter;

    DistributedBranchDecompositionFilter bd_filter;
    result = bd_filter.Execute(result);
  }

  if (numberOfRanks == 1)
  {
    // Serial or only one parallel rank -> Result is already
    // everything we need
    return result;
  }
  else
  {
    // Mutiple ranks -> Some assembly required. Collect data
    // on rank 0, all other ranks return empty data sets

    // Communicate results to rank 0
    auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
    viskoresdiy::Master master(comm, 1);
    struct EmptyBlock
    {
    }; // Dummy block structure, since we need block data for DIY
    master.add(comm.rank(), new EmptyBlock, new viskoresdiy::Link);
    // .. Send data to rank 0
    master.foreach (
      [result, filter](void*, const viskoresdiy::Master::ProxyWithLink& p)
      {
        viskoresdiy::BlockID root{ 0, 0 }; // Rank 0
        p.enqueue(root, result.GetNumberOfPartitions());
        for (const viskores::cont::DataSet& curr_ds : result)
        {
          p.enqueue(root, curr_ds);
        }
      });
    // Exchange data, i.e., send to rank 0 (pass "true" to exchange data between
    // *all* blocks, not just neighbors)
    master.exchange(true);

    if (comm.rank() == 0)
    {
      // Receive data on rank zero and return combined results
      viskores::cont::PartitionedDataSet combined_result;
      master.foreach (
        [&combined_result, filter, numberOfRanks](void*,
                                                  const viskoresdiy::Master::ProxyWithLink& p)
        {
          for (int receiveFromRank = 0; receiveFromRank < numberOfRanks; ++receiveFromRank)
          {
            viskores::Id numberOfDataSetsToReceive;
            p.dequeue({ receiveFromRank, receiveFromRank }, numberOfDataSetsToReceive);
            for (viskores::Id currReceiveDataSetNo = 0;
                 currReceiveDataSetNo < numberOfDataSetsToReceive;
                 ++currReceiveDataSetNo)
            {
              viskores::cont::DataSet dsIncoming;
              p.dequeue({ receiveFromRank, receiveFromRank }, dsIncoming);
              combined_result.AppendPartition(dsIncoming);
            }
          }
        });
      return combined_result; // Return combined result on rank 0
    }
    else
    {
      // Return an empty data set on all other ranks
      return viskores::cont::PartitionedDataSet{};
    }
  }
}

inline viskores::cont::PartitionedDataSet RunContourTreeDUniformDistributed(
  const viskores::cont::DataSet& ds,
  std::string fieldName,
  bool useMarchingCubes,
  int numberOfBlocks,
  int rank = 0,
  int numberOfRanks = 1,
  bool augmentHierarchicalTree = false,
  bool computeHierarchicalVolumetricBranchDecomposition = false,
  bool passBlockIndices = true)
{
  viskores::Id3 globalSize;

  return RunContourTreeDUniformDistributed(ds,
                                           fieldName,
                                           useMarchingCubes,
                                           numberOfBlocks,
                                           rank,
                                           numberOfRanks,
                                           augmentHierarchicalTree,
                                           computeHierarchicalVolumetricBranchDecomposition,
                                           globalSize,
                                           passBlockIndices);
}

inline void TestContourTreeUniformDistributed8x9(int nBlocks, int rank = 0, int size = 1)
{
  if (rank == 0)
  {
    std::cout << "Testing ContourTreeUniformDistributed on 2D 8x9 data set divided into " << nBlocks
              << " blocks." << std::endl;
  }
  viskores::cont::DataSet in_ds =
    viskores::cont::testing::MakeTestDataSet().Make2DUniformDataSet3();
  viskores::cont::PartitionedDataSet result =
    RunContourTreeDUniformDistributed(in_ds, "pointvar", false, nBlocks, rank, size);

  if (viskores::cont::EnvironmentTracker::GetCommunicator().rank() == 0)
  {
    viskores::worklet::contourtree_distributed::TreeCompiler treeCompiler;
    for (viskores::Id ds_no = 0; ds_no < result.GetNumberOfPartitions(); ++ds_no)
    {
      treeCompiler.AddHierarchicalTree(result.GetPartition(ds_no));
    }
    treeCompiler.ComputeSuperarcs();

    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    treeCompiler.PrintSuperarcs();

    // Print the expected contour tree
    std::cout << "Expected Contour Tree" << std::endl;
    std::cout << "          10           20" << std::endl;
    std::cout << "          20           34" << std::endl;
    std::cout << "          20           38" << std::endl;
    std::cout << "          20           61" << std::endl;
    std::cout << "          23           34" << std::endl;
    std::cout << "          24           34" << std::endl;
    std::cout << "          50           61" << std::endl;
    std::cout << "          61           71" << std::endl;

    using Edge = viskores::worklet::contourtree_distributed::Edge;
    VISKORES_TEST_ASSERT(test_equal(treeCompiler.superarcs.size(), 8),
                         "Wrong result for ContourTreeUniformDistributed filter");
    VISKORES_TEST_ASSERT(treeCompiler.superarcs[0] == Edge{ 10, 20 },
                         "Wrong result for ContourTreeUniformDistributed filter");
    VISKORES_TEST_ASSERT(treeCompiler.superarcs[1] == Edge{ 20, 34 },
                         "Wrong result for ContourTreeUniformDistributed filter");
    VISKORES_TEST_ASSERT(treeCompiler.superarcs[2] == Edge{ 20, 38 },
                         "Wrong result for ContourTreeUniformDistributed filter");
    VISKORES_TEST_ASSERT(treeCompiler.superarcs[3] == Edge{ 20, 61 },
                         "Wrong result for ContourTreeUniformDistributed filter");
    VISKORES_TEST_ASSERT(treeCompiler.superarcs[4] == Edge{ 23, 34 },
                         "Wrong result for ContourTreeUniformDistributed filter");
    VISKORES_TEST_ASSERT(treeCompiler.superarcs[5] == Edge{ 24, 34 },
                         "Wrong result for ContourTreeUniformDistributed filter");
    VISKORES_TEST_ASSERT(treeCompiler.superarcs[6] == Edge{ 50, 61 },
                         "Wrong result for ContourTreeUniformDistributed filter");
    VISKORES_TEST_ASSERT(treeCompiler.superarcs[7] == Edge{ 61, 71 },
                         "Wrong result for ContourTreeUniformDistributed filter");
  }
}

inline void TestContourTreeUniformDistributedBranchDecomposition8x9(int nBlocks,
                                                                    int rank = 0,
                                                                    int size = 1)
{
  if (rank == 0)
  {
    std::cout << "Testing Distributed Branch Decomposition on 2D 8x9 data set " << nBlocks
              << " blocks." << std::endl;
  }
  viskores::cont::DataSet in_ds =
    viskores::cont::testing::MakeTestDataSet().Make2DUniformDataSet3();
  bool augmentHierarchicalTree = true;
  bool computeHierarchicalVolumetricBranchDecomposition = true;
  viskores::cont::PartitionedDataSet result =
    RunContourTreeDUniformDistributed(in_ds,
                                      "pointvar",
                                      false,
                                      nBlocks,
                                      rank,
                                      size,
                                      augmentHierarchicalTree,
                                      computeHierarchicalVolumetricBranchDecomposition);

  using viskores::filter::scalar_topology::SelectTopVolumeBranchesFilter;

  viskores::Id numBranches = 2;
  SelectTopVolumeBranchesFilter tp_filter;

  tp_filter.SetSavedBranches(numBranches);

  auto tp_result = tp_filter.Execute(result);

  // add filter for contour extraction
  using viskores::filter::scalar_topology::ExtractTopVolumeContoursFilter;
  ExtractTopVolumeContoursFilter iso_filter;

  iso_filter.SetMarchingCubes(false);
  auto iso_result = iso_filter.Execute(tp_result);

  if (viskores::cont::EnvironmentTracker::GetCommunicator().rank() == 0)
  {
    using Edge = viskores::worklet::contourtree_distributed::Edge;
    std::vector<Edge> computed;

    for (viskores::Id ds_no = 0; ds_no < result.GetNumberOfPartitions(); ++ds_no)
    {
      auto ds = result.GetPartition(ds_no);
      auto upperEndGRId = ds.GetField("UpperEndGlobalRegularIds")
                            .GetData()
                            .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()
                            .ReadPortal();
      auto lowerEndGRId = ds.GetField("LowerEndGlobalRegularIds")
                            .GetData()
                            .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()
                            .ReadPortal();
      viskores::Id nBranches = upperEndGRId.GetNumberOfValues();

      for (viskores::Id branch = 0; branch < nBranches; ++branch)
      {
        Edge edge(upperEndGRId.Get(branch), lowerEndGRId.Get(branch));

        if (std::find(computed.begin(), computed.end(), edge) == computed.end())
        {
          computed.push_back(edge);
        }
      }
    }

    std::vector<Edge> expected{
      Edge(10, 20), Edge(23, 71), Edge(34, 24), Edge(38, 20), Edge(61, 50)
    };

    std::sort(computed.begin(), computed.end());
    std::sort(expected.begin(), expected.end());

    if (computed != expected)
    {
      std::cout << "Branch Decomposition Results:" << std::endl;
      std::cout << "Computed Contour Tree" << std::endl;
      for (std::size_t i = 0; i < computed.size(); i++)
      {
        std::cout << std::setw(12) << computed[i].low << std::setw(14) << computed[i].high
                  << std::endl;
      }

      std::cout << "Expected Contour Tree" << std::endl;
      for (std::size_t i = 0; i < expected.size(); i++)
      {
        std::cout << std::setw(12) << expected[i].low << std::setw(14) << expected[i].high
                  << std::endl;
      }
      VISKORES_TEST_FAIL("Branch Decomposition Failed!");
    }

    std::cout << "Branch Decomposition: Results Match!" << std::endl;

    for (viskores::Id ds_no = 0; ds_no < result.GetNumberOfPartitions(); ++ds_no)
    {
      auto ds = tp_result.GetPartition(ds_no);
      auto topVolBranchGRId = ds.GetField("TopVolumeBranchGlobalRegularIds")
                                .GetData()
                                .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()
                                .ReadPortal();
      auto topVolBranchVolume = ds.GetField("TopVolumeBranchVolume")
                                  .GetData()
                                  .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()
                                  .ReadPortal();
      auto topVolBranchSaddleEpsilon = ds.GetField("TopVolumeBranchSaddleEpsilon")
                                         .GetData()
                                         .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()
                                         .ReadPortal();
      auto topVolBranchSaddleIsoValue =
        ds.GetField("TopVolumeBranchSaddleIsoValue")
          .GetData()
          .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>()
          .ReadPortal();

      viskores::Id nSelectedBranches = topVolBranchGRId.GetNumberOfValues();
      Edge expectedGRIdVolumeAtBranch0(38, 6);
      Edge expectedEpsilonIsoAtBranch0(1, 50);
      Edge expectedGRIdVolumeAtBranch1(50, 2);
      Edge expectedEpsilonIsoAtBranch1(-1, 30);

      for (viskores::Id branch = 0; branch < nSelectedBranches; ++branch)
      {
        bool failed = false;
        Edge computedGRIdVolume(topVolBranchGRId.Get(branch), topVolBranchVolume.Get(branch));
        Edge computedEpsilonIso(topVolBranchSaddleEpsilon.Get(branch),
                                (viskores::Id)topVolBranchSaddleIsoValue.Get(branch));

        switch (branch)
        {
          case 0:
            failed = !(computedGRIdVolume == expectedGRIdVolumeAtBranch0);
            failed = (failed || !(computedEpsilonIso == expectedEpsilonIsoAtBranch0));
            break;
          case 1:
            failed = !(computedGRIdVolume == expectedGRIdVolumeAtBranch1);
            failed = (failed || !(computedEpsilonIso == expectedEpsilonIsoAtBranch1));
            break;
          default:
            VISKORES_TEST_ASSERT(false);
        }

        if (failed)
        {
          std::vector<Edge> expectedGRIdVolume{ expectedGRIdVolumeAtBranch0,
                                                expectedGRIdVolumeAtBranch1 };

          std::vector<Edge> expectedEpsilonIso{ expectedEpsilonIsoAtBranch0,
                                                expectedEpsilonIsoAtBranch1 };

          std::cout << "Top Branch Volume Results:" << std::endl;
          std::cout << "Computed Top Branch Volume:branch=" << branch << std::endl;
          std::cout << computedGRIdVolume.low << std::setw(14) << computedGRIdVolume.high
                    << std::setw(5) << computedEpsilonIso.low << std::setw(14)
                    << computedEpsilonIso.high << std::endl;

          std::cout << "Expected Top Branch Volume:branch=" << branch << std::endl;
          std::cout << expectedGRIdVolume[branch].low << std::setw(14)
                    << expectedGRIdVolume[branch].high << std::setw(5)
                    << expectedEpsilonIso[branch].low << std::setw(14)
                    << expectedEpsilonIso[branch].high << std::endl;
          VISKORES_TEST_FAIL("Top Branch Volume Computation Failed!");
        }
      }
    }

    std::cout << "Top Branch Volume: Results Match!" << std::endl;

    if (nBlocks != 2)
      return;

    for (viskores::Id ds_no = 0; ds_no < result.GetNumberOfPartitions(); ++ds_no)
    {
      auto ds = iso_result.GetPartition(ds_no);
      auto isosurfaceEdgesFrom = ds.GetField("IsosurfaceEdgesFrom")
                                   .GetData()
                                   .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec3f_64>>()
                                   .ReadPortal();
      auto isosurfaceEdgesTo = ds.GetField("IsosurfaceEdgesTo")
                                 .GetData()
                                 .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec3f_64>>()
                                 .ReadPortal();
      auto isosurfaceEdgesLabels = ds.GetField("IsosurfaceEdgesLabels")
                                     .GetData()
                                     .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()
                                     .ReadPortal();
      auto isosurfaceEdgesOrders = ds.GetField("IsosurfaceEdgesOrders")
                                     .GetData()
                                     .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()
                                     .ReadPortal();
      auto isosurfaceEdgesOffset = ds.GetField("IsosurfaceEdgesOffset")
                                     .GetData()
                                     .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()
                                     .ReadPortal();
      auto isosurfaceIsoValue = ds.GetField("IsosurfaceIsoValue")
                                  .GetData()
                                  .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>()
                                  .ReadPortal();
      viskores::Id nIsosurfaceEdges = isosurfaceEdgesFrom.GetNumberOfValues();
      viskores::Id isoSurfaceCount = 0;
      std::vector<viskores::Id> computed_iso_surface_info;

      for (viskores::Id edge = 0; edge < nIsosurfaceEdges; ++edge)
      {
        while (isoSurfaceCount < isosurfaceEdgesLabels.GetNumberOfValues() &&
               edge == isosurfaceEdgesOffset.Get(isoSurfaceCount))
        {
          computed_iso_surface_info.push_back(isosurfaceEdgesLabels.Get(isoSurfaceCount));
          computed_iso_surface_info.push_back(isosurfaceEdgesOrders.Get(isoSurfaceCount));
          computed_iso_surface_info.push_back(
            static_cast<viskores::Id>(isosurfaceIsoValue.Get(isoSurfaceCount)));
          isoSurfaceCount++;
        }
      }

      VISKORES_TEST_ASSERT(isoSurfaceCount == 2, "Wrong result for isoSurfaceCount");

      std::vector<viskores::Id> expected_iso_surface_info;
      viskores::Vec3f_64 expected_from_edge0, expected_to_edge0;

      switch (ds_no)
      {
        case 0:
          expected_iso_surface_info = { 5, 1, 50, 4, 0, 50 };
          expected_from_edge0 = viskores::make_Vec(0.519231, 3, 0);
          expected_to_edge0 = viskores::make_Vec(0.5, 2.5, 0);
          break;
        case 1:
          expected_iso_surface_info = { 1, 2, 30, 4, 0, 50 };
          expected_from_edge0 = viskores::make_Vec(4.33333, 5, 0);
          expected_to_edge0 = viskores::make_Vec(4.61538, 4.61538, 0);
          break;
        default:
          VISKORES_TEST_ASSERT(false);
      }

      if (computed_iso_surface_info != expected_iso_surface_info)
      {
        std::cout << "Expected Isosurface Info for block " << ds_no << ":" << std::endl;
        for (std::size_t i = 0; i < expected_iso_surface_info.size(); i += 3)
          std::cout << "Isosurface Info:" << std::setw(5) << expected_iso_surface_info[i]
                    << std::setw(10) << expected_iso_surface_info[i + 1] << std::setw(10)
                    << expected_iso_surface_info[i + 2] << std::endl;
        std::cout << "Computed Isosurface Info for block " << ds_no << ":" << std::endl;
        for (std::size_t i = 0; i < computed_iso_surface_info.size(); i += 3)
          std::cout << "Isosurface Info:" << std::setw(5) << computed_iso_surface_info[i]
                    << std::setw(10) << computed_iso_surface_info[i + 1] << std::setw(10)
                    << computed_iso_surface_info[i + 2] << std::endl;
        VISKORES_TEST_FAIL("Iso Surface Info Don't Match!");
      }

      VISKORES_TEST_ASSERT((ds_no == 0 && nIsosurfaceEdges == 25) ||
                           (ds_no == 1 && nIsosurfaceEdges == 26));
      VISKORES_TEST_ASSERT(test_equal(isosurfaceEdgesFrom.Get(0), expected_from_edge0));
      VISKORES_TEST_ASSERT(test_equal(isosurfaceEdgesTo.Get(0), expected_to_edge0));
    }

    std::cout << "Isosurface: Results Match!" << std::endl;
  }
}

inline void TestContourTreeUniformDistributed5x6x7(int nBlocks,
                                                   bool marchingCubes,
                                                   int rank = 0,
                                                   int size = 1)
{
  if (rank == 0)
  {
    std::cout << "Testing ContourTreeUniformDistributed with "
              << (marchingCubes ? "marching cubes" : "Freudenthal")
              << " mesh connectivity on 3D 5x6x7 data set divided into " << nBlocks << " blocks."
              << std::endl;
  }

  viskores::cont::DataSet in_ds =
    viskores::cont::testing::MakeTestDataSet().Make3DUniformDataSet4();
  viskores::cont::PartitionedDataSet result =
    RunContourTreeDUniformDistributed(in_ds, "pointvar", marchingCubes, nBlocks, rank, size);

  if (rank == 0)
  {
    viskores::worklet::contourtree_distributed::TreeCompiler treeCompiler;
    for (viskores::Id ds_no = 0; ds_no < result.GetNumberOfPartitions(); ++ds_no)
    {
      treeCompiler.AddHierarchicalTree(result.GetPartition(ds_no));
    }
    treeCompiler.ComputeSuperarcs();

    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    treeCompiler.PrintSuperarcs();

    // Print the expected contour tree
    using Edge = viskores::worklet::contourtree_distributed::Edge;
    std::cout << "Expected Contour Tree" << std::endl;
    if (!marchingCubes)
    {
      std::cout << "           0          112" << std::endl;
      std::cout << "          71           72" << std::endl;
      std::cout << "          72           78" << std::endl;
      std::cout << "          72          101" << std::endl;
      std::cout << "         101          112" << std::endl;
      std::cout << "         101          132" << std::endl;
      std::cout << "         107          112" << std::endl;
      std::cout << "         131          132" << std::endl;
      std::cout << "         132          138" << std::endl;

      VISKORES_TEST_ASSERT(test_equal(treeCompiler.superarcs.size(), 9),
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[0] == Edge{ 0, 112 },
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[1] == Edge{ 71, 72 },
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[2] == Edge{ 72, 78 },
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[3] == Edge{ 72, 101 },
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[4] == Edge{ 101, 112 },
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[5] == Edge{ 101, 132 },
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[6] == Edge{ 107, 112 },
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[7] == Edge{ 131, 132 },
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[8] == Edge{ 132, 138 },
                           "Wrong result for ContourTreeUniformDistributed filter");
    }
    else
    {
      std::cout << "           0          203" << std::endl;
      std::cout << "          71           72" << std::endl;
      std::cout << "          72           78" << std::endl;
      std::cout << "          72          101" << std::endl;
      std::cout << "         101          112" << std::endl;
      std::cout << "         101          132" << std::endl;
      std::cout << "         107          112" << std::endl;
      std::cout << "         112          203" << std::endl;
      std::cout << "         131          132" << std::endl;
      std::cout << "         132          138" << std::endl;
      std::cout << "         203          209" << std::endl;

      VISKORES_TEST_ASSERT(test_equal(treeCompiler.superarcs.size(), 11),
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[0] == Edge{ 0, 203 },
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[1] == Edge{ 71, 72 },
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[2] == Edge{ 72, 78 },
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[3] == Edge{ 72, 101 },
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[4] == Edge{ 101, 112 },
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[5] == Edge{ 101, 132 },
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[6] == Edge{ 107, 112 },
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[7] == Edge{ 112, 203 },
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[8] == Edge{ 131, 132 },
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[9] == Edge{ 132, 138 },
                           "Wrong result for ContourTreeUniformDistributed filter");
      VISKORES_TEST_ASSERT(treeCompiler.superarcs[10] == Edge{ 203, 209 },
                           "Wrong result for ContourTreeUniformDistributed filter");
    }
  }
}

inline void TestContourTreeFile(std::string ds_filename,
                                std::string fieldName,
                                std::string gtct_filename,
                                int nBlocks,
                                bool marchingCubes = false,
                                int rank = 0,
                                int size = 1,
                                bool augmentHierarchicalTree = false,
                                bool computeHierarchicalVolumetricBranchDecomposition = false,
                                bool passBlockIndices = true)
{
  if (rank == 0)
  {
    std::cout << "Testing ContourTreeUniformDistributed with "
              << (marchingCubes ? "marching cubes" : "Freudenthal") << " mesh connectivity on \""
              << ds_filename << "\" divided into " << nBlocks << " blocks." << std::endl;
  }

  viskores::io::VTKDataSetReader reader(ds_filename);
  viskores::cont::DataSet ds;
  try
  {
    ds = reader.ReadDataSet();
  }
  catch (viskores::io::ErrorIO& e)
  {
    std::string message("Error reading: ");
    message += ds_filename;
    message += ", ";
    message += e.GetMessage();

    VISKORES_TEST_FAIL(message.c_str());
  }

  viskores::Id3 globalSize;

  viskores::cont::PartitionedDataSet result =
    RunContourTreeDUniformDistributed(ds,
                                      fieldName,
                                      marchingCubes,
                                      nBlocks,
                                      rank,
                                      size,
                                      augmentHierarchicalTree,
                                      computeHierarchicalVolumetricBranchDecomposition,
                                      globalSize,
                                      passBlockIndices);

  if (rank == 0)
  {
    if (!augmentHierarchicalTree && computeHierarchicalVolumetricBranchDecomposition)
      augmentHierarchicalTree = true;

    if (augmentHierarchicalTree)
    {
      if (computeHierarchicalVolumetricBranchDecomposition)
      {
        SuperArcHelper helper;

        for (viskores::Id ds_no = 0; ds_no < result.GetNumberOfPartitions(); ++ds_no)
        {
          auto lds = result.GetPartition(ds_no);

          using viskores::filter::scalar_topology::HierarchicalVolumetricBranchDecomposer;
          helper.Parse(HierarchicalVolumetricBranchDecomposer::PrintBranches(lds));
        }

        std::stringstream out;

        helper.Print(out);

        std::stringstream in(out.str());
        viskores::worklet::contourtree_distributed::BranchCompiler compiler1, compiler2;

        compiler1.Parse(in);
        compiler2.Load(gtct_filename);

        if (compiler1.branches != compiler2.branches)
        {
          std::cout << "Computed Branch Decomposition/BranchCompiler" << std::endl;
          compiler1.Print(std::cout);
          std::cout << "Expected Branch Decomposition/BranchCompiler" << std::endl;
          compiler2.Print(std::cout);
          VISKORES_TEST_FAIL("Branch Decomposition/BranchCompiler FAILED");
        }
      }
      else
      {
        VolumeHelper volumeHelper1, volumeHelper2;

        for (viskores::Id ds_no = 0; ds_no < result.GetNumberOfPartitions(); ++ds_no)
        {
          auto lds = result.GetPartition(ds_no);
          viskores::worklet::contourtree_augmented::IdArrayType supernodes;
          lds.GetField("Supernodes").GetData().AsArrayHandle(supernodes);
          viskores::worklet::contourtree_augmented::IdArrayType superarcs;
          lds.GetField("Superarcs").GetData().AsArrayHandle(superarcs);
          viskores::worklet::contourtree_augmented::IdArrayType regularNodeGlobalIds;
          lds.GetField("RegularNodeGlobalIds").GetData().AsArrayHandle(regularNodeGlobalIds);
          viskores::Id totalVolume = globalSize[0] * globalSize[1] * globalSize[2];
          viskores::worklet::contourtree_augmented::IdArrayType intrinsicVolume;
          lds.GetField("IntrinsicVolume").GetData().AsArrayHandle(intrinsicVolume);
          viskores::worklet::contourtree_augmented::IdArrayType dependentVolume;
          lds.GetField("DependentVolume").GetData().AsArrayHandle(dependentVolume);

          std::string dumpVolumesString =
            viskores::worklet::contourtree_distributed::HierarchicalContourTree<
              viskores::Float32>::DumpVolumes(supernodes,
                                              superarcs,
                                              regularNodeGlobalIds,
                                              totalVolume,
                                              intrinsicVolume,
                                              dependentVolume);

          volumeHelper1.Parse(dumpVolumesString);
        }

        volumeHelper2.Load(gtct_filename);

        if (volumeHelper1.volumes != volumeHelper2.volumes)
        {
          std::cout << "Computed AugmentHierarchicalTree:" << std::endl;
          volumeHelper1.Print(std::cout);
          std::cout << "Expected AugmentHierarchicalTree:" << std::endl;
          volumeHelper2.Print(std::cout);

          VISKORES_TEST_FAIL("AugmentHierarchicalTree FAILED");
        }
      }
    }
    else
    {
      viskores::worklet::contourtree_distributed::TreeCompiler treeCompiler;
      for (viskores::Id ds_no = 0; ds_no < result.GetNumberOfPartitions(); ++ds_no)
      {
        treeCompiler.AddHierarchicalTree(result.GetPartition(ds_no));
      }
      treeCompiler.ComputeSuperarcs();

      std::vector<viskores::worklet::contourtree_distributed::Edge> groundTruthSuperarcs =
        ReadGroundTruthContourTree(gtct_filename);
      if (groundTruthSuperarcs.size() < 50)
      {
        std::cout << "Computed Contour Tree" << std::endl;
        treeCompiler.PrintSuperarcs();

        // Print the expected contour tree
        std::cout << "Expected Contour Tree" << std::endl;
        viskores::worklet::contourtree_distributed::TreeCompiler::PrintSuperarcArray(
          groundTruthSuperarcs);
      }
      else
      {
        std::cout << "Not printing computed and expected contour tree due to size." << std::endl;
      }

      VISKORES_TEST_ASSERT(treeCompiler.superarcs == groundTruthSuperarcs,
                           "Test failed for data set " + ds_filename);
    }
  }
}

// Routine to verify the branch decomposition results from presimplification.
inline void VerifyContourTreePresimplificationOutput(std::string datasetName,
                                                     viskores::cont::PartitionedDataSet& tp_result,
                                                     std::vector<viskores::Id>& gtBranchDirections,
                                                     std::vector<viskores::Id>& gtBranchInnerEnds,
                                                     std::vector<viskores::Id>& gtBranchVolumes,
                                                     int rank = 0,
                                                     const viskores::Id presimplifyThreshold = 1)
{
  if (rank == 0)
  {
    // the top branches by volume are consistent across all blocks
    auto tp_ds = tp_result.GetPartition(0);
    auto topVolBranchUpperEndGRIds = tp_ds.GetField("TopVolumeBranchUpperEnd")
                                       .GetData()
                                       .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()
                                       .ReadPortal();
    auto topVolBranchLowerEndGRIds = tp_ds.GetField("TopVolumeBranchLowerEnd")
                                       .GetData()
                                       .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()
                                       .ReadPortal();
    auto topVolBranchVolume = tp_ds.GetField("TopVolumeBranchVolume")
                                .GetData()
                                .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()
                                .ReadPortal();
    auto topVolBranchSaddleEpsilon = tp_ds.GetField("TopVolumeBranchSaddleEpsilon")
                                       .GetData()
                                       .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()
                                       .ReadPortal();
    viskores::Id nSelectedBranches = topVolBranchUpperEndGRIds.GetNumberOfValues();
    std::vector<viskores::Id> gtSortedOrder, sortedOrder;
    std::vector<viskores::Id> topVolBranchInnerEnds;
    for (viskores::Id branch = 0; branch < nSelectedBranches; branch++)
    {
      if (topVolBranchVolume.Get(branch) > presimplifyThreshold)
      {
        sortedOrder.push_back(branch);
      }
      if (topVolBranchSaddleEpsilon.Get(branch) < 0)
        topVolBranchInnerEnds.push_back(topVolBranchUpperEndGRIds.Get(branch));
      else
        topVolBranchInnerEnds.push_back(topVolBranchLowerEndGRIds.Get(branch));
    }
    for (size_t branch = 0; branch < gtBranchVolumes.size(); branch++)
    {
      if (gtBranchVolumes.at(branch) > presimplifyThreshold)
        gtSortedOrder.push_back(branch);
    }
    VISKORES_TEST_ASSERT(sortedOrder.size() == gtSortedOrder.size(),
                         "Test failed: number of branches does not match for data set " +
                           datasetName);
    std::sort(sortedOrder.begin(),
              sortedOrder.end(),
              [topVolBranchInnerEnds, topVolBranchVolume, topVolBranchSaddleEpsilon](
                const viskores::Id& lhs, const viskores::Id& rhs)
              {
                if (topVolBranchInnerEnds.at(lhs) < topVolBranchInnerEnds.at(rhs))
                  return true;
                if (topVolBranchInnerEnds.at(lhs) > topVolBranchInnerEnds.at(rhs))
                  return false;
                if (topVolBranchVolume.Get(lhs) < topVolBranchVolume.Get(rhs))
                  return true;
                if (topVolBranchVolume.Get(lhs) > topVolBranchVolume.Get(rhs))
                  return false;
                return (topVolBranchSaddleEpsilon.Get(lhs) < topVolBranchSaddleEpsilon.Get(rhs));
              });
    std::sort(gtSortedOrder.begin(),
              gtSortedOrder.end(),
              [gtBranchInnerEnds, gtBranchVolumes, gtBranchDirections](const viskores::Id& lhs,
                                                                       const viskores::Id& rhs)
              {
                if (gtBranchInnerEnds.at(lhs) < gtBranchInnerEnds.at(rhs))
                  return true;
                if (gtBranchInnerEnds.at(lhs) > gtBranchInnerEnds.at(rhs))
                  return false;
                if (gtBranchVolumes.at(lhs) < gtBranchVolumes.at(rhs))
                  return true;
                if (gtBranchVolumes.at(lhs) > gtBranchVolumes.at(rhs))
                  return false;
                return (gtBranchDirections.at(lhs) < gtBranchDirections.at(rhs));
              });

    for (size_t branch = 0; branch < sortedOrder.size(); branch++)
    {
      VISKORES_TEST_ASSERT(topVolBranchInnerEnds.at(sortedOrder.at(branch)) ==
                             gtBranchInnerEnds.at(gtSortedOrder.at(branch)),
                           "Test failed: branch inner end does not match for data set " +
                             datasetName);

      VISKORES_TEST_ASSERT(topVolBranchVolume.Get(sortedOrder.at(branch)) ==
                             gtBranchVolumes.at(gtSortedOrder.at(branch)),
                           "Test failed: branch volume does not match for data set " + datasetName);

      VISKORES_TEST_ASSERT(topVolBranchSaddleEpsilon.Get(sortedOrder.at(branch)) ==
                             gtBranchDirections.at(gtSortedOrder.at(branch)),
                           "Test failed: branch direction does not match for data set " +
                             datasetName);
    }
  }
}

// routine to run distributed contour tree and presimplification
inline void RunContourTreePresimplification(std::string fieldName,
                                            viskores::cont::DataSet& ds,
                                            viskores::cont::PartitionedDataSet& tp_result,
                                            int nBlocks,
                                            bool marchingCubes = false,
                                            int rank = 0,
                                            int size = 1,
                                            bool passBlockIndices = true,
                                            const viskores::Id presimplifyThreshold = 1)
{
  viskores::Id3 globalSize;

  viskores::cont::PartitionedDataSet result =
    RunContourTreeDUniformDistributed(ds,
                                      fieldName,
                                      marchingCubes,
                                      nBlocks,
                                      rank,
                                      size,
                                      true,
                                      true,
                                      globalSize,
                                      passBlockIndices,
                                      presimplifyThreshold);

  // Compute branch decomposition
  viskores::cont::PartitionedDataSet bd_result;

  viskores::filter::scalar_topology::DistributedBranchDecompositionFilter bd_filter;
  bd_result = bd_filter.Execute(result);

  // Compute SelectTopVolumeBranches
  viskores::filter::scalar_topology::SelectTopVolumeBranchesFilter tp_filter;

  // numBranches needs to be large enough to include all branches
  // numBranches < numSuperarcs < globalSize
  tp_filter.SetSavedBranches(globalSize[0] * globalSize[1] *
                             (globalSize[2] > 1 ? globalSize[2] : 1));
  tp_filter.SetPresimplifyThreshold(presimplifyThreshold);
  tp_result = tp_filter.Execute(bd_result);
}

// routine to test contour tree presimplification when data set is
// already in memory
inline void TestContourTreePresimplification(std::string datasetName,
                                             std::string fieldName,
                                             std::string gtbr_filename,
                                             int nBlocks,
                                             viskores::cont::DataSet input_ds,
                                             const viskores::Id presimplifyThreshold = 1,
                                             bool marchingCubes = false,
                                             int rank = 0,
                                             int size = 1,
                                             bool passBlockIndices = true)
{
  if (rank == 0)
  {
    std::cout << "Testing ContourTreeUniformDistributed with "
              << (marchingCubes ? "marching cubes" : "Freudenthal") << " mesh connectivity on \""
              << datasetName << "\" divided into " << nBlocks
              << " blocks. Using presimplification threshold = " << presimplifyThreshold
              << std::endl;
  }

  // get the output of contour tree + presimplification
  viskores::cont::PartitionedDataSet tp_result;
  RunContourTreePresimplification(fieldName,
                                  input_ds,
                                  tp_result,
                                  nBlocks,
                                  marchingCubes,
                                  rank,
                                  size,
                                  passBlockIndices,
                                  presimplifyThreshold);

  // get the ground truth from file
  if (rank == 0)
  {
    std::vector<viskores::Id> gtBranchDirections;
    std::vector<viskores::Id> gtBranchInnerEnds;
    std::vector<viskores::Id> gtBranchVolumes;

    // load the ground truth branch decomposition by volume from file
    ReadGroundTruthBranchVolume(
      gtbr_filename, gtBranchDirections, gtBranchInnerEnds, gtBranchVolumes);

    // verify the contour tree presimplification output with ground truth
    VerifyContourTreePresimplificationOutput(datasetName,
                                             tp_result,
                                             gtBranchDirections,
                                             gtBranchInnerEnds,
                                             gtBranchVolumes,
                                             rank,
                                             presimplifyThreshold);
  }
}

inline void TestContourTreePresimplification(std::string datasetName,
                                             std::string fieldName,
                                             std::string gtbr_filename,
                                             int nBlocks,
                                             std::string ds_filename, // dataset file name
                                             const viskores::Id presimplifyThreshold = 1,
                                             bool marchingCubes = false,
                                             int rank = 0,
                                             int size = 1,
                                             bool passBlockIndices = true)
{
  viskores::cont::DataSet ds;
  if (rank == 0)
    std::cout << "Loading data from " << ds_filename << std::endl;
  viskores::io::VTKDataSetReader reader(ds_filename);
  try
  {
    ds = reader.ReadDataSet();
  }
  catch (viskores::io::ErrorIO& e)
  {
    std::string message("Error reading: ");
    message += ds_filename;
    message += ", ";
    message += e.GetMessage();

    VISKORES_TEST_FAIL(message.c_str());
  }

  TestContourTreePresimplification(datasetName,
                                   fieldName,
                                   gtbr_filename,
                                   nBlocks,
                                   ds,
                                   presimplifyThreshold,
                                   marchingCubes,
                                   rank,
                                   size,
                                   passBlockIndices);
}

}
}
}
} // viskores::filter::testing::contourtree_uniform_distributed

#endif
