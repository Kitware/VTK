// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPResolveHangingNodes.h"

#include "vtkBoundingBox.h"
#include "vtkCommunicator.h"
#include "vtkDIYExplicitAssigner.h"
#include "vtkDIYUtilities.h"
#include "vtkDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <set>
#include <vector>

// clang-format off
#include "vtk_diy2.h"
#include VTK_DIY2(diy/mpi.hpp)
#include VTK_DIY2(diy/master.hpp)
#include VTK_DIY2(diy/link.hpp)
#include VTK_DIY2(diy/decomposition.hpp)
#include VTK_DIY2(diy/reduce-operations.hpp)
#include VTK_DIY2(diy/assigner.hpp)
// clang-format on

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPResolveHangingNodes);
vtkCxxSetObjectMacro(vtkPResolveHangingNodes, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
vtkPResolveHangingNodes::vtkPResolveHangingNodes()
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkPResolveHangingNodes::~vtkPResolveHangingNodes()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
void vtkPResolveHangingNodes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

//------------------------------------------------------------------------------
bool vtkPResolveHangingNodes::UseRemotePoints()
{
  return this->Controller && this->Controller->GetNumberOfProcesses() > 1;
}

//------------------------------------------------------------------------------
void vtkPResolveHangingNodes::GetGlobalMeasureRange(double range[2], int& dimension)
{
  if (!this->UseRemotePoints())
  {
    return;
  }
  // A single reduction: the minimum is the opposite of the maximum of the opposites.
  const double local[3] = { -range[0], range[1], static_cast<double>(dimension) };
  double global[3];
  this->Controller->AllReduce(local, global, 3, vtkCommunicator::MAX_OP);
  range[0] = -global[0];
  range[1] = global[1];
  dimension = static_cast<int>(global[2]);
}

namespace
{
//------------------------------------------------------------------------------
// The data of a diy block, one per partition. BoundaryBins is
// vtkResolveHangingNodes::BoundaryBins, given by GetRemotePoints(): it is protected, so only the
// members of the filters can name it.
template <class BoundaryBins>
struct BlockT
{
  // The true boundary of the partition (see vtkResolveHangingNodes::GetRemotePoints), its bounds,
  // and the bins it occupies, only computed if the block has neighbors.
  vtkPolyData* Boundary = nullptr;
  vtkBoundingBox BoundaryBounds;
  BoundaryBins OccupiedBins;

  // -1: the block is linked to all the other blocks. Otherwise, the index of the
  // vtkPartitionedDataSet of the partition, e.g., an AMR level, whose partitions are conforming
  // with it: the block is only linked to the blocks of other vtkPartitionedDataSet (see
  // vtkResolveHangingNodes::GetRemotePoints).
  int PartitionedDataSetId = -1;

  // Bounds of the points to share of the other blocks, for the blocks that have some.
  std::map<int, vtkBoundingBox> Bounds;

  // Bins of the boundary of the neighbor blocks.
  std::map<int, BoundaryBins> Bins;

  // Points received from the other blocks.
  std::map<int, vtkSmartPointer<vtkPolyData>> Received;
};

//------------------------------------------------------------------------------
// Share the bounds of every block with all the blocks: a flag telling if there are some, the
// bounds (see vtkDIYGhostUtilities::ExchangeBoundingBoxes), then the index of the
// vtkPartitionedDataSet of the block. They are stored in Block::Bounds, unless the block has an
// index, not -1, and the other block the same one: both are then in the same conforming
// vtkPartitionedDataSet, e.g., an AMR level.
template <class Block>
void ExchangeBounds(diy::Master& master, const vtkDIYExplicitAssigner& assigner)
{
  diy::all_to_all(master, assigner,
    [](Block* block, const diy::ReduceProxy& rp)
    {
      const int myBlockId = rp.gid();
      if (rp.round() == 0)
      {
        const vtkBoundingBox& bounds = block->BoundaryBounds;
        double data[8] = { bounds.IsValid() ? 1.0 : 0.0, 0, 0, 0, 0, 0, 0,
          static_cast<double>(block->PartitionedDataSetId) };
        if (bounds.IsValid())
        {
          bounds.GetBounds(data + 1);
        }
        for (int i = 0; i < rp.out_link().size(); ++i)
        {
          if (rp.out_link().target(i).gid != myBlockId)
          {
            rp.enqueue(rp.out_link().target(i), data, 8);
          }
        }
      }
      else
      {
        for (int i = 0; i < rp.in_link().size(); ++i)
        {
          const diy::BlockID& source = rp.in_link().target(i);
          if (source.gid != myBlockId)
          {
            double data[8];
            rp.dequeue(source, data, 8);
            const int partitionedDataSetId = static_cast<int>(data[7]);
            if (data[0] != 0.0 &&
              (block->PartitionedDataSetId < 0 ||
                partitionedDataSetId != block->PartitionedDataSetId))
            {
              block->Bounds.emplace(source.gid, vtkBoundingBox(data + 1));
            }
          }
        }
      }
    });
}

//------------------------------------------------------------------------------
// Link the blocks whose bounds overlap. The links are symmetric, since the overlap test is
// symmetric (see vtkDIYGhostUtilities::ComputeLinkMapUsingBoundingBoxes).
template <class Block>
void LinkOverlappingBlocks(diy::Master& master, const vtkDIYExplicitAssigner& assigner)
{
  std::vector<std::set<int>> linkMap(master.size());
  for (int localId = 0; localId < static_cast<int>(master.size()); ++localId)
  {
    const Block* block = master.block<Block>(localId);
    if (block->BoundaryBounds.IsValid())
    {
      for (const auto& [blockId, boundingBox] : block->Bounds)
      {
        if (block->BoundaryBounds.Intersects(boundingBox))
        {
          linkMap[localId].insert(blockId);
        }
      }
    }
  }
  vtkDIYUtilities::Link(master, assigner, linkMap);
}

//------------------------------------------------------------------------------
// Send the occupied bins of every block to its neighbors, and receive theirs in Block::Bins. Their
// bins follow from the bounds already received in Block::Bounds, so only the occupied bits are
// exchanged.
template <class Block>
void ExchangeBoundaryBins(diy::Master& master)
{
  master.foreach (
    [](Block* block, const diy::Master::ProxyWithLink& cp)
    {
      for (int i = 0; i < cp.link()->size(); ++i)
      {
        cp.enqueue(cp.link()->target(i), block->OccupiedBins.Occupied);
      }
    });
  master.exchange();
  master.foreach (
    [](Block* block, const diy::Master::ProxyWithLink& cp)
    {
      for (int i = 0; i < cp.link()->size(); ++i)
      {
        const int gid = cp.link()->target(i).gid;
        auto& bins = block->Bins[gid];
        bins.Initialize(block->Bounds.at(gid));
        cp.dequeue(gid, bins.Occupied);
      }
    });
}

//------------------------------------------------------------------------------
// Send to each neighbor of every block the points of its boundary that are in the occupied bins of
// the neighbor, and receive the points of the neighbors in Block::Received (see
// vtkDIYGhostUtilities::ExchangeGhosts and EnqueueGhosts). extractPointsInBins is
// vtkResolveHangingNodes::ExtractPointsInBins, protected as well.
template <class Block, class ExtractPointsInBins>
void ExchangePoints(diy::Master& master, ExtractPointsInBins extractPointsInBins)
{
  master.foreach (
    [&extractPointsInBins](Block* block, const diy::Master::ProxyWithLink& cp)
    {
      // Without points to share, the bounds are invalid and there is no neighbor.
      for (int i = 0; i < cp.link()->size(); ++i)
      {
        const diy::BlockID& target = cp.link()->target(i);
        vtkSmartPointer<vtkPolyData> points =
          extractPointsInBins(block->Boundary, block->Bins.at(target.gid));
        vtkDataSet* dataSet = points;
        cp.enqueue<vtkDataSet*>(target, dataSet);
      }
    });
  master.exchange();
  master.foreach (
    [](Block* block, const diy::Master::ProxyWithLink& cp)
    {
      std::vector<int> incoming;
      cp.incoming(incoming);
      for (int gid : incoming)
      {
        // incoming can list blocks that sent nothing to this one
        if (!cp.incoming(gid).empty())
        {
          // Only vtkDataSet* has a serialization in vtkDIYUtilities: with another pointer type, diy
          // would copy the address of the object instead of its data.
          vtkDataSet* dataSet = nullptr;
          cp.dequeue<vtkDataSet*>(gid, dataSet);
          auto received = vtk::TakeSmartPointer(dataSet);
          block->Received.emplace(gid, vtkPolyData::SafeDownCast(received));
        }
      }
    });
}
} // namespace

//------------------------------------------------------------------------------
// The communication is a simpler version of the one of the ghost cell generator, see
// vtkDIYGhostUtilities::GenerateGhostCells in Parallel/DIY/vtkDIYGhostUtilities.txx, which has the
// same steps: one diy block per partition, ExchangeBoundingBoxes, ComputeLinkMapUsingBoundingBoxes
// then vtkDIYUtilities::Link, and ExchangeGhosts (enqueue, master.exchange, dequeue). It does not
// exchange the structure of the blocks to refine the links, since the overlap of the bounds is
// enough here. The ghost cell generator cannot be used instead: it connects cells of partitions
// that have the same points on their common boundary, which is what this function provides.
std::vector<vtkSmartPointer<vtkPolyData>> vtkPResolveHangingNodes::GetRemotePoints(
  const std::vector<vtkPolyData*>& boundaries, const std::vector<int>& partitionedDataSetIds)
{
  if (!this->UseRemotePoints())
  {
    return this->Superclass::GetRemotePoints(boundaries, partitionedDataSetIds);
  }

  // Set up diy, with one block per partition (see "Decomposing master" in GenerateGhostCells). A
  // process without partitions has one block without boundary, so that it takes part in the
  // exchange.
  using Block = BlockT<BoundaryBins>;
  const int numBlocks = std::max(static_cast<int>(boundaries.size()), 1);
  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(this->Controller);
  diy::Master master(
    comm, 1, -1, []() -> void* { return static_cast<void*>(new Block()); },
    [](void* b) -> void { delete static_cast<Block*>(b); });
  vtkDIYExplicitAssigner assigner(comm, numBlocks);
  diy::RegularDecomposer<diy::DiscreteBounds> decomposer(
    /*dim*/ 1, diy::interval(0, assigner.nblocks() - 1), assigner.nblocks());
  decomposer.decompose(comm.rank(), assigner, master);

  // The boundary of every partition, its bounds, and the index of its vtkPartitionedDataSet.
  for (std::size_t localId = 0; localId < boundaries.size(); ++localId)
  {
    Block* block = master.block<Block>(static_cast<int>(localId));
    block->PartitionedDataSetId = partitionedDataSetIds[localId];
    block->Boundary = boundaries[localId];
    block->BoundaryBounds =
      vtkResolveHangingNodes::GetBoundaryBounds(block->Boundary, this->Tolerance);
  }

  // The neighbors are linked by bounds.
  ExchangeBounds<Block>(master, assigner);
  LinkOverlappingBlocks<Block>(master, assigner);

  // The bins that the boundary of every block occupies, only needed with neighbors.
  for (int localId = 0; localId < static_cast<int>(master.size()); ++localId)
  {
    if (master.link(localId)->size() > 0)
    {
      Block* block = master.block<Block>(localId);
      block->OccupiedBins.Initialize(block->BoundaryBounds);
      vtkResolveHangingNodes::BinBoundaryElements(
        block->OccupiedBins, block->Boundary, this->Tolerance);
    }
  }

  // The neighbors receive only the points in the occupied bins.
  ExchangeBoundaryBins<Block>(master);
  ExchangePoints<Block>(master, &vtkResolveHangingNodes::ExtractPointsInBins);

  std::vector<vtkSmartPointer<vtkPolyData>> remotePoints(boundaries.size());
  for (std::size_t localId = 0; localId < boundaries.size(); ++localId)
  {
    const Block* block = master.block<Block>(static_cast<int>(localId));
    remotePoints[localId] = this->MergeReceivedPoints(block->Received, block->Boundary);
  }
  return remotePoints;
}

//------------------------------------------------------------------------------
void vtkPResolveHangingNodes::GetCentroidGlobalIdsOffset(vtkIdType& maxGlobalId,
  vtkIdType numberOfCentroidPoints, vtkIdType& offset, vtkIdType& totalNumberOfCentroidPoints)
{
  offset = 0;
  totalNumberOfCentroidPoints = numberOfCentroidPoints;
  if (!this->UseRemotePoints())
  {
    return;
  }
  vtkIdType globalMaxGlobalId = -1;
  this->Controller->AllReduce(&maxGlobalId, &globalMaxGlobalId, 1, vtkCommunicator::MAX_OP);
  maxGlobalId = globalMaxGlobalId;
  // The numbers of center points of all the processes, to sum those of lower rank
  std::vector<vtkIdType> numbersOfCentroidPoints(this->Controller->GetNumberOfProcesses());
  this->Controller->AllGather(&numberOfCentroidPoints, numbersOfCentroidPoints.data(), 1);
  offset = std::accumulate(numbersOfCentroidPoints.begin(),
    numbersOfCentroidPoints.begin() + this->Controller->GetLocalProcessId(),
    static_cast<vtkIdType>(0));
  totalNumberOfCentroidPoints = std::accumulate(
    numbersOfCentroidPoints.begin(), numbersOfCentroidPoints.end(), static_cast<vtkIdType>(0));
}
VTK_ABI_NAMESPACE_END
