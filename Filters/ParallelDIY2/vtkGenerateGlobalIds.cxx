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
#include "vtkDIYExplicitAssigner.h"
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

#include <algorithm>
#include <climits>
#include <tuple>
#include <type_traits>

// clang-format off
#include "vtk_diy2.h"
// #define DIY_USE_SPDLOG
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
template <typename ElementBlockT>
static bool GenerateIds(vtkDataObject* dobj, vtkGenerateGlobalIds* self, bool cell_centers)
{
  self->UpdateProgress(0.0);
  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(self->GetController());

  vtkLogStartScope(TRACE, "extract points");
  auto datasets = vtkDIYUtilities::GetDataSets(dobj);
  datasets.erase(std::remove_if(datasets.begin(), datasets.end(),
                   [&cell_centers](vtkDataSet* ds) {
                     return ds == nullptr || ds->GetNumberOfPoints() == 0 ||
                       (cell_centers && ds->GetNumberOfCells() == 0);
                   }),
    datasets.end());
  const auto points = vtkDIYUtilities::ExtractPoints(datasets, cell_centers);
  vtkLogEndScope("extract points");

  // get the bounds for the domain globally.
  const diy::ContinuousBounds gdomain =
    vtkDIYUtilities::Convert(impl::AllReduceBounds(comm, points));

  const int local_num_blocks = static_cast<int>(points.size());
  vtkDIYExplicitAssigner assigner(comm, local_num_blocks, /*pow-of-2*/ true);

  diy::Master master(
    comm, 1, -1, []() { return static_cast<void*>(new ElementBlockT); },
    [](void* b) -> void { delete static_cast<ElementBlockT*>(b); });

  vtkLogStartScope(TRACE, "populate master");
  std::vector<int> gids;
  assigner.local_gids(comm.rank(), gids);
  for (size_t lid = 0; lid < gids.size(); ++lid)
  {
    auto block = new ElementBlockT();
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

  if (assigner.nblocks() > 1)
  {
    vtkLogStartScope(TRACE, "kdtree");
    // use diy::kdtree to shuffle points around so that all spatially co-located
    // points are within a block.
    diy::kdtree(master, assigner, 3, gdomain, &ElementBlockT::Elements, /*hist_bins=*/512);
    vtkLogEndScope("kdtree");
  }
  self->UpdateProgress(0.50);

  vtkLogStartScope(TRACE, "merge-points");
  // iterate over all local blocks to give them unique ids.
  master.foreach ([](ElementBlockT* b,                         // local block
                    const diy::Master::ProxyWithLink&) -> void // communication proxy
    { b->MergeElements(); });
  vtkLogEndScope("merge-points");
  self->UpdateProgress(0.75);

  // now communicate point ownership information and assign ids to locally owned
  // points.
  vtkLogStartScope(TRACE, "exchange-ownership-ids");
  diy::all_to_all(master, assigner, [](ElementBlockT* b, const diy::ReduceProxy& rp) {
    if (rp.round() == 0)
    {
      // now enqueue ownership information.
      b->EnqueueOwnershipInformation(rp);
    }
    else
    {
      // now dequeue owership information and process locally to assign ids
      // to locally owned points and flag ghost points.
      b->DequeueOwnershipInformation(rp);
    }
  });
  vtkLogEndScope("exchange-ownership-ids");

  // exchange unique ids count so that we can determine global id offsets
  vtkLogStartScope(TRACE, "exchange-unique-ids");
  diy::all_to_all(master, assigner, [](ElementBlockT* b, const diy::ReduceProxy& rp) {
    if (rp.round() == 0)
    {
      for (int i = rp.gid() + 1; i < rp.nblocks(); ++i)
      {
        rp.enqueue(rp.out_link().target(i), b->UniqueElementsCount);
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
      b->AddOffset(offset);
    }
  });
  vtkLogEndScope("exchange-unique-ids");

  // exchange assigned ids.
  vtkLogStartScope(TRACE, "exchange-assigned-ids");
  diy::all_to_all(master, assigner, [](ElementBlockT* b, const diy::ReduceProxy& rp) {
    if (rp.round() == 0)
    {
      b->EnqueueReplies(rp);
    }
    else
    {
      b->DequeueReplies(rp);
    }
  });
  vtkLogEndScope("exchange-assigned-ids");

  // final back communication to assign ids to ghosted points.
  vtkLogStartScope(TRACE, "exchange-ghosted-ids");
  diy::all_to_all(master, assigner, [](ElementBlockT* b, const diy::ReduceProxy& rp) {
    if (rp.round() == 0)
    {
      b->EnqueueGhostedIds(rp);
    }
    else
    {
      b->DequeueGhostedIds(rp);
    }
  });
  vtkLogEndScope("exchange-ghosted-ids");
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
  static const int attr_type = vtkDataObject::POINT;

  vtkTuple<double, 3> coords;
  int source_gid;
  vtkIdType source_id;

  // note: there's loss of precision here, but that's okay. this is only used by
  // DIY when building the kdtree
  float operator[](unsigned int index) const { return static_cast<float>(this->coords[index]); }

  static std::vector<PointTT> GetElements(int gid, vtkPoints* pts, vtkDataSet*)
  {
    std::vector<PointTT> elems;
    elems.resize(pts->GetNumberOfPoints());
    vtkSMPTools::For(
      0, pts->GetNumberOfPoints(), [&elems, pts, gid](vtkIdType start, vtkIdType end) {
        for (vtkIdType cc = start; cc < end; ++cc)
        {
          auto& pt = elems[cc];
          pts->GetPoint(cc, pt.coords.GetData());
          pt.source_gid = gid;
          pt.source_id = cc;
        }
      });
    return elems;
  }

  static void Sort(std::vector<PointTT>& points)
  {
    // let's sort the points by source-id. This ensures that when a point is
    // duplicated among multiple blocks, the block with lower block-id owns the
    // point. Thus, keeping the numbering consistent.
    std::sort(points.begin(), points.end(), [](const PointTT& a, const PointTT& b) {
      return (a.source_gid == b.source_gid) ? (a.source_id < b.source_id)
                                            : (a.source_gid < b.source_gid);
    });
  }

  static std::vector<vtkIdType> GenerateMergeMap(const std::vector<PointTT>& points)
  {
    std::vector<vtkIdType> mergemap(points.size(), -1);
    if (points.size() == 0)
    {
      return mergemap;
    }

    auto numPts = static_cast<vtkIdType>(points.size());

    vtkNew<vtkUnstructuredGrid> grid;
    vtkNew<vtkPoints> pts;
    pts->SetDataTypeToDouble();
    pts->SetNumberOfPoints(numPts);
    vtkSMPTools::For(0, numPts, [&](vtkIdType start, vtkIdType end) {
      for (vtkIdType cc = start; cc < end; ++cc)
      {
        pts->SetPoint(cc, points[cc].coords.GetData());
      }
    });
    grid->SetPoints(pts);

    vtkNew<vtkStaticPointLocator> locator;
    locator->SetDataSet(grid);
    locator->BuildLocator();
    locator->MergePoints(0.0, &mergemap[0]);
    return mergemap;
  }
};

struct CellTT
{
  static const int attr_type = vtkDataObject::CELL;
  vtkTuple<double, 3> center;
  int source_gid;
  vtkIdType source_id;
  std::vector<vtkIdType> point_ids;

  // note: there's loss of precision here, but that's okay. this is only used by
  // DIY when building the kdtree
  float operator[](unsigned int index) const { return static_cast<float>(this->center[index]); }

  static std::vector<CellTT> GetElements(int gid, vtkPoints* centers, vtkDataSet* ds)
  {
    const vtkIdType ncells = ds->GetNumberOfCells();
    assert(centers->GetNumberOfPoints() == ncells);

    std::vector<CellTT> elems(ncells);

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
        auto& cell = elems[cc];
        centers->GetPoint(cc, cell.center.GetData());
        cell.source_gid = gid;
        cell.source_id = cc;

        ds->GetCellPoints(cc, ids);
        cell.point_ids.resize(ids->GetNumberOfIds());
        for (vtkIdType kk = 0, max = ids->GetNumberOfIds(); kk < max; ++kk)
        {
          cell.point_ids[kk] = pt_gids->GetTypedComponent(ids->GetId(kk), 0);
        }
      }
    });

    return elems;
  }

  static void Sort(std::vector<CellTT>& cells)
  {
    // here, we are sorting such that for duplicated cells, we always order the
    // cell on the lower block before the one on the higher block. This is
    // essential to keep the cell numbering consistent.
    std::sort(cells.begin(), cells.end(), [](const CellTT& lhs, const CellTT& rhs) {
      return (lhs.point_ids == rhs.point_ids ? lhs.source_gid < rhs.source_gid
                                             : lhs.point_ids < rhs.point_ids);
    });
  }

  static std::vector<vtkIdType> GenerateMergeMap(const std::vector<CellTT>& cells)
  {
    std::vector<vtkIdType> mergemap(cells.size(), -1);
    if (cells.size() == 0)
    {
      return mergemap;
    }

    mergemap[0] = 0;
    for (size_t cc = 1, max = cells.size(); cc < max; ++cc)
    {
      if (cells[cc - 1].point_ids == cells[cc].point_ids)
      {
        mergemap[cc] = mergemap[cc - 1];
      }
      else
      {
        mergemap[cc] = static_cast<vtkIdType>(cc);
      }
    }
    return mergemap;
  }
};

struct MessageItemTT
{
  vtkIdType elem_id;
  vtkIdType index;
};

template <typename ElementT>
struct BlockT
{
private:
  void Enqueue(const diy::ReduceProxy& rp)
  {
    for (const auto& pair : this->OutMessage)
    {
      rp.enqueue(rp.out_link().target(pair.first), pair.second);
    }
    this->OutMessage.clear();
  }

public:
  vtkDataSet* Dataset{ nullptr };
  std::vector<ElementT> Elements;
  std::vector<vtkIdType> MergeMap;
  vtkIdType UniqueElementsCount{ 0 };
  std::map<int, std::vector<MessageItemTT> > OutMessage;

  vtkSmartPointer<vtkIdTypeArray> GlobalIds;
  vtkSmartPointer<vtkUnsignedCharArray> GhostArray;

  void Initialize(int self_gid, vtkPoints* points, vtkDataSet* dataset)
  {
    this->Dataset = dataset;
    this->Elements = ElementT::GetElements(self_gid, points, dataset);

    if (dataset)
    {
      this->GlobalIds = vtkSmartPointer<vtkIdTypeArray>::New();
      this->GlobalIds->SetName(
        ElementT::attr_type == vtkDataObject::POINT ? "GlobalPointIds" : "GlobalCellIds");
      this->GlobalIds->SetNumberOfTuples(points->GetNumberOfPoints());
      this->GlobalIds->FillValue(-1);
      dataset->GetAttributes(ElementT::attr_type)->SetGlobalIds(this->GlobalIds);

      this->GhostArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
      this->GhostArray->SetName(vtkDataSetAttributes::GhostArrayName());
      this->GhostArray->SetNumberOfTuples(points->GetNumberOfPoints());
      this->GhostArray->FillValue(vtkDataSetAttributes::DUPLICATEPOINT);

      // we're only adding ghost points, not cells.
      if (ElementT::attr_type == vtkDataObject::POINT)
      {
        dataset->GetAttributes(ElementT::attr_type)->AddArray(this->GhostArray);
      }
    }
  }

  void MergeElements()
  {
    // sort to make elements on lower gid's the primary elements
    ElementT::Sort(this->Elements);
    this->MergeMap = ElementT::GenerateMergeMap(this->Elements);

    std::vector<char> needs_replies(this->MergeMap.size());
    for (size_t cc = 0, max = this->MergeMap.size(); cc < max; ++cc)
    {
      if (this->MergeMap[cc] != static_cast<vtkIdType>(cc))
      {
        needs_replies[this->MergeMap[cc]] = 1;
      }
    }

    // populate out-message.
    for (size_t cc = 0, max = this->MergeMap.size(); cc < max; ++cc)
    {
      if (this->MergeMap[cc] == static_cast<vtkIdType>(cc))
      {
        const auto& elem = this->Elements[cc];

        MessageItemTT datum;
        datum.elem_id = elem.source_id;
        datum.index = needs_replies[cc] == 1 ? static_cast<vtkIdType>(cc) : vtkIdType(-1);
        this->OutMessage[elem.source_gid].push_back(datum);
      }
    }
  }

  void EnqueueOwnershipInformation(const diy::ReduceProxy& rp) { this->Enqueue(rp); }
  void DequeueOwnershipInformation(const diy::ReduceProxy& rp)
  {
    std::map<int, std::vector<MessageItemTT> > inmessage;
    for (int i = 0; i < rp.in_link().size(); ++i)
    {
      const int in_gid = rp.in_link().target(i).gid;
      while (rp.incoming(in_gid))
      {
        auto& ownerships = inmessage[in_gid];
        rp.dequeue(in_gid, ownerships);
      }
    }

    // we should not have received any message if we don't have a dataset.
    assert(this->Dataset != nullptr || inmessage.size() == 0);
    if (!this->Dataset)
    {
      return;
    }

    for (const auto& pair : inmessage)
    {
      for (const auto& data : pair.second)
      {
        this->GhostArray->SetTypedComponent(data.elem_id, 0, 0);
      }
    }

    // Assign global ids starting with 0 for locally owned elems.
    this->UniqueElementsCount = 0;
    for (vtkIdType cc = 0, max = this->GhostArray->GetNumberOfTuples(); cc < max; ++cc)
    {
      if (this->GhostArray->GetTypedComponent(cc, 0) == 0)
      {
        this->GlobalIds->SetTypedComponent(cc, 0, this->UniqueElementsCount);
        this->UniqueElementsCount++;
      }
    }

    // Generate message send back assigned global ids to requesting blocks.
    for (const auto& pair : inmessage)
    {
      for (const auto& data : pair.second)
      {
        if (data.index != -1)
        {
          MessageItemTT reply;
          reply.index = data.index;
          reply.elem_id = this->GlobalIds->GetTypedComponent(data.elem_id, 0);
          this->OutMessage[pair.first].push_back(reply);
        }
      }
    }
  }

  void AddOffset(const vtkIdType offset)
  {
    if (this->GlobalIds == nullptr || offset == 0)
    {
      return;
    }
    vtkSMPTools::For(
      0, this->GlobalIds->GetNumberOfTuples(), [&offset, this](vtkIdType start, vtkIdType end) {
        for (vtkIdType cc = start; cc < end; ++cc)
        {
          const auto id = this->GlobalIds->GetTypedComponent(cc, 0);
          if (id != -1)
          {
            this->GlobalIds->SetTypedComponent(cc, 0, id + offset);
          }
        }
      });

    // offset replies too.
    for (auto& pair : this->OutMessage)
    {
      for (auto& data : pair.second)
      {
        data.elem_id += offset;
      }
    }
  }

  void EnqueueReplies(const diy::ReduceProxy& rp) { this->Enqueue(rp); }
  void DequeueReplies(const diy::ReduceProxy& rp)
  {
    for (int i = 0; i < rp.in_link().size(); ++i)
    {
      const int in_gid = rp.in_link().target(i).gid;
      while (rp.incoming(in_gid))
      {
        std::vector<MessageItemTT> ownerships;
        rp.dequeue(in_gid, ownerships);
        for (const auto& data : ownerships)
        {
          // we're changing the id in our local storage to now be the global
          // id.
          this->Elements[data.index].source_id = data.elem_id;
        }
      }
    }

    const auto& mergemap = this->MergeMap;
    for (size_t cc = 0, max = mergemap.size(); cc < max; ++cc)
    {
      if (mergemap[cc] != static_cast<vtkIdType>(cc))
      {
        auto& original_elem = this->Elements[mergemap[cc]];
        auto& duplicate_elem = this->Elements[cc];

        MessageItemTT data;
        data.elem_id = original_elem.source_id;
        data.index = duplicate_elem.source_id;
        this->OutMessage[duplicate_elem.source_gid].push_back(data);
      }
    }
  }

  void EnqueueGhostedIds(const diy::ReduceProxy& rp) { this->Enqueue(rp); }
  void DequeueGhostedIds(const diy::ReduceProxy& rp)
  {
    for (int i = 0; i < rp.in_link().size(); ++i)
    {
      const int in_gid = rp.in_link().target(i).gid;
      while (rp.incoming(in_gid))
      {
        std::vector<MessageItemTT> ownerships;
        rp.dequeue(in_gid, ownerships);
        assert(this->Dataset != nullptr || ownerships.size() == 0);
        for (const auto& data : ownerships)
        {
          this->GlobalIds->SetTypedComponent(data.index, 0, data.elem_id);
        }
      }
    }
  }
};
}

namespace diy
{
template <>
struct Serialization< ::CellTT>
{
  static void save(BinaryBuffer& bb, const CellTT& c)
  {
    diy::save(bb, c.center);
    diy::save(bb, c.source_gid);
    diy::save(bb, c.source_id);
    diy::save(bb, c.point_ids);
  }
  static void load(BinaryBuffer& bb, CellTT& c)
  {
    c.point_ids.clear();

    diy::load(bb, c.center);
    diy::load(bb, c.source_gid);
    diy::load(bb, c.source_id);
    diy::load(bb, c.point_ids);
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
    if (!impl::GenerateIds<BlockT<PointTT> >(outputDO, this, false))
    {
      this->SetProgressShiftScale(0, 1.0);
      return 0;
    }
  }

  // generate cell ids next (this needs global point ids)
  {
    this->SetProgressShiftScale(0.5, 0.5);
    vtkLogScopeF(TRACE, "generate global cell ids");
    if (!impl::GenerateIds<BlockT<CellTT> >(outputDO, this, true))
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
