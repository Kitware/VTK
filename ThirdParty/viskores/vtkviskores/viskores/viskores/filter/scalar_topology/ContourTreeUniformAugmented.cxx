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

#include <viskores/filter/scalar_topology/ContourTreeUniformAugmented.h>
#include <viskores/filter/scalar_topology/internal/ComputeBlockIndices.h>
#include <viskores/filter/scalar_topology/worklet/ContourTreeUniformAugmented.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/ContourTreeMesh.h>

// clang-format off
VISKORES_THIRDPARTY_PRE_INCLUDE
#include <viskores/thirdparty/diy/Configure.h>
#include <viskores/thirdparty/diy/diy.h>
VISKORES_THIRDPARTY_POST_INCLUDE
// clang-format on

#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/ContourTreeBlockData.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/MergeBlockFunctor.h>

#include <memory>

namespace viskores
{
namespace filter
{
namespace scalar_topology
{

//-----------------------------------------------------------------------------
ContourTreeAugmented::ContourTreeAugmented(bool useMarchingCubes,
                                           unsigned int computeRegularStructure)
  : UseMarchingCubes(useMarchingCubes)
  , ComputeRegularStructure(computeRegularStructure)
  , MultiBlockTreeHelper(nullptr)
{
  this->SetOutputFieldName("resultData");
}

void ContourTreeAugmented::SetBlockIndices(
  viskores::Id3 blocksPerDim,
  const viskores::cont::ArrayHandle<viskores::Id3>& localBlockIndices)
{
  if (this->MultiBlockTreeHelper)
  {
    this->MultiBlockTreeHelper.reset();
  }
  this->MultiBlockTreeHelper =
    std::make_unique<viskores::worklet::contourtree_distributed::MultiBlockContourTreeHelper>(
      blocksPerDim, localBlockIndices);
}

const viskores::worklet::contourtree_augmented::ContourTree& ContourTreeAugmented::GetContourTree()
  const
{
  return this->ContourTreeData;
}

const viskores::worklet::contourtree_augmented::IdArrayType& ContourTreeAugmented::GetSortOrder()
  const
{
  return this->MeshSortOrder;
}

viskores::Id ContourTreeAugmented::GetNumIterations() const
{
  return this->NumIterations;
}

//-----------------------------------------------------------------------------
viskores::cont::DataSet ContourTreeAugmented::DoExecute(const viskores::cont::DataSet& input)
{
  viskores::cont::Timer timer;
  timer.Start();

  // Check that the field is Ok
  const auto& field = this->GetFieldFromDataSet(input);
  if (!field.IsPointField())
  {
    throw viskores::cont::ErrorFilterExecution("Point field expected.");
  }

  // Use the GetPointDimensions struct defined in the header to collect the meshSize information
  viskores::Id3 meshSize;
  const auto& cells = input.GetCellSet();
  cells.CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>(
    viskores::worklet::contourtree_augmented::GetPointDimensions(), meshSize);

  // TODO blockIndex needs to change if we have multiple blocks per MPI rank and DoExecute is called for multiple blocks
  std::size_t blockIndex = 0;

  // Determine if and what augmentation we need to do
  unsigned int compRegularStruct = this->ComputeRegularStructure;
  // When running in parallel we need to at least augment with the boundary vertices
  if (compRegularStruct == 0)
  {
    if (this->MultiBlockTreeHelper)
    {
      if (this->MultiBlockTreeHelper->GetGlobalNumberOfBlocks() > 1)
      {
        compRegularStruct = 2; // Compute boundary augmentation
      }
    }
  }

  // Create the result object
  viskores::cont::DataSet result;

  // FIXME: reduce the size of lambda.
  auto resolveType = [&](const auto& concrete)
  {
    using T = typename std::decay_t<decltype(concrete)>::ValueType;

    viskores::worklet::ContourTreeAugmented worklet;
    // Run the worklet
    worklet.Run(concrete,
                MultiBlockTreeHelper ? MultiBlockTreeHelper->LocalContourTrees[blockIndex]
                                     : this->ContourTreeData,
                MultiBlockTreeHelper ? MultiBlockTreeHelper->LocalSortOrders[blockIndex]
                                     : this->MeshSortOrder,
                this->NumIterations,
                meshSize,
                this->UseMarchingCubes,
                compRegularStruct);

    // If we run in parallel but with only one global block, then we need set our outputs correctly
    // here to match the expected behavior in parallel
    if (this->MultiBlockTreeHelper)
    {
      if (this->MultiBlockTreeHelper->GetGlobalNumberOfBlocks() == 1)
      {
        // Copy the contour tree and mesh sort order to the output
        this->ContourTreeData = this->MultiBlockTreeHelper->LocalContourTrees[0];
        this->MeshSortOrder = this->MultiBlockTreeHelper->LocalSortOrders[0];
        // In parallel we need the sorted values as output resulti
        // Construct the sorted values by permutting the input field
        auto fieldPermutted =
          viskores::cont::make_ArrayHandlePermutation(this->MeshSortOrder, concrete);
        // FIXME: can sortedValues be ArrayHandleUnknown?
        viskores::cont::ArrayHandle<T> sortedValues;
        viskores::cont::Algorithm::Copy(fieldPermutted, sortedValues);

        // FIXME: is this the right way to create the DataSet? The original code creates an empty
        //  DataSet without any coordinate system etc.
        result = this->CreateResultField(input,
                                         this->GetOutputFieldName(),
                                         viskores::cont::Field::Association::WholeDataSet,
                                         sortedValues);
        //        viskores::cont::Field rfield(
        //          this->GetOutputFieldName(), viskores::cont::Field::Association::WholeDataSet, sortedValues);
        //        result.AddField(rfield);
        //        return result;
      }
    }
    else
    {
      // Construct the expected result for serial execution. Note, in serial the result currently
      // not actually being used, but in parallel we need the sorted mesh values as output
      // This part is being hit when we run in serial or parallel with more than one rank.
      result =
        this->CreateResultFieldPoint(input, this->GetOutputFieldName(), ContourTreeData.Arcs);
      //  return CreateResultFieldPoint(input, ContourTreeData.Arcs, this->GetOutputFieldName());
    }
  };
  this->CastAndCallScalarField(field, resolveType);

  VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
                 std::endl
                   << "    " << std::setw(38) << std::left << "Contour Tree Filter DoExecute"
                   << ": " << timer.GetElapsedTime() << " seconds");

  return result;
} // ContourTreeAugmented::DoExecute

// TODO: is multiblock case ever tested?
VISKORES_CONT viskores::cont::PartitionedDataSet ContourTreeAugmented::DoExecutePartitions(
  const viskores::cont::PartitionedDataSet& input)
{
  this->PreExecute(input);
  auto result = this->Filter::DoExecutePartitions(input);
  this->PostExecute(input, result);
  return result;
}

//-----------------------------------------------------------------------------
VISKORES_CONT void ContourTreeAugmented::PreExecute(const viskores::cont::PartitionedDataSet& input)
{
  if (this->MultiBlockTreeHelper)
  {
    if (input.GetGlobalNumberOfPartitions() !=
        this->MultiBlockTreeHelper->GetGlobalNumberOfBlocks())
    {
      throw viskores::cont::ErrorFilterExecution(
        "Global number of block in MultiBlock dataset does not match the SpatialDecomposition");
    }
    if (this->MultiBlockTreeHelper->GetLocalNumberOfBlocks() != input.GetNumberOfPartitions())
    {
      throw viskores::cont::ErrorFilterExecution(
        "Global number of block in MultiBlock dataset does not match the SpatialDecomposition");
    }
  }
  else
  {
    // No block indices set -> compute information automatically later
    this->MultiBlockTreeHelper =
      std::make_unique<viskores::worklet::contourtree_distributed::MultiBlockContourTreeHelper>(
        input);
  }
}

//-----------------------------------------------------------------------------
template <typename T>
VISKORES_CONT void ContourTreeAugmented::DoPostExecute(
  const viskores::cont::PartitionedDataSet& input,
  viskores::cont::PartitionedDataSet& output)
{
  auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  viskores::Id size = comm.size();
  viskores::Id rank = comm.rank();

  std::vector<viskores::worklet::contourtree_augmented::ContourTreeMesh<T>*> localContourTreeMeshes;
  localContourTreeMeshes.resize(static_cast<std::size_t>(input.GetNumberOfPartitions()));
  // TODO need to allocate and free these ourselves. May need to update detail::MultiBlockContourTreeHelper::ComputeLocalContourTreeMesh
  std::vector<viskores::worklet::contourtree_distributed::ContourTreeBlockData<T>*> localDataBlocks;
  localDataBlocks.resize(static_cast<size_t>(input.GetNumberOfPartitions()));
  std::vector<viskoresdiy::Link*> localLinks; // dummy links needed to make DIY happy
  localLinks.resize(static_cast<size_t>(input.GetNumberOfPartitions()));
  // We need to augment at least with the boundary vertices when running in parallel, even if the user requested at the end only the unaugmented contour tree
  unsigned int compRegularStruct =
    (this->ComputeRegularStructure > 0) ? this->ComputeRegularStructure : 2;

  for (std::size_t bi = 0; bi < static_cast<std::size_t>(input.GetNumberOfPartitions()); bi++)
  {
    // create the local contour tree mesh
    localLinks[bi] = new viskoresdiy::Link;
    auto currBlock = input.GetPartition(static_cast<viskores::Id>(bi));
    auto currField =
      currBlock.GetField(this->GetActiveFieldName(), this->GetActiveFieldAssociation());

    viskores::Id3 pointDimensions, globalPointDimensions, globalPointIndexStart;
    currBlock.GetCellSet().CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>(
      viskores::worklet::contourtree_augmented::GetLocalAndGlobalPointDimensions(),
      pointDimensions,
      globalPointDimensions,
      globalPointIndexStart);

    //const viskores::cont::ArrayHandle<T,StorageType> &fieldData = currField.GetData().Cast<viskores::cont::ArrayHandle<T,StorageType> >();
    viskores::cont::ArrayHandle<T> fieldData;
    viskores::cont::ArrayCopy(currField.GetData(), fieldData);
    auto currContourTreeMesh =
      viskores::worklet::contourtree_distributed::MultiBlockContourTreeHelper::
        ComputeLocalContourTreeMesh<T>(globalPointIndexStart,
                                       pointDimensions,
                                       globalPointDimensions,
                                       fieldData,
                                       MultiBlockTreeHelper->LocalContourTrees[bi],
                                       MultiBlockTreeHelper->LocalSortOrders[bi],
                                       compRegularStruct);
    localContourTreeMeshes[bi] = currContourTreeMesh;
    // create the local data block structure
    localDataBlocks[bi] = new viskores::worklet::contourtree_distributed::ContourTreeBlockData<T>();
    localDataBlocks[bi]->NumVertices = currContourTreeMesh->NumVertices;
    // localDataBlocks[bi]->SortOrder = currContourTreeMesh->SortOrder;
    localDataBlocks[bi]->SortedValue = currContourTreeMesh->SortedValues;
    localDataBlocks[bi]->GlobalMeshIndex = currContourTreeMesh->GlobalMeshIndex;
    localDataBlocks[bi]->NeighborConnectivity = currContourTreeMesh->NeighborConnectivity;
    localDataBlocks[bi]->NeighborOffsets = currContourTreeMesh->NeighborOffsets;
    localDataBlocks[bi]->MaxNeighbors = currContourTreeMesh->MaxNeighbors;
    localDataBlocks[bi]->BlockOrigin = globalPointIndexStart;
    localDataBlocks[bi]->BlockSize = pointDimensions;
    localDataBlocks[bi]->GlobalSize = globalPointDimensions;
    // We need to augment at least with the boundary vertices when running in parallel
    localDataBlocks[bi]->ComputeRegularStructure = compRegularStruct;
  }
  // Setup viskoresdiy to do global binary reduction of neighbouring blocks. See also RecuctionOperation struct for example

  // Create the viskoresdiy master
  viskoresdiy::Master master(comm,
                             1, // Use 1 thread, Viskores will do the treading
                             -1 // All block in memory
  );

  // Compute the gids for our local blocks
  using RegularDecomposer = viskoresdiy::RegularDecomposer<viskoresdiy::DiscreteBounds>;

  RegularDecomposer::DivisionsVector diyDivisions;
  std::vector<int> viskoresdiyLocalBlockGids;
  viskoresdiy::DiscreteBounds diyBounds(0);
  if (this->MultiBlockTreeHelper->BlocksPerDimension[0] == -1)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   "BlocksPerDimension not set. Computing block indices "
                   "from information in CellSetStructured.");
    diyBounds = viskores::filter::scalar_topology::internal::ComputeBlockIndices(
      input, diyDivisions, viskoresdiyLocalBlockGids);
  }
  else
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   "BlocksPerDimension set. Using information provided by caller.");
    diyBounds = viskores::filter::scalar_topology::internal::ComputeBlockIndices(
      input,
      this->MultiBlockTreeHelper->BlocksPerDimension,
      this->MultiBlockTreeHelper->LocalBlockIndices,
      diyDivisions,
      viskoresdiyLocalBlockGids);
  }
  int numDims = diyBounds.min.dimension();
  int globalNumberOfBlocks =
    std::accumulate(diyDivisions.cbegin(), diyDivisions.cend(), 1, std::multiplies<int>{});

  // Add my local blocks to the viskoresdiy master.
  for (std::size_t bi = 0; bi < static_cast<std::size_t>(input.GetNumberOfPartitions()); bi++)
  {
    master.add(static_cast<int>(viskoresdiyLocalBlockGids[bi]), // block id
               localDataBlocks[bi],
               localLinks[bi]);
  }

  // Define the decomposition of the domain into regular blocks
  RegularDecomposer::BoolVector shareFace(3, true);
  RegularDecomposer::BoolVector wrap(3, false);
  RegularDecomposer::CoordinateVector ghosts(3, 1);
  RegularDecomposer decomposer(static_cast<int>(numDims),
                               diyBounds,
                               globalNumberOfBlocks,
                               shareFace,
                               wrap,
                               ghosts,
                               diyDivisions);

  // Define which blocks live on which rank so that viskoresdiy can manage them
  viskoresdiy::DynamicAssigner assigner(comm, static_cast<int>(size), globalNumberOfBlocks);
  for (viskores::Id bi = 0; bi < input.GetNumberOfPartitions(); bi++)
  {
    assigner.set_rank(static_cast<int>(rank),
                      static_cast<int>(viskoresdiyLocalBlockGids[static_cast<size_t>(bi)]));
  }

  // Fix the viskoresdiy links. (NOTE: includes an MPI barrier)
  viskoresdiy::fix_links(master, assigner);

  // partners for merge over regular block grid
  viskoresdiy::RegularMergePartners partners(
    decomposer, // domain decomposition
    2,          // raix of k-ary reduction.
    true        // contiguous: true=distance doubling , false=distnace halving
  );
  // reduction
  viskoresdiy::reduce(
    master, assigner, partners, &viskores::worklet::contourtree_distributed::MergeBlockFunctor<T>);

  comm.barrier(); // Be safe!

  if (rank == 0)
  {
    viskores::Id3 dummy1, globalPointDimensions, dummy2;
    viskores::cont::DataSet firstDS = input.GetPartition(0);
    firstDS.GetCellSet().CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>(
      viskores::worklet::contourtree_augmented::GetLocalAndGlobalPointDimensions(),
      dummy1,
      globalPointDimensions,
      dummy2);
    // Now run the contour tree algorithm on the last block to compute the final tree
    viskores::Id currNumIterations;
    viskores::worklet::contourtree_augmented::ContourTree currContourTree;
    viskores::worklet::contourtree_augmented::IdArrayType currSortOrder;
    viskores::worklet::ContourTreeAugmented worklet;
    viskores::cont::ArrayHandle<T> currField;
    // Construct the contour tree mesh from the last block
    viskores::worklet::contourtree_augmented::ContourTreeMesh<T> contourTreeMeshOut;
    contourTreeMeshOut.NumVertices = localDataBlocks[0]->NumVertices;
    contourTreeMeshOut.SortOrder = viskores::cont::ArrayHandleIndex(contourTreeMeshOut.NumVertices);
    contourTreeMeshOut.SortIndices =
      viskores::cont::ArrayHandleIndex(contourTreeMeshOut.NumVertices);
    contourTreeMeshOut.SortedValues = localDataBlocks[0]->SortedValue;
    contourTreeMeshOut.GlobalMeshIndex = localDataBlocks[0]->GlobalMeshIndex;
    contourTreeMeshOut.NeighborConnectivity = localDataBlocks[0]->NeighborConnectivity;
    contourTreeMeshOut.NeighborOffsets = localDataBlocks[0]->NeighborOffsets;
    contourTreeMeshOut.MaxNeighbors = localDataBlocks[0]->MaxNeighbors;
    // Construct the mesh boundary exectuion object needed for boundary augmentation
    viskores::Id3 minIdx(0, 0, 0);
    viskores::Id3 maxIdx = globalPointDimensions;
    maxIdx[0] = maxIdx[0] - 1;
    maxIdx[1] = maxIdx[1] - 1;
    maxIdx[2] = maxIdx[2] > 0 ? (maxIdx[2] - 1) : 0;
    auto meshBoundaryExecObj =
      contourTreeMeshOut.GetMeshBoundaryExecutionObject(globalPointDimensions, minIdx, maxIdx);
    // Run the worklet to compute the final contour tree
    worklet.Run(
      contourTreeMeshOut.SortedValues, // Unused param. Provide something to keep API happy
      contourTreeMeshOut,
      this->ContourTreeData,
      this->MeshSortOrder,
      currNumIterations,
      this->ComputeRegularStructure,
      meshBoundaryExecObj);

    // Set the final mesh sort order we need to use
    this->MeshSortOrder = contourTreeMeshOut.GlobalMeshIndex;
    // Remeber the number of iterations for the output
    this->NumIterations = currNumIterations;

    // Return the sorted values of the contour tree as the result
    // TODO the result we return for the parallel and serial case are different right now. This should be made consistent. However, only in the parallel case are we useing the result output
    viskores::cont::DataSet temp;
    viskores::cont::Field rfield(this->GetOutputFieldName(),
                                 viskores::cont::Field::Association::WholeDataSet,
                                 contourTreeMeshOut.SortedValues);
    temp.AddField(rfield);
    output = viskores::cont::PartitionedDataSet(temp);
  }
  else
  {
    this->ContourTreeData = MultiBlockTreeHelper->LocalContourTrees[0];
    this->MeshSortOrder = MultiBlockTreeHelper->LocalSortOrders[0];

    // Free allocated temporary pointers
    for (std::size_t bi = 0; bi < static_cast<std::size_t>(input.GetNumberOfPartitions()); bi++)
    {
      delete localContourTreeMeshes[bi];
      delete localDataBlocks[bi];
      // delete localLinks[bi];
    }
  }
  localContourTreeMeshes.clear();
  localDataBlocks.clear();
  localLinks.clear();
}

//-----------------------------------------------------------------------------
VISKORES_CONT void ContourTreeAugmented::PostExecute(
  const viskores::cont::PartitionedDataSet& input,
  viskores::cont::PartitionedDataSet& result)
{
  if (this->MultiBlockTreeHelper)
  {
    viskores::cont::Timer timer;
    timer.Start();

    // We are running in parallel and need to merge the contour tree in PostExecute
    if (MultiBlockTreeHelper->GetGlobalNumberOfBlocks() == 1)
    {
      return;
    }

    auto field =
      input.GetPartition(0).GetField(this->GetActiveFieldName(), this->GetActiveFieldAssociation());

    // To infer and pass on the ValueType of the field.
    auto PostExecuteCaller = [&](const auto& concrete)
    {
      using T = typename std::decay_t<decltype(concrete)>::ValueType;
      this->DoPostExecute<T>(input, result);
    };
    this->CastAndCallScalarField(field, PostExecuteCaller);

    this->MultiBlockTreeHelper.reset();
    VISKORES_LOG_S(viskores::cont::LogLevel::Perf,
                   std::endl
                     << "    " << std::setw(38) << std::left << "Contour Tree Filter PostExecute"
                     << ": " << timer.GetElapsedTime() << " seconds");
  }
}

} // namespace scalar_topology
} // namespace filter
} // namespace viskores
