/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenerateGlobalIds.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenerateGlobalIds.h"

#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDIYUtilities.h"
#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStaticPointLocator.h"
#include "vtkTuple.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <climits>
#include <tuple>
#include <type_traits>

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

namespace impl
{

static vtkBoundingBox AllReduceBounds(
  diy::mpi::communicator& comm, std::vector<vtkSmartPointer<vtkPoints> > points)
{
  vtkBoundingBox bbox;
  for (auto& pts : points)
  {
    if (pts)
    {
      double bds[6];
      pts->GetBounds(bds);
      bbox.AddBounds(bds);
    }
  }
  vtkDIYUtilities::AllReduce(comm, bbox);
  return bbox;
}

static std::vector<int> AllGatherBlockCounts(diy::mpi::communicator& comm, int local_nblocks)
{
  std::vector<int> global_block_counts;
  diy::mpi::all_gather(comm, local_nblocks, global_block_counts);

  const int global_num_blocks =
    std::accumulate(global_block_counts.begin(), global_block_counts.end(), 0);

  const int global_block_counts_pow_2 =
    (global_num_blocks == 1) ? 2 : vtkMath::NearestPowerOfTwo(global_num_blocks);

  // since kd-tree needs pow-of-2, we pad each rank with extra blocks
  auto extra_blocks = global_block_counts_pow_2 - global_num_blocks;
  const auto extra_blocks_per_rank = static_cast<int>(std::ceil(extra_blocks * 1.0 / comm.size()));
  for (auto& count : global_block_counts)
  {
    if (extra_blocks > 0)
    {
      const auto padding = std::min(extra_blocks_per_rank, extra_blocks);
      count += padding;
      extra_blocks -= padding;
    }
  }
  assert(std::accumulate(global_block_counts.begin(), global_block_counts.end(), 0) ==
    global_block_counts_pow_2);
  return global_block_counts;
}

class ExplicitAssigner : public ::diy::StaticAssigner
{
  std::vector<int> GIDs;

public:
  ExplicitAssigner(const std::vector<int>& counts)
    : diy::StaticAssigner(
        static_cast<int>(counts.size()), std::accumulate(counts.begin(), counts.end(), 0))
    , GIDs(counts)
  {
    for (size_t cc = 1; cc < this->GIDs.size(); ++cc)
    {
      this->GIDs[cc] += this->GIDs[cc - 1];
    }
  }

  //! returns the process rank of the block with global id gid (need not be local)
  int rank(int gid) const override
  {
    for (size_t cc = 0; cc < this->GIDs.size(); ++cc)
    {
      if (gid < this->GIDs[cc])
      {
        return static_cast<int>(cc);
      }
    }
    abort();
  }

  //! gets the local gids for a given process rank
  void local_gids(int rank, std::vector<int>& gids) const override
  {
    const auto min = rank == 0 ? 0 : this->GIDs[rank - 1];
    const auto max = this->GIDs[rank];
    gids.resize(max - min);
    std::iota(gids.begin(), gids.end(), min);
  }
};

template<typename T>
struct BlockTraits
{
};

/**
 * This is the main implementation of the global id generation algorithm.
 * The code is similar for both point and cell ids generation except small
 * differences that are implemented using `BlockTraits` and `BlockT`.
 *
 * The general algorithm can be described as:
 * - sort points (or cells) globally so that all "coincident" points (or cells)
 *   are within the same block;
 * - merge coincident points (or cells) per block and assign unique ids for
 *   unique points (or cells) -- note this is local to each block since we know
 *   all coincident points are same block after earlier step;
 * - uniquify the generated ids globally by exchanging information of local
 *   unique id counts;
 * - communicate back the assigned unique id to the source block where the point
 *   (or cell) came from.
 */
template<typename BlockT>
static bool GenerateIds(vtkDataObject* dobj, vtkGenerateGlobalIds* self)
{
  using BlockTraitsT = BlockTraits<BlockT>;

  self->UpdateProgress(0.0);
  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(self->GetController());

  vtkLogStartScope(TRACE, "extract points");
  const auto datasets = vtkDIYUtilities::GetDataSets(dobj);
  const auto points = vtkDIYUtilities::ExtractPoints(datasets, BlockTraitsT::UseCellCenters);
  vtkLogEndScope("extract points");

  // get the bounds for the domain globally.
  const diy::ContinuousBounds gdomain =
    vtkDIYUtilities::Convert(impl::AllReduceBounds(comm, points));

  const int local_num_blocks = static_cast<int>(points.size());
  // note, these may be padded on each rank so that the total number of blocks
  // is a power-of-2 as needed by diy::kdtree
  const std::vector<int> global_block_counts = impl::AllGatherBlockCounts(comm, local_num_blocks);

  diy::Master master(comm, 1, -1, []() { return static_cast<void*>(new BlockT); },
    [](void* b) { delete static_cast<BlockT*>(b); });

  impl::ExplicitAssigner assigner(global_block_counts);

  vtkLogStartScope(TRACE, "populate master");
  std::vector<int> gids;
  assigner.local_gids(comm.rank(), gids);
  for (size_t lid = 0; lid < gids.size(); ++lid)
  {
    auto block = new BlockT();
    if (lid < points.size() && points[lid] != nullptr)
    {
      assert(datasets[lid] != nullptr);
      block->Initialize(gids[lid], points[lid], datasets[lid]);
    }

    auto link = new diy::RegularContinuousLink(3, gdomain, gdomain);
    master.add(gids[lid], block, link);
  }
  vtkLogEndScope("populate master");
  self->UpdateProgress(0.25);

  vtkLogStartScope(TRACE, "kdtree");
  // use diy::kdtree to shuffle points around so that all spatially co-located
  // points are within a block.
  diy::kdtree_sampling(master, assigner, 3, gdomain, BlockTraitsT::GetPoints(), /*hist_bins=*/512);
  vtkLogEndScope("kdtree");
  self->UpdateProgress(0.50);

  vtkLogStartScope(TRACE, "merge-points");
  // iterate over all local blocks to give them unique ids.
  master.foreach ([](BlockT* b,                        // local block
                    const diy::Master::ProxyWithLink&) // communication proxy
    { BlockTraitsT::DoLocalMerge(b); });

  const int nblocks = assigner.nblocks();
  // exchange unique ids count so that we can determine global id offsets
  diy::all_to_all(master, assigner, [&nblocks](BlockT* b, const diy::ReduceProxy& rp) {
    if (rp.round() == 0)
    {
      for (int dest_gid = rp.gid() + 1; dest_gid < nblocks; ++dest_gid)
      {
        rp.enqueue(rp.out_link().target(dest_gid), BlockTraitsT::GetUniqueIdsCount(b));
      }
    }
    else
    {
      vtkIdType offset = 0;
      for (int src_gid = 0; src_gid < rp.gid(); ++src_gid)
      {
        vtkIdType msg;
        rp.dequeue(src_gid, msg);
        offset += msg;
      }
      BlockTraitsT::OffsetUniqueIds(b, offset);
    }
  });
  vtkLogEndScope("merge-points");
  self->UpdateProgress(0.75);

  // now communicate global ids.
  vtkLogStartScope(TRACE, "exchange-global-ids");
  diy::all_to_all(master, assigner, [](BlockT* b, const diy::ReduceProxy& rp) {
    if (rp.round() == 0)
    {
      // now enqueue data for source blocks.
      const auto num_points = BlockTraitsT::GetNumberOfPoints(b);
      for (size_t cc = 0; cc < num_points; ++cc)
      {
        rp.enqueue(rp.out_link().target(BlockTraitsT::GetPointSourceGID(b, cc)),
          BlockTraitsT::GetIDMessage(b, cc));
      }
    }
    else
    {
      // now dequeue global id information and update local points.
      using msg_type = decltype(BlockTraitsT::GetIDMessage(nullptr, 0));
      msg_type msg;
      for (int i = 0; i < rp.in_link().size(); ++i)
      {
        const int in_gid = rp.in_link().target(i).gid;
        while (rp.incoming(in_gid))
        {
          rp.dequeue(in_gid, msg);
          BlockTraitsT::SetIDMessage(b, msg);
        }
      }
    }
  });
  vtkLogEndScope("exchange-global-ids");

  self->UpdateProgress(1.0);
  return true;
}
}

namespace
{

/**
 * This is the point type that keeps the coordinates for each point in the
 * dataset as well as enough information to track where that point came from so
 * that we can communicate back to the source once a unique global id has been
 * assigned.
 */
struct PointTT
{
  vtkTuple<double, 3> coords;
  int source_gid;
  vtkIdType source_ptid;

  // note: there's loss of precision here, but that's okay. this is only used by
  // DIY when building the kdtree
  float operator[](unsigned int index) const { return static_cast<float>(this->coords[index]); }
};

struct PointBlockT
{
public:
  vtkDataSet* Dataset{ nullptr };
  std::vector<PointTT> Points;
  std::vector<vtkIdType> Ids;
  std::vector<unsigned char> GhostPointFlags;
  vtkIdType UniquePointsCount{ 0 };

  // these are final arrays that are generated.
  vtkIdTypeArray* GlobalIds = nullptr;
  vtkUnsignedCharArray* GhostPoints = nullptr;

  void Initialize(int gid, vtkPoints* pts, vtkDataSet* dataset)
  {
    this->Dataset = dataset;
    this->Points.resize(pts->GetNumberOfPoints());
    vtkSMPTools::For(0, pts->GetNumberOfPoints(), [this, pts, gid](vtkIdType start, vtkIdType end) {
      for (vtkIdType cc = start; cc < end; ++cc)
      {
        auto& pt = this->Points[cc];
        pts->GetPoint(cc, pt.coords.GetData());
        pt.source_gid = gid;
        pt.source_ptid = cc;
      }
    });
  }

  void MergePoints()
  {
    vtkNew<vtkStaticPointLocator> locator;
    locator->SetDataSet(this->CreateDataSet());
    locator->BuildLocator();

    std::vector<vtkIdType> mergemap(this->Points.size(), -1);
    locator->MergePoints(0.0, &mergemap[0]);
    // locator's mergemap does not remove unused id i.e. if we have 3 pts
    // `0, 1, 2` and the first two are the same, then the mergemap will be `0,
    // 0, 2`. We want to make it `0, 0, 1` so that it represents contiguous ids.
    // we can also update the ghost points flags at the same time.
    this->GhostPointFlags.resize(this->Points.size(), 0);
    vtkIdType nextid=0;
    for (vtkIdType cc = 0, max = static_cast<vtkIdType>(mergemap.size()); cc < max; ++cc)
    {
      if (mergemap[cc] != cc)
      {
        mergemap[cc] = mergemap[mergemap[cc]];
        this->GhostPointFlags[cc] = vtkDataSetAttributes::DUPLICATEPOINT;
      }
      else
      {
        mergemap[cc] = nextid++;
        this->GhostPointFlags[cc] = 0;
      }
    }
    this->Ids = std::move(mergemap);
    this->UniquePointsCount = nextid;
    assert(nextid ==
        std::accumulate(this->GhostPointFlags.begin(), this->GhostPointFlags.end(), 0,
        [](vtkIdType sum, unsigned char ghost_flag) { return sum + (ghost_flag == 0 ? 1 : 0); }));
  }

  void AddOffset(const vtkIdType offset)
  {
    vtkSMPTools::For(0, static_cast<vtkIdType>(this->Ids.size()),
      [&offset, this](vtkIdType start, vtkIdType end) {
        for (vtkIdType cc = start; cc < end; ++cc)
        {
          this->Ids[cc] += offset;
        }
      });
  }

  void AllocateOutputArrays()
  {
    if (this->GlobalIds == nullptr)
    {
      this->GlobalIds = vtkIdTypeArray::New();
      this->GlobalIds->SetName("GlobalPointIds");
      this->GlobalIds->SetNumberOfTuples(this->Dataset->GetNumberOfPoints());
      this->Dataset->GetPointData()->SetGlobalIds(this->GlobalIds);
      this->GlobalIds->FastDelete();
    }
    if (this->GhostPoints == nullptr)
    {
      this->GhostPoints = vtkUnsignedCharArray::New();
      this->GhostPoints->SetName(vtkDataSetAttributes::GhostArrayName());
      this->GhostPoints->SetNumberOfTuples(this->Dataset->GetNumberOfPoints());
      this->Dataset->GetPointData()->AddArray(this->GhostPoints);
      this->GhostPoints->FastDelete();
    }
  }

  void SetGlobalId(vtkIdType pt_id, vtkIdType global_id)
  {
    this->GlobalIds->SetTypedComponent(pt_id, 0, global_id);
  }

  void SetGhostPointFlag(vtkIdType pt_id, unsigned char flag)
  {
    this->GhostPoints->SetTypedComponent(pt_id, 0, flag);
  }

private:
  vtkSmartPointer<vtkDataSet> CreateDataSet()
  {
    auto numPts = static_cast<vtkIdType>(this->Points.size());
    vtkNew<vtkUnstructuredGrid> grid;
    vtkNew<vtkPoints> pts;
    pts->SetDataTypeToDouble();
    pts->SetNumberOfPoints(numPts);
    vtkSMPTools::For(0, numPts, [&](vtkIdType start, vtkIdType end) {
      for (vtkIdType cc = start; cc < end; ++cc)
      {
        pts->SetPoint(cc, this->Points[cc].coords.GetData());
      }
    });
    grid->SetPoints(pts);
    return grid;
  }
};

struct CellTT
{
  vtkTuple<double, 3> center;
  int source_gid;
  vtkIdType source_cellid;
  std::vector<vtkIdType> point_ids;

  // note: there's loss of precision here, but that's okay. this is only used by
  // DIY when building the kdtree
  float operator[](unsigned int index) const { return static_cast<float>(this->center[index]); }
};

struct CellBlockT
{
public:
  vtkDataSet* Dataset{ nullptr };
  std::vector<CellTT> Cells;
  std::vector<vtkIdType> Ids;
  vtkIdType UniqueCellsCount;

  // these are final arrays that are generated.
  vtkIdTypeArray* GlobalIds = nullptr;

  void Initialize(int gid, vtkPoints* centers, vtkDataSet* ds)
  {
    this->Dataset = ds;

    const vtkIdType ncells = ds->GetNumberOfCells();
    assert(centers->GetNumberOfPoints() == ncells);
    this->Cells.resize(ncells);

    vtkSMPThreadLocalObject<vtkIdList> tlIdList;
    if (ncells > 0)
    {
      // so that we can call GetCellPoints in vtkSMPTools::For
      ds->GetCellPoints(0, tlIdList.Local());
    }

    auto pt_gids = vtkIdTypeArray::SafeDownCast(ds->GetPointData()->GetGlobalIds());
    assert(ncells == 0 || pt_gids != nullptr);
    vtkSMPTools::For(0, ncells, [&](vtkIdType start, vtkIdType end) {
      auto ids = tlIdList.Local();
      for (vtkIdType cc = start; cc < end; ++cc)
      {
        auto& cell = this->Cells[cc];
        centers->GetPoint(cc, cell.center.GetData());
        cell.source_gid = gid;
        cell.source_cellid = cc;

        ds->GetCellPoints(cc, ids);
        cell.point_ids.resize(ids->GetNumberOfIds());
        for (vtkIdType kk = 0, max = ids->GetNumberOfIds(); kk < max; ++kk)
        {
          cell.point_ids[kk] = pt_gids->GetTypedComponent(ids->GetId(kk), 0);
        }
      }
    });
  }

  void MergeCells()
  {
    this->UniqueCellsCount = 0;
    this->Ids.resize(this->Cells.size(), -1);

    std::sort(this->Cells.begin(), this->Cells.end(),
      [](const CellTT& lhs, const CellTT& rhs) { return lhs.point_ids < rhs.point_ids; });

    if (this->Cells.size() > 0)
    {
      this->Ids[0] = 0;
      vtkIdType nextid = 1;
      for (size_t cc = 1, max = this->Cells.size(); cc < max; ++cc)
      {
        if (this->Cells[cc - 1].point_ids == this->Cells[cc].point_ids)
        {
          this->Ids[cc] = this->Ids[cc - 1];
        }
        else
        {
          this->Ids[cc] = nextid++;
        }
      }
      this->UniqueCellsCount = nextid;
    }
  }

  void AddOffset(const vtkIdType offset)
  {
    vtkSMPTools::For(
      0, static_cast<vtkIdType>(this->Ids.size()), [&offset, this](vtkIdType start, vtkIdType end) {
        for (vtkIdType cc = start; cc < end; ++cc)
        {
          this->Ids[cc] += offset;
        }
      });
  }

  void AllocateOutputArrays()
  {
    if (this->GlobalIds == nullptr)
    {
      this->GlobalIds = vtkIdTypeArray::New();
      this->GlobalIds->SetName("GlobalCellIds");
      this->GlobalIds->SetNumberOfTuples(this->Dataset->GetNumberOfCells());
      this->Dataset->GetCellData()->SetGlobalIds(this->GlobalIds);
      this->GlobalIds->FastDelete();
    }
  }

  void SetGlobalId(vtkIdType cell_id, vtkIdType global_id)
  {
    this->GlobalIds->SetTypedComponent(cell_id, 0, global_id);
  }

private:
  vtkSmartPointer<vtkDataSet> CreateDataSet()
  {
    auto numCells = static_cast<vtkIdType>(this->Cells.size());
    vtkNew<vtkUnstructuredGrid> grid;
    vtkNew<vtkPoints> pts;
    pts->SetDataTypeToDouble();
    pts->SetNumberOfPoints(numCells);
    vtkSMPTools::For(0, numCells, [&](vtkIdType start, vtkIdType end) {
      for (vtkIdType cc = start; cc < end; ++cc)
      {
        pts->SetPoint(cc, this->Cells[cc].center.GetData());
      }
    });
    grid->SetPoints(pts);
    return grid;
  }
};
}

namespace diy
{
template<>
struct Serialization< ::CellTT>
{
  static void save(BinaryBuffer& bb, const CellTT& c)
  {
    diy::save(bb, c.center);
    diy::save(bb, c.source_gid);
    diy::save(bb, c.source_cellid);
    diy::save(bb, c.point_ids);
  }
  static void load(BinaryBuffer& bb, CellTT& c)
  {
    c.point_ids.clear();

    diy::load(bb, c.center);
    diy::load(bb, c.source_gid);
    diy::load(bb, c.source_cellid);
    diy::load(bb, c.point_ids);
  }
};
}

namespace impl
{
template<>
struct BlockTraits< ::PointBlockT>
{
  using BlockT = ::PointBlockT;
  static constexpr bool UseCellCenters = false;
  static std::vector< ::PointTT>& GetPoints(BlockT* block) { return block->Points; }
  static std::vector< ::PointTT> BlockT::*GetPoints() { return &BlockT::Points; }
  static void DoLocalMerge(BlockT* b) { b->MergePoints(); }
  static vtkIdType GetUniqueIdsCount(const BlockT* b) { return b->UniquePointsCount; }
  static void OffsetUniqueIds(BlockT* b, vtkIdType offset)
  {
    if (offset > 0)
    {
      b->AddOffset(offset);
    }
  }
  static size_t GetNumberOfPoints(const BlockT* b) { return b->Points.size(); }
  static int GetPointSourceGID(const BlockT* b, size_t index)
  {
    return b->Points[index].source_gid;
  }
  static std::tuple<vtkIdType, vtkIdType, unsigned char> GetIDMessage(const BlockT* b, size_t index)
  {
    return std::make_tuple(
      b->Points[index].source_ptid, b->Ids[index], b->GhostPointFlags[index]);
  }
  static void SetIDMessage(BlockT* b, const std::tuple<vtkIdType, vtkIdType, unsigned char>& msg)
  {
    b->AllocateOutputArrays();
    b->SetGlobalId(std::get<0>(msg), std::get<1>(msg));
    b->SetGhostPointFlag(std::get<0>(msg), std::get<2>(msg));
  }
};

template<>
struct BlockTraits< ::CellBlockT>
{
  using BlockT = ::CellBlockT;
  static constexpr bool UseCellCenters = true;
  static std::vector< ::CellTT> BlockT::*GetPoints() { return &BlockT::Cells; }
  static void DoLocalMerge(BlockT* b) { b->MergeCells(); }
  static vtkIdType GetUniqueIdsCount(const BlockT* b) { return b->UniqueCellsCount; }
  static void OffsetUniqueIds(BlockT* b, vtkIdType offset)
  {
    if (offset > 0)
    {
      b->AddOffset(offset);
    }
  }
  static size_t GetNumberOfPoints(const BlockT* b) { return b->Cells.size(); }
  static int GetPointSourceGID(const BlockT* b, size_t index) { return b->Cells[index].source_gid; }
  static std::pair<vtkIdType, vtkIdType> GetIDMessage(const BlockT* b, size_t index)
  {
    auto& cell = b->Cells[index];
    return std::make_pair(cell.source_cellid, b->Ids[index]);
  }
  static void SetIDMessage(BlockT* b, const std::pair<vtkIdType, vtkIdType>& msg)
  {
    b->AllocateOutputArrays();
    b->SetGlobalId(msg.first, msg.second);
  }
};
}

vtkStandardNewMacro(vtkGenerateGlobalIds);
vtkCxxSetObjectMacro(vtkGenerateGlobalIds, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkGenerateGlobalIds::vtkGenerateGlobalIds()
  : Controller(nullptr)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkGenerateGlobalIds::~vtkGenerateGlobalIds()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int vtkGenerateGlobalIds::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  auto outputDO = vtkDataObject::GetData(outputVector, 0);
  outputDO->ShallowCopy(inputDO);

  // generate point ids first.
  {
    this->SetProgressShiftScale(0, 0.5);
    vtkLogScopeF(TRACE, "generate global point ids");
    if (!impl::GenerateIds<PointBlockT>(outputDO, this))
    {
      this->SetProgressShiftScale(0, 1.0);
      return 0;
    }
  }

  // generate cell ids next (this needs global point ids)
  {
    this->SetProgressShiftScale(0.5, 0.5);
    vtkLogScopeF(TRACE, "generate global cell ids");
    if (!impl::GenerateIds<CellBlockT>(outputDO, this))
    {
      this->SetProgressShiftScale(0, 1.0);
      return 0;
    }
  }
  this->SetProgressShiftScale(0, 1.0);
  return 1;
}

//----------------------------------------------------------------------------
void vtkGenerateGlobalIds::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
