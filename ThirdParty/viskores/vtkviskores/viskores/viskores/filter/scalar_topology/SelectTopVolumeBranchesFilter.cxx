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
#include <viskores/filter/scalar_topology/SelectTopVolumeBranchesFilter.h>
#include <viskores/filter/scalar_topology/internal/SelectTopVolumeBranchesBlock.h>
#include <viskores/filter/scalar_topology/internal/SelectTopVolumeBranchesFunctor.h>
#include <viskores/filter/scalar_topology/internal/UpdateParentBranchFunctor.h>
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

VISKORES_CONT viskores::cont::DataSet SelectTopVolumeBranchesFilter::DoExecute(
  const viskores::cont::DataSet&)
{
  throw viskores::cont::ErrorFilterExecution(
    "SelectTopVolumeBranchesFilter expects PartitionedDataSet as input.");
}

VISKORES_CONT viskores::cont::PartitionedDataSet SelectTopVolumeBranchesFilter::DoExecutePartitions(
  const viskores::cont::PartitionedDataSet& input)
{
  viskores::cont::Timer timer;
  timer.Start();
  std::stringstream timingsStream;

  auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  int rank = comm.rank();
  int size = comm.size();


  using SelectTopVolumeBranchesBlock =
    viskores::filter::scalar_topology::internal::SelectTopVolumeBranchesBlock;
  viskoresdiy::Master branch_top_volume_master(comm,
                                               1,  // Use 1 thread, Viskores will do the treading
                                               -1, // All blocks in memory
                                               0,  // No create function
                                               SelectTopVolumeBranchesBlock::Destroy);

  timingsStream << "    " << std::setw(60) << std::left
                << "Create DIY Master and Assigner (Branch Selection)"
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
  using RegularDecomposer = viskoresdiy::RegularDecomposer<viskoresdiy::DiscreteBounds>;
  RegularDecomposer::DivisionsVector diyDivisions(numDims);
  viskoresdiy::DiscreteBounds diyBounds(numDims);
  int globalNumberOfBlocks = 1;

  for (viskores::IdComponent d = 0; d < static_cast<viskores::IdComponent>(numDims); ++d)
  {
    diyDivisions[d] = static_cast<int>(viskoresBlocksPerDimensionRP.Get(d));
    globalNumberOfBlocks *= static_cast<int>(viskoresBlocksPerDimensionRP.Get(d));
    diyBounds.min[d] = 0;
    diyBounds.max[d] = static_cast<int>(firstGlobalPointDimensions[d]);
  }

  // Record time to compute the local block ids

  timingsStream << "    " << std::setw(60) << std::left << "Get DIY Information (Branch Selection)"
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

    SelectTopVolumeBranchesBlock* b =
      new SelectTopVolumeBranchesBlock(localBlockIndex, globalBlockId);

    branch_top_volume_master.add(globalBlockId, b, new viskoresdiy::Link());
    assigner.set_rank(rank, globalBlockId);
  }

  // Log time to copy the data to the block data objects
  timingsStream << "    " << std::setw(60) << std::left << "Initialize Branch Selection Data"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // Set up DIY for binary reduction
  RegularDecomposer::BoolVector shareFace(3, true);
  RegularDecomposer::BoolVector wrap(3, false);
  RegularDecomposer::CoordinateVector ghosts(3, 1);
  RegularDecomposer decomposer(numDims,
                               diyBounds,
                               static_cast<int>(globalNumberOfBlocks),
                               shareFace,
                               wrap,
                               ghosts,
                               diyDivisions);

  timingsStream << "    " << std::setw(60) << std::left
                << "Create DIY Decomposer and Assigner (Branch Decomposition)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // Fix the viskoresdiy links.
  viskoresdiy::fix_links(branch_top_volume_master, assigner);

  timingsStream << "    " << std::setw(60) << std::left << "Fix DIY Links (Branch Selection)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // partners for merge over regular block grid
  viskoresdiy::RegularSwapPartners partners(
    decomposer, // domain decomposition
    2,          // radix of k-ary reduction.
    true        // contiguous: true=distance doubling, false=distance halving
  );

  timingsStream << "    " << std::setw(60) << std::left
                << "Create DIY Swap Partners (Branch Selection)"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // compute the branch volume, and select the top branch by volume locally
  branch_top_volume_master.foreach (
    [&](SelectTopVolumeBranchesBlock* b, const viskoresdiy::Master::ProxyWithLink&)
    {
      using viskores::worklet::contourtree_augmented::IdArrayType;
      const auto& globalSize = firstGlobalPointDimensions;
      viskores::Id totalVolume = globalSize[0] * globalSize[1] * globalSize[2];
      const viskores::cont::DataSet& ds = input.GetPartition(b->LocalBlockNo);

      // compute the volume of branches
      b->SortBranchByVolume(ds, totalVolume);
      // select the top branch by volume
      b->SelectLocalTopVolumeBranches(ds, this->GetSavedBranches());
    });

  timingsStream << "    " << std::setw(60) << std::left << "SelectBranchByVolume"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // We apply block reduction to collect the top NumSavedBranches branches by volume
  viskoresdiy::reduce(branch_top_volume_master,
                      assigner,
                      partners,
                      viskores::filter::scalar_topology::internal::SelectTopVolumeBranchesFunctor(
                        this->NumSavedBranches, this->TimingsLogLevel));

  timingsStream << "    " << std::setw(60) << std::left << "SelectGlobalTopVolumeBranches"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // before computing the hierarchy of selected branches, we exclude selected branches
  // with volume <= presimplifyThreshold
  branch_top_volume_master.foreach (
    [&](SelectTopVolumeBranchesBlock* b, const viskoresdiy::Master::ProxyWithLink&) {
      this->SetSavedBranches(b->ExcludeTopVolumeBranchByThreshold(this->GetPresimplifyThreshold()));
    });

  // if we do not have any saved branches,
  //   case 1. didn't specify nBranches correctly, and/or
  //   case 2. over pre-simplified,
  // then we terminate the function prematurely.
  if (this->NumSavedBranches <= 0)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                   "No branch is remaining!\n"
                   "Check the presimplification level or the number of branches to save.");
    std::vector<viskores::cont::DataSet> emptyDataSets(input.GetNumberOfPartitions());
    return viskores::cont::PartitionedDataSet{ emptyDataSets };
  }

  // we compute the hierarchy of selected branches adding the root branch for each block
  branch_top_volume_master.foreach (
    [&](SelectTopVolumeBranchesBlock* b, const viskoresdiy::Master::ProxyWithLink&)
    {
      const viskores::cont::DataSet& ds = input.GetPartition(b->LocalBlockNo);
      b->ComputeTopVolumeBranchHierarchy(ds);
    });

  timingsStream << "    " << std::setw(60) << std::left << "ComputeTopVolumeBranchHierarchy"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // We apply block reduction to update
  //   1. the global branch hierarchy
  //   2. the outer-most saddle isovalue on all parent branches
  viskoresdiy::reduce(
    branch_top_volume_master,
    assigner,
    partners,
    viskores::filter::scalar_topology::internal::UpdateParentBranchFunctor(this->TimingsLogLevel));

  timingsStream << "    " << std::setw(60) << std::left << "Update Parent Branch Information"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // The next step is to extract contours.
  // However, we use a separate filter to do it.
  // This is because we want to utilize the existing Contour filter in Viskores,
  // but the work is not trivial and need more discussion (e.g., implicit mesh triangulation)

  // Create output dataset
  std::vector<viskores::cont::DataSet> outputDataSets(input.GetNumberOfPartitions());
  // Copy input data set to output
  // This will make the output dataset pretty large.
  // Unfortunately, this step seems to be inevitable,
  // because searching for the superarc of cells requires information of the contour tree
  for (viskores::Id ds_no = 0; ds_no < input.GetNumberOfPartitions(); ++ds_no)
  {
    outputDataSets[ds_no] = input.GetPartition(ds_no);
  }

  // we need to send everything that contour extraction needs to the output dataset
  branch_top_volume_master.foreach (
    [&](SelectTopVolumeBranchesBlock* b, const viskoresdiy::Master::ProxyWithLink&)
    {
      viskores::cont::Field BranchVolumeField("BranchVolume",
                                              viskores::cont::Field::Association::WholeDataSet,
                                              b->TopVolumeData.BranchVolume);
      outputDataSets[b->LocalBlockNo].AddField(BranchVolumeField);
      viskores::cont::Field BranchSaddleEpsilonField(
        "BranchSaddleEpsilon",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.BranchSaddleEpsilon);
      outputDataSets[b->LocalBlockNo].AddField(BranchSaddleEpsilonField);
      viskores::cont::Field TopVolBranchUpperEndField(
        "TopVolumeBranchUpperEnd",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.TopVolumeBranchUpperEndGRId);
      outputDataSets[b->LocalBlockNo].AddField(TopVolBranchUpperEndField);
      viskores::cont::Field TopVolBranchLowerEndField(
        "TopVolumeBranchLowerEnd",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.TopVolumeBranchLowerEndGRId);
      outputDataSets[b->LocalBlockNo].AddField(TopVolBranchLowerEndField);
      viskores::cont::Field TopVolumeBranchGRIdsField(
        "TopVolumeBranchGlobalRegularIds",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.TopVolumeBranchRootGRId);
      outputDataSets[b->LocalBlockNo].AddField(TopVolumeBranchGRIdsField);
      viskores::cont::Field TopVolBranchVolumeField(
        "TopVolumeBranchVolume",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.TopVolumeBranchVolume);
      outputDataSets[b->LocalBlockNo].AddField(TopVolBranchVolumeField);
      viskores::cont::Field TopVolBranchSaddleEpsilonField(
        "TopVolumeBranchSaddleEpsilon",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.TopVolumeBranchSaddleEpsilon);
      outputDataSets[b->LocalBlockNo].AddField(TopVolBranchSaddleEpsilonField);
      viskores::cont::Field TopVolBranchSaddleIsoValueField(
        "TopVolumeBranchSaddleIsoValue",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.TopVolumeBranchSaddleIsoValue);
      outputDataSets[b->LocalBlockNo].AddField(TopVolBranchSaddleIsoValueField);

      // additional data for isosurface extraction.
      // Most of them are intermediate arrays and should not be parts of the actual output.
      // this->TopVolumeData.TopVolBranchKnownByBlockStencil
      viskores::cont::Field TopVolBranchKnownByBlockStencilField(
        "TopVolumeBranchKnownByBlockStencil",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.TopVolBranchKnownByBlockStencil);
      outputDataSets[b->LocalBlockNo].AddField(TopVolBranchKnownByBlockStencilField);
      // this->TopVolumeData.TopVolBranchInfoActualIndex
      viskores::cont::Field TopVolBranchInfoActualIndexField(
        "TopVolumeBranchInformationIndex",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.TopVolBranchInfoActualIndex);
      outputDataSets[b->LocalBlockNo].AddField(TopVolBranchInfoActualIndexField);
      // this->TopVolumeData.IsParentBranch
      viskores::cont::Field IsParentBranchField("IsParentBranch",
                                                viskores::cont::Field::Association::WholeDataSet,
                                                b->TopVolumeData.IsParentBranch);
      outputDataSets[b->LocalBlockNo].AddField(IsParentBranchField);
      // this->TopVolumeData.ExtraMaximaBranchLowerEnd
      viskores::cont::Field ExtraMaximaBranchLowerEndField(
        "ExtraMaximaBranchLowerEnd",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.ExtraMaximaBranchLowerEnd);
      outputDataSets[b->LocalBlockNo].AddField(ExtraMaximaBranchLowerEndField);
      // this->TopVolumeData.ExtraMaximaBranchUpperEnd
      viskores::cont::Field ExtraMaximaBranchUpperEndField(
        "ExtraMaximaBranchUpperEnd",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.ExtraMaximaBranchUpperEnd);
      outputDataSets[b->LocalBlockNo].AddField(ExtraMaximaBranchUpperEndField);
      // this->TopVolumeData.ExtraMaximaBranchOrder
      viskores::cont::Field ExtraMaximaBranchOrderField(
        "ExtraMaximaBranchOrder",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.ExtraMaximaBranchOrder);
      outputDataSets[b->LocalBlockNo].AddField(ExtraMaximaBranchOrderField);
      // this->TopVolumeData.ExtraMaximaBranchSaddleGRId
      viskores::cont::Field ExtraMaximaBranchSaddleGRIdField(
        "ExtraMaximaBranchSaddleGRId",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.ExtraMaximaBranchSaddleGRId);
      outputDataSets[b->LocalBlockNo].AddField(ExtraMaximaBranchSaddleGRIdField);
      // this->TopVolumeData.ExtraMaximaBranchIsoValue
      viskores::cont::Field ExtraMaximaBranchIsoValueField(
        "ExtraMaximaBranchIsoValue",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.ExtraMaximaBranchIsoValue);
      outputDataSets[b->LocalBlockNo].AddField(ExtraMaximaBranchIsoValueField);
      // this->TopVolumeData.ExtraMinimaBranchLowerEnd
      viskores::cont::Field ExtraMinimaBranchLowerEndField(
        "ExtraMinimaBranchLowerEnd",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.ExtraMinimaBranchLowerEnd);
      outputDataSets[b->LocalBlockNo].AddField(ExtraMinimaBranchLowerEndField);
      // this->TopVolumeData.ExtraMinimaBranchUpperEnd
      viskores::cont::Field ExtraMinimaBranchUpperEndField(
        "ExtraMinimaBranchUpperEnd",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.ExtraMinimaBranchUpperEnd);
      outputDataSets[b->LocalBlockNo].AddField(ExtraMinimaBranchUpperEndField);
      // this->TopVolumeData.ExtraMinimaBranchOrder
      viskores::cont::Field ExtraMinimaBranchOrderField(
        "ExtraMinimaBranchOrder",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.ExtraMinimaBranchOrder);
      outputDataSets[b->LocalBlockNo].AddField(ExtraMinimaBranchOrderField);
      // this->TopVolumeData.ExtraMinimaBranchSaddleGRId
      viskores::cont::Field ExtraMinimaBranchSaddleGRIdField(
        "ExtraMinimaBranchSaddleGRId",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.ExtraMinimaBranchSaddleGRId);
      outputDataSets[b->LocalBlockNo].AddField(ExtraMinimaBranchSaddleGRIdField);
      // this->TopVolumeData.ExtraMinimaBranchIsoValue
      viskores::cont::Field ExtraMinimaBranchIsoValueField(
        "ExtraMinimaBranchIsoValue",
        viskores::cont::Field::Association::WholeDataSet,
        b->TopVolumeData.ExtraMinimaBranchIsoValue);
      outputDataSets[b->LocalBlockNo].AddField(ExtraMinimaBranchIsoValueField);
    });

  timingsStream << "    " << std::setw(38) << std::left << "Creating Branch Selection Output Data"
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
