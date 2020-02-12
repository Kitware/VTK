/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDIYKdTreeUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDIYKdTreeUtilities.h"

#include "vtkAppendFilter.h"
#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDIYExplicitAssigner.h"
#include "vtkDIYUtilities.h"
#include "vtkIdTypeArray.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"
#include "vtkTuple.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <iterator>
#include <map>
#include <tuple>

// clang-format off
#include "vtk_diy2.h"
#include VTK_DIY2(diy/mpi.hpp)
#include VTK_DIY2(diy/master.hpp)
#include VTK_DIY2(diy/link.hpp)
#include VTK_DIY2(diy/reduce.hpp)
#include VTK_DIY2(diy/reduce-operations.hpp)
#include VTK_DIY2(diy/partners/swap.hpp)
#include VTK_DIY2(diy/assigner.hpp)
#include VTK_DIY2(diy/algorithms.hpp)
// clang-format on

namespace
{
struct PointTT
{
  vtkTuple<double, 3> coords;

  float operator[](unsigned int idx) const { return static_cast<float>(this->coords[idx]); }
};

struct BlockT
{
  std::vector<PointTT> Points;
  std::vector<diy::ContinuousBounds> BlockBounds;

  void AddPoints(vtkPoints* pts)
  {
    if (!pts)
    {
      return;
    }

    const auto start_offset = this->Points.size();
    this->Points.resize(start_offset + pts->GetNumberOfPoints());

    vtkSMPTools::For(
      0, pts->GetNumberOfPoints(), [this, pts, start_offset](vtkIdType start, vtkIdType end) {
        for (vtkIdType cc = start; cc < end; ++cc)
        {
          auto& pt = this->Points[cc + start_offset];
          pts->GetPoint(cc, pt.coords.GetData());
        }
      });
  }
};

}

//----------------------------------------------------------------------------
vtkDIYKdTreeUtilities::vtkDIYKdTreeUtilities() {}

//----------------------------------------------------------------------------
vtkDIYKdTreeUtilities::~vtkDIYKdTreeUtilities() {}

//----------------------------------------------------------------------------
void vtkDIYKdTreeUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
std::vector<vtkBoundingBox> vtkDIYKdTreeUtilities::GenerateCuts(vtkDataObject* dobj,
  int number_of_partitions, bool use_cell_centers, vtkMultiProcessController* controller,
  const double* local_bounds)
{
  double bds[6];
  vtkMath::UninitializeBounds(bds);
  if (local_bounds == nullptr)
  {
    auto bbox = vtkDIYUtilities::GetLocalBounds(dobj);
    if (bbox.IsValid())
    {
      bbox.GetBounds(bds);
    }
  }
  else
  {
    std::copy(local_bounds, local_bounds + 6, bds);
  }
  const auto datasets = vtkDIYUtilities::GetDataSets(dobj);
  const auto pts = vtkDIYUtilities::ExtractPoints(datasets, use_cell_centers);
  return vtkDIYKdTreeUtilities::GenerateCuts(pts, number_of_partitions, controller, bds);
}

//----------------------------------------------------------------------------
std::vector<vtkBoundingBox> vtkDIYKdTreeUtilities::GenerateCuts(
  const std::vector<vtkSmartPointer<vtkPoints> >& points, int number_of_partitions,
  vtkMultiProcessController* controller, const double* local_bounds /*=nullptr*/)
{
  if (number_of_partitions == 0)
  {
    return std::vector<vtkBoundingBox>();
  }

  // communicate global bounds and number of blocks.
  vtkBoundingBox bbox;
  if (local_bounds != nullptr)
  {
    bbox.SetBounds(local_bounds);
  }
  if (!bbox.IsValid())
  {
    for (auto& pts : points)
    {
      if (pts)
      {
        double bds[6];
        pts->GetBounds(bds);
        bbox.AddBounds(bds);
      }
    }
  }

  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(controller);

  // determine global domain bounds.
  vtkDIYUtilities::AllReduce(comm, bbox);

  if (!bbox.IsValid())
  {
    // nothing to split since global bounds are empty.
    return std::vector<vtkBoundingBox>();
  }

  // I am removing this. it doesn't not make sense to inflate here.
  // bbox.Inflate(0.1 * bbox.GetDiagonalLength());

  if (number_of_partitions == 1)
  {
    return std::vector<vtkBoundingBox>{ bbox };
  }

  const int num_cuts = vtkMath::NearestPowerOfTwo(number_of_partitions);
  if (num_cuts < comm.size())
  {
    // TODO: we need a MxN transfer
    vtkLogF(WARNING,
      "Requested cuts (%d) is less than number of ranks (%d), "
      "current implementation may not load balance correctly.",
      num_cuts, comm.size());
  }

  diy::Master master(
    comm, 1, -1, []() { return static_cast<void*>(new BlockT); },
    [](void* b) { delete static_cast<BlockT*>(b); });

  const diy::ContinuousBounds gdomain = vtkDIYUtilities::Convert(bbox);

  diy::ContiguousAssigner cuts_assigner(comm.size(), num_cuts);

  std::vector<int> gids;
  cuts_assigner.local_gids(comm.rank(), gids);
  for (const int gid : gids)
  {
    auto block = new BlockT();
    if (gid == gids[0])
    {
      for (size_t idx = 0; idx < points.size(); ++idx)
      {
        block->AddPoints(points[idx]);
      }
    }
    auto link = new diy::RegularContinuousLink(3, gdomain, gdomain);
    master.add(gid, block, link);
  }

  diy::kdtree(master, cuts_assigner, 3, gdomain, &BlockT::Points, /*hist_bins=*/256);

  // collect bounds for all blocks globally.
  diy::all_to_all(master, cuts_assigner, [](void* b, const diy::ReduceProxy& srp) {
    BlockT* block = reinterpret_cast<BlockT*>(b);
    if (srp.round() == 0)
    {
      for (int i = 0; i < srp.out_link().size(); ++i)
      {
        auto link = static_cast<diy::RegularContinuousLink*>(
          srp.master()->link(srp.master()->lid(srp.gid())));
        srp.enqueue(srp.out_link().target(i), link->bounds());
      }
    }
    else
    {
      block->BlockBounds.resize(srp.in_link().size());
      for (int i = 0; i < srp.in_link().size(); ++i)
      {
        assert(i == srp.in_link().target(i).gid);
        srp.dequeue(srp.in_link().target(i).gid, block->BlockBounds[i]);
      }
    }
  });

  std::vector<vtkBoundingBox> cuts(num_cuts);
  if (master.size() > 0)
  {
    const auto b0 = master.get<BlockT>(0);
    for (int gid = 0; gid < num_cuts; ++gid)
    {
      cuts[gid] = vtkDIYUtilities::Convert(b0->BlockBounds[gid]);
    }
  }

  if (num_cuts < comm.size())
  {
    // we have a case where some ranks may not have any blocks and hence will
    // not have the partition information at all. Just broadcast that info to
    // all.
    vtkDIYUtilities::Broadcast(comm, cuts, 0);
  }
  return cuts;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPartitionedDataSet> vtkDIYKdTreeUtilities::Exchange(
  vtkPartitionedDataSet* localParts, vtkMultiProcessController* controller)
{
  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(controller);
  const int nblocks = static_cast<int>(localParts->GetNumberOfPartitions());
#if !defined(NDEBUG)
  {
    // ensure that all ranks report exactly same number of partitions.
    int sumblocks = 0;
    diy::mpi::all_reduce(comm, nblocks, sumblocks, std::plus<int>());
    assert(sumblocks == nblocks * comm.size());
  }
#endif
  diy::ContiguousAssigner block_assigner(comm.size(), nblocks);

  using VectorOfUG = std::vector<vtkSmartPointer<vtkUnstructuredGrid> >;
  using VectorOfVectorOfUG = std::vector<VectorOfUG>;

  diy::Master master(
    comm, 1, -1, []() { return static_cast<void*>(new VectorOfVectorOfUG()); },
    [](void* b) { delete static_cast<VectorOfVectorOfUG*>(b); });

  diy::ContiguousAssigner assigner(comm.size(), comm.size());
  diy::RegularDecomposer<diy::DiscreteBounds> decomposer(
    /*dim*/ 1, diy::interval(0, comm.size() - 1), comm.size());
  decomposer.decompose(comm.rank(), assigner, master);
  assert(master.size() == 1);

  const int myrank = comm.rank();
  diy::all_to_all(master, assigner,
    [&block_assigner, &myrank, localParts](VectorOfVectorOfUG* block, const diy::ReduceProxy& rp) {
      if (rp.in_link().size() == 0)
      {
        // enqueue blocks to send.
        block->resize(localParts->GetNumberOfPartitions());
        for (unsigned int partId = 0; partId < localParts->GetNumberOfPartitions(); ++partId)
        {
          if (auto part = vtkUnstructuredGrid::SafeDownCast(localParts->GetPartition(partId)))
          {
            auto target_rank = block_assigner.rank(partId);
            if (target_rank == myrank)
            {
              // short-circuit messages to self.
              (*block)[partId].push_back(part);
            }
            else
            {
              rp.enqueue(rp.out_link().target(target_rank), partId);
              rp.enqueue<vtkDataSet*>(rp.out_link().target(target_rank), part);
            }
          }
        }
      }
      else
      {
        for (int i = 0; i < rp.in_link().size(); ++i)
        {
          const int gid = rp.in_link().target(i).gid;
          while (rp.incoming(gid))
          {
            unsigned int partId = 0;
            rp.dequeue(rp.in_link().target(i), partId);

            vtkDataSet* ptr = nullptr;
            rp.dequeue<vtkDataSet*>(rp.in_link().target(i), ptr);

            vtkSmartPointer<vtkUnstructuredGrid> sptr;
            sptr.TakeReference(vtkUnstructuredGrid::SafeDownCast(ptr));
            (*block)[partId].push_back(sptr);
          }
        }
      }
    });

  vtkNew<vtkPartitionedDataSet> result;
  result->SetNumberOfPartitions(localParts->GetNumberOfPartitions());
  auto block0 = *master.block<VectorOfVectorOfUG>(0);
  assert(static_cast<unsigned int>(block0.size()) == result->GetNumberOfPartitions());

  for (unsigned int cc = 0; cc < result->GetNumberOfPartitions(); ++cc)
  {
    if (block0[cc].size() == 1)
    {
      result->SetPartition(cc, block0[cc][0]);
    }
    else if (block0[cc].size() > 1)
    {
      vtkNew<vtkAppendFilter> appender;
      for (auto& ug : block0[cc])
      {
        appender->AddInputDataObject(ug);
      }
      appender->Update();
      result->SetPartition(cc, appender->GetOutputDataObject(0));
    }
  }

  return result;
}

//----------------------------------------------------------------------------
bool vtkDIYKdTreeUtilities::GenerateGlobalCellIds(vtkPartitionedDataSet* parts,
  vtkMultiProcessController* controller, vtkIdType* mb_offset /*=nullptr*/)
{
  // We need to generate global cells ids. The algorithm is simple.
  // 1. globally count non-ghost cells and then determine what range of gids
  //    each block will assign to its non-ghost cells
  // 2. each block then locally assign gids to its non-ghost cells.

  // the thing to remember that the parts here are not yet split based on cuts, as a result they
  // are not uniquely assigned among ranks. Thus number of partitions on all ranks may be different

  auto nblocks = parts->GetNumberOfPartitions();
  std::vector<vtkIdType> local_cell_counts(nblocks, 0);

  // Iterate over each part and count non-ghost cells.
  for (unsigned int partId = 0; partId < nblocks; ++partId)
  {
    if (auto ds = parts->GetPartition(partId))
    {
      auto& count = local_cell_counts[partId];

      auto ghostcells = vtkUnsignedCharArray::SafeDownCast(
        ds->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName()));
      if (ghostcells)
      {
        for (vtkIdType cc = 0; cc < ds->GetNumberOfCells(); ++cc)
        {
          const bool is_ghost =
            (ghostcells->GetTypedComponent(cc, 0) & vtkDataSetAttributes::DUPLICATECELL) != 0;
          count += is_ghost ? 0 : 1;
        }
      }
      else
      {
        count += ds->GetNumberOfCells();
      }
    }
  }

  const vtkIdType total_local_cells =
    std::accumulate(local_cell_counts.begin(), local_cell_counts.end(), 0);
  vtkIdType global_offset = 0;

  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(controller);
  diy::mpi::scan(comm, total_local_cells, global_offset, std::plus<vtkIdType>());
  // convert to exclusive scan since mpi_scan is inclusive.
  global_offset -= total_local_cells;

  // keep track of an additional offset when performing this on multiblock datasets
  if (mb_offset)
  {
    global_offset += *mb_offset;

    // need an Allreduce to get the offset for next time
    vtkIdType total_global_cells = 0;
    diy::mpi::all_reduce(comm, total_local_cells, total_global_cells, std::plus<vtkIdType>());
    (*mb_offset) += total_global_cells;
  }

  // compute exclusive scan to determine the global id offsets for each local partition.
  std::vector<vtkIdType> local_cell_offsets(nblocks, 0);
  local_cell_offsets[0] = global_offset;
  for (unsigned int cc = 1; cc < nblocks; ++cc)
  {
    local_cell_offsets[cc] = local_cell_offsets[cc - 1] + local_cell_counts[cc - 1];
  }

  // now assign global ids for non-ghost cells alone.
  for (unsigned int partId = 0; partId < nblocks; ++partId)
  {
    if (auto ds = parts->GetPartition(partId))
    {
      const auto numCells = ds->GetNumberOfCells();

      vtkNew<vtkIdTypeArray> gids;
      gids->SetName("vtkGlobalCellIds");
      gids->SetNumberOfTuples(numCells);
      auto ghostcells = vtkUnsignedCharArray::SafeDownCast(
        ds->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName()));
      auto id = local_cell_offsets[partId];
      if (ghostcells)
      {
        for (vtkIdType cc = 0; cc < numCells; ++cc)
        {
          const bool is_ghost =
            (ghostcells->GetTypedComponent(cc, 0) & vtkDataSetAttributes::DUPLICATECELL) != 0;
          gids->SetTypedComponent(cc, 0, is_ghost ? -1 : id++);
        }
      }
      else
      {
        for (vtkIdType cc = 0; cc < numCells; ++cc)
        {
          gids->SetTypedComponent(cc, 0, id++);
        }
      }

      ds->GetCellData()->SetGlobalIds(gids);
    }
  }

  return true;
}

//----------------------------------------------------------------------------
std::vector<int> vtkDIYKdTreeUtilities::ComputeAssignments(int num_blocks, int num_ranks)
{
  assert(num_blocks == vtkMath::NearestPowerOfTwo(num_blocks));

  std::vector<int> assignments(num_blocks);
  std::iota(assignments.begin(), assignments.end(), 0);

  if (num_ranks >= num_blocks)
  {
    return assignments;
  }

  const int next = vtkMath::NearestPowerOfTwo(num_ranks);
  const int divisor = num_blocks / next;
  for (auto& rank : assignments)
  {
    rank /= divisor;
  }

  const int window = divisor * 2;
  int num_windows_to_merge = (next - num_ranks);
  int rank = (num_ranks - 1);
  for (int cc = (num_blocks - window); cc >= 0 && num_windows_to_merge > 0; cc -= window)
  {
    for (int kk = 0; kk < window; ++kk)
    {
      assignments[cc + kk] = rank;
    }
    --num_windows_to_merge;
    --rank;
  }

  return assignments;
}

//----------------------------------------------------------------------------
vtkDIYExplicitAssigner vtkDIYKdTreeUtilities::CreateAssigner(
  diy::mpi::communicator& comm, int num_blocks)
{
  assert(num_blocks == vtkMath::NearestPowerOfTwo(num_blocks));

  const auto assignments = vtkDIYKdTreeUtilities::ComputeAssignments(num_blocks, comm.size());
  const int rank = comm.rank();
  int local_blocks = 0;
  for (const auto& assignment : assignments)
  {
    if (rank == assignment)
    {
      ++local_blocks;
    }
  }
  return vtkDIYExplicitAssigner(comm, local_blocks, true);
}
