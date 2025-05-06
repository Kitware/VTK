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

#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/scalar_topology/ExtractTopVolumeContoursFilter.h>
#include <viskores/filter/scalar_topology/internal/ExtractTopVolumeContoursBlock.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ArrayTransforms.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/DataSetMesh.h>

// viskores includes
#include <viskores/cont/Timer.h>

// DIY includes
// clang-format off
VISKORES_THIRDPARTY_PRE_INCLUDE
#include <viskores/thirdparty/diy/Configure.h>
#include <viskores/thirdparty/diy/diy.h>
VISKORES_THIRDPARTY_POST_INCLUDE
// clang-format on

namespace viskores
{
namespace filter
{
namespace scalar_topology
{

VISKORES_CONT viskores::cont::DataSet ExtractTopVolumeContoursFilter::DoExecute(
  const viskores::cont::DataSet&)
{
  throw viskores::cont::ErrorFilterExecution(
    "ExtractTopVolumeContoursFilter expects PartitionedDataSet as input.");
}

VISKORES_CONT viskores::cont::PartitionedDataSet
ExtractTopVolumeContoursFilter::DoExecutePartitions(const viskores::cont::PartitionedDataSet& input)
{
  viskores::cont::Timer timer;
  timer.Start();
  std::stringstream timingsStream;

  auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  int rank = comm.rank();
  int size = comm.size();


  using ExtractTopVolumeContoursBlock =
    viskores::filter::scalar_topology::internal::ExtractTopVolumeContoursBlock;
  viskoresdiy::Master branch_top_volume_master(comm,
                                               1,  // Use 1 thread, Viskores will do the treading
                                               -1, // All blocks in memory
                                               0,  // No create function
                                               ExtractTopVolumeContoursBlock::Destroy);

  timingsStream << "    " << std::setw(60) << std::left
                << "Create DIY Master and Assigner (Contour Extraction)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  auto firstDS = input.GetPartition(0);
  viskores::Id3 firstPointDimensions, firstGlobalPointDimensions, firstGlobalPointIndexStart;
  firstDS.GetCellSet().CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>(
    viskores::worklet::contourtree_augmented::GetLocalAndGlobalPointDimensions(),
    firstPointDimensions,
    firstGlobalPointDimensions,
    firstGlobalPointIndexStart);
  int numDims = firstGlobalPointDimensions[2] > 1 ? 3 : 2;
  auto viskoresBlocksPerDimensionRP = input.GetPartition(0)
                                        .GetField("viskoresBlocksPerDimension")
                                        .GetData()
                                        .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()
                                        .ReadPortal();

  // ... compute division vector for global domain
  int globalNumberOfBlocks = 1;

  for (viskores::IdComponent d = 0; d < static_cast<viskores::IdComponent>(numDims); ++d)
  {
    globalNumberOfBlocks *= static_cast<int>(viskoresBlocksPerDimensionRP.Get(d));
  }

  // Record time to compute the local block ids
  timingsStream << "    " << std::setw(60) << std::left
                << "Get DIY Information (Contour Extraction)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  viskoresdiy::DynamicAssigner assigner(comm, size, globalNumberOfBlocks);
  for (viskores::Id localBlockIndex = 0; localBlockIndex < input.GetNumberOfPartitions();
       ++localBlockIndex)
  {
    const viskores::cont::DataSet& ds = input.GetPartition(localBlockIndex);
    int globalBlockId = static_cast<int>(
      viskores::cont::ArrayGetValue(0,
                                    ds.GetField("viskoresGlobalBlockId")
                                      .GetData()
                                      .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()));

    ExtractTopVolumeContoursBlock* b =
      new ExtractTopVolumeContoursBlock(localBlockIndex, globalBlockId);

    branch_top_volume_master.add(globalBlockId, b, new viskoresdiy::Link());
    assigner.set_rank(rank, globalBlockId);
  }

  // Log time to copy the data to the block data objects
  timingsStream << "    " << std::setw(60) << std::left << "Initialize Contour Extraction Data"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  timingsStream << "    " << std::setw(60) << std::left
                << "Create DIY Assigner (Contour Extraction)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // Fix the viskoresdiy links.
  viskoresdiy::fix_links(branch_top_volume_master, assigner);

  timingsStream << "    " << std::setw(60) << std::left << "Fix DIY Links (Contour Extraction)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // We compute everything we need for contour extraction and put them in the output dataset.
  branch_top_volume_master.foreach (
    [&](ExtractTopVolumeContoursBlock* b, const viskoresdiy::Master::ProxyWithLink&)
    {
      const viskores::cont::DataSet& ds = input.GetPartition(b->LocalBlockNo);
      b->ExtractIsosurfaceOnSelectedBranch(ds,
                                           this->GetMarchingCubes(),
                                           this->GetShiftIsovalueByEpsilon(),
                                           this->GetTimingsLogLevel());
    });

  timingsStream << "    " << std::setw(60) << std::left << "Draw Contours By Branches"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  std::vector<viskores::cont::DataSet> outputDataSets(input.GetNumberOfPartitions());
  // we need to send everything that contour extraction needs to the output dataset
  branch_top_volume_master.foreach (
    [&](ExtractTopVolumeContoursBlock* b, const viskoresdiy::Master::ProxyWithLink&)
    {
      viskores::cont::Field IsosurfaceEdgeFromField(
        "IsosurfaceEdgesFrom",
        viskores::cont::Field::Association::WholeDataSet,
        b->IsosurfaceEdgesFrom);
      outputDataSets[b->LocalBlockNo].AddField(IsosurfaceEdgeFromField);
      viskores::cont::Field IsosurfaceEdgeToField("IsosurfaceEdgesTo",
                                                  viskores::cont::Field::Association::WholeDataSet,
                                                  b->IsosurfaceEdgesTo);
      outputDataSets[b->LocalBlockNo].AddField(IsosurfaceEdgeToField);
      viskores::cont::Field IsosurfaceEdgeLabelField(
        "IsosurfaceEdgesLabels",
        viskores::cont::Field::Association::WholeDataSet,
        b->IsosurfaceEdgesLabels);
      outputDataSets[b->LocalBlockNo].AddField(IsosurfaceEdgeLabelField);

      viskores::cont::Field IsosurfaceEdgeOffsetField(
        "IsosurfaceEdgesOffset",
        viskores::cont::Field::Association::WholeDataSet,
        b->IsosurfaceEdgesOffset);
      outputDataSets[b->LocalBlockNo].AddField(IsosurfaceEdgeOffsetField);

      viskores::cont::Field IsosurfaceEdgeOrderField(
        "IsosurfaceEdgesOrders",
        viskores::cont::Field::Association::WholeDataSet,
        b->IsosurfaceEdgesOrders);
      outputDataSets[b->LocalBlockNo].AddField(IsosurfaceEdgeOrderField);
      viskores::cont::Field IsosurfaceIsoValueField(
        "IsosurfaceIsoValue",
        viskores::cont::Field::Association::WholeDataSet,
        b->IsosurfaceIsoValue);
      outputDataSets[b->LocalBlockNo].AddField(IsosurfaceIsoValueField);
    });

  timingsStream << "    " << std::setw(38) << std::left << "Creating Contour Extraction Output Data"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;

  VISKORES_LOG_S(this->TimingsLogLevel,
                 std::endl
                   << "-----------  DoExecutePartitions Timings ------------" << std::endl
                   << timingsStream.str());

  return viskores::cont::PartitionedDataSet{ outputDataSets };
}

} // namespace scalar_topology
} // namespace filter
} // namespace viskores
