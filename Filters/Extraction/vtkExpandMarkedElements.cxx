// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkExpandMarkedElements.h"

#include "vtkAbstractPointLocator.h"
#include "vtkBoundingBox.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDIYExplicitAssigner.h"
#include "vtkDIYUtilities.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkSignedCharArray.h"

// clang-format off
#include "vtk_diy2.h"
// #define DIY_USE_SPDLOG
#include VTK_DIY2(diy/mpi.hpp)
#include VTK_DIY2(diy/master.hpp)
#include VTK_DIY2(diy/link.hpp)
#include VTK_DIY2(diy/reduce.hpp)
#include VTK_DIY2(diy/reduce-operations.hpp)
#include VTK_DIY2(diy/partners/swap.hpp)
#include VTK_DIY2(diy/algorithms.hpp)
// clang-format on

#include <algorithm>
#include <set>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
void ShallowCopy(vtkDataObject* input, vtkDataObject* output)
{
  auto inCD = vtkCompositeDataSet::SafeDownCast(input);
  auto outCD = vtkCompositeDataSet::SafeDownCast(output);
  if (inCD && outCD)
  {
    outCD->CopyStructure(inCD);
    auto iter = inCD->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      auto clone = iter->GetCurrentDataObject()->NewInstance();
      clone->ShallowCopy(iter->GetCurrentDataObject());
      outCD->SetDataSet(iter, clone);
      clone->FastDelete();
    }
    iter->Delete();
  }
  else
  {
    output->ShallowCopy(input);
  }
}

struct BlockT
{
  vtkDataSet* Dataset = nullptr;
  vtkAbstractPointLocator* Locator = nullptr;
  vtkNew<vtkSignedCharArray> SeedMarkedArray;
  vtkNew<vtkSignedCharArray> MarkedArray;
  vtkNew<vtkIntArray> UpdateFlags;
  std::vector<std::pair<diy::BlockID, vtkBoundingBox>> Neighbors;

  void BuildLocator();
  void EnqueueAndExpand(int assoc, int round, const diy::Master::ProxyWithLink& cp);
  void DequeueAndExpand(int assoc, int round, const diy::Master::ProxyWithLink& cp);
  void RemoveExcedentLayers(bool removeSeed, bool removeIntermediateLayers, int finalRound);

private:
  void Expand(int assoc, int round, const std::set<vtkIdType>& ptids);
  vtkNew<vtkIdList> CellIds;
  vtkNew<vtkIdList> PtIds;
};

void BlockT::BuildLocator()
{
  if (auto pointSet = vtkPointSet::SafeDownCast(this->Dataset))
  {
    // build internal cell locator to avoid rebuilding it
    pointSet->BuildPointLocator();
    this->Locator = pointSet->GetPointLocator();
  }
}

void BlockT::EnqueueAndExpand(int assoc, int round, const diy::Master::ProxyWithLink& cp)
{
  const int rminusone = (round - 1);
  std::set<vtkIdType> chosen_ptids;
  if (assoc == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    vtkIdType numCellPts, cc;
    const vtkIdType* cellPts;
    for (vtkIdType cellid = 0, max = this->Dataset->GetNumberOfCells(); cellid < max; ++cellid)
    {
      if (this->MarkedArray->GetTypedComponent(cellid, 0) &&
        (this->UpdateFlags->GetTypedComponent(cellid, 0) == rminusone))
      {
        this->Dataset->GetCellPoints(cellid, numCellPts, cellPts, this->PtIds);
        for (cc = 0; cc < numCellPts; ++cc)
        {
          chosen_ptids.insert(cellPts[cc]);
        }
      }
    }
  }
  else
  {
    for (vtkIdType ptid = 0, max = this->Dataset->GetNumberOfPoints(); ptid < max; ++ptid)
    {
      if (this->MarkedArray->GetTypedComponent(ptid, 0) &&
        (this->UpdateFlags->GetTypedComponent(ptid, 0) == rminusone))
      {
        chosen_ptids.insert(ptid);
      }
    }
  }

  double pt[3];
  for (const vtkIdType& ptid : chosen_ptids)
  {
    this->Dataset->GetPoint(ptid, pt);
    for (const auto& pair : this->Neighbors)
    {
      if (pair.second.ContainsPoint(pt))
      {
        cp.enqueue(pair.first, pt, 3);
      }
    }
  }
  this->Expand(assoc, round, chosen_ptids);
}

void BlockT::DequeueAndExpand(int assoc, int round, const diy::Master::ProxyWithLink& cp)
{
  std::vector<int> incoming;
  cp.incoming(incoming);

  std::set<vtkIdType> pointIds;

  double pt[3];
  for (const int& gid : incoming)
  {
    while (cp.incoming(gid))
    {
      cp.dequeue(gid, pt, 3);
      double d;
      auto ptid = this->Locator ? this->Locator->FindClosestPointWithinRadius(0.000000000001, pt, d)
                                : this->Dataset->FindPoint(pt);
      if (ptid != -1)
      {
        pointIds.insert(ptid);
      }
    }
  }

  this->Expand(assoc, round, pointIds);
}

void BlockT::Expand(int assoc, int round, const std::set<vtkIdType>& ptids)
{
  if (assoc == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    for (const auto& startptid : ptids)
    {
      this->Dataset->GetPointCells(startptid, this->CellIds);
      for (const auto& cellid : *this->CellIds)
      {
        if (this->MarkedArray->GetTypedComponent(cellid, 0) == 0)
        {
          this->MarkedArray->SetTypedComponent(cellid, 0, 1);
          this->UpdateFlags->SetTypedComponent(cellid, 0, round);
        }
      }
    }
  }
  else
  {
    for (const auto& startptid : ptids)
    {
      if (this->MarkedArray->GetTypedComponent(startptid, 0) == 0)
      {
        this->MarkedArray->SetTypedComponent(startptid, 0, 1);
        this->UpdateFlags->SetTypedComponent(startptid, 0, round);
      }

      // get adjacent cells for the startptid.
      this->Dataset->GetPointCells(startptid, this->CellIds);
      vtkIdType numCellPts, cc;
      const vtkIdType* cellPts;
      for (const auto& cellid : *this->CellIds)
      {
        this->Dataset->GetCellPoints(cellid, numCellPts, cellPts, this->PtIds);
        for (cc = 0; cc < numCellPts; ++cc)
        {
          if (this->MarkedArray->GetTypedComponent(cellPts[cc], 0) == 0)
          {
            this->MarkedArray->SetTypedComponent(cellPts[cc], 0, 1);
            this->UpdateFlags->SetTypedComponent(cellPts[cc], 0, round);
          }
        }
      }
    }
  }
}

void BlockT::RemoveExcedentLayers(bool removeSeed, bool removeIntermediateLayers, int finalRound)
{
  for (vtkIdType i = 0; i < this->MarkedArray->GetNumberOfValues(); i++)
  {
    bool remove = false;
    if (removeSeed)
    {
      if (this->SeedMarkedArray->GetTypedComponent(i, 0) != 0)
      {
        remove = true;
      }
    }
    if (!remove && removeIntermediateLayers)
    {
      int updateRound = this->UpdateFlags->GetTypedComponent(i, 0);
      if (updateRound != finalRound && updateRound != -1)
      {
        remove = true;
      }
    }
    if (remove)
    {
      this->MarkedArray->SetTypedComponent(i, 0, 0);
    }
  }
}
}

vtkStandardNewMacro(vtkExpandMarkedElements);
vtkCxxSetObjectMacro(vtkExpandMarkedElements, Controller, vtkMultiProcessController);
//------------------------------------------------------------------------------
vtkExpandMarkedElements::vtkExpandMarkedElements()
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
vtkExpandMarkedElements::~vtkExpandMarkedElements()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
int vtkExpandMarkedElements::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto outputDO = vtkDataObject::GetData(outputVector, 0);
  ::ShallowCopy(vtkDataObject::GetData(inputVector[0], 0), outputDO);

  vtkInformation* info = this->GetInputArrayInformation(0);
  const int assoc = info->Get(vtkDataObject::FIELD_ASSOCIATION());
  auto datasets = vtkCompositeDataSet::GetDataSets(outputDO);
  datasets.erase(std::remove_if(datasets.begin(), datasets.end(),
                   [](vtkDataSet* ds) { return (ds->GetNumberOfPoints() == 0); }),
    datasets.end());

  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(this->GetController());
  const int local_num_blocks = static_cast<int>(datasets.size());

  vtkDIYExplicitAssigner assigner(comm, local_num_blocks);

  diy::Master master(
    comm, 1, -1, []() { return static_cast<void*>(new BlockT); },
    [](void* b) { delete static_cast<BlockT*>(b); });

  vtkLogStartScope(TRACE, "populate master");
  std::vector<int> gids;
  assigner.local_gids(comm.rank(), gids);
  assert(gids.size() == datasets.size());

  std::string arrayname;
  for (size_t lid = 0; lid < gids.size(); ++lid)
  {
    auto block = new BlockT();
    block->Dataset = datasets[lid];
    assert(block->Dataset != nullptr);
    const auto numElems = block->Dataset->GetNumberOfElements(assoc);
    if (auto array =
          vtkSignedCharArray::SafeDownCast(this->GetInputArrayToProcess(0, datasets[lid])))
    {
      // deep copy so we can modify it.
      block->MarkedArray->DeepCopy(array);
      if (arrayname.empty() && array->GetName())
      {
        arrayname = array->GetName();
      }
    }
    else
    {
      block->MarkedArray->SetNumberOfTuples(numElems);
      block->MarkedArray->FillValue(0);
    }
    block->SeedMarkedArray->DeepCopy(block->MarkedArray);
    assert(block->MarkedArray->GetNumberOfTuples() == numElems);
    block->UpdateFlags->SetNumberOfTuples(numElems);
    block->UpdateFlags->FillValue(-1);
    block->BuildLocator();

    master.add(gids[lid], block, new diy::Link);
  }
  vtkLogEndScope("populate master");

  // exchange bounding boxes to determine neighbours; helps avoid all_to_all
  // communication.
  vtkLogStartScope(TRACE, "populate block neighbours");
  diy::all_to_all(master, assigner, [](BlockT* b, const diy::ReduceProxy& rp) {
    double bds[6];
    b->Dataset->GetBounds(bds);
    const vtkBoundingBox bbox(bds);
    if (rp.round() == 0)
    {
      for (int i = 0; i < rp.out_link().size(); ++i)
      {
        const auto dest = rp.out_link().target(i);
        rp.enqueue(dest, bds, 6);
      }
    }
    else
    {
      for (int i = 0; i < rp.in_link().size(); ++i)
      {
        const auto src = rp.in_link().target(i);
        double in_bds[6];
        rp.dequeue(src, in_bds, 6);
        vtkBoundingBox in_bbx(in_bds);
        if (src.gid != rp.gid() && in_bbx.IsValid() && in_bbx.Intersects(bbox))
        {
          vtkLogF(TRACE, "%d --> %d", rp.gid(), src.gid);
          b->Neighbors.emplace_back(src, in_bbx);
        }
      }
    }
  });

  // update local links.
  for (int cc = 0; cc < static_cast<int>(gids.size()); ++cc)
  {
    auto b = master.block<BlockT>(cc);
    if (!b->Neighbors.empty())
    {
      auto l = new diy::Link();
      for (const auto& npair : b->Neighbors)
      {
        l->add_neighbor(npair.first);
      }
      master.replace_link(cc, l);
    }
  }
  vtkLogEndScope("populate block neighbours");

  // Expand the selection
  for (int round = 0; round < this->NumberOfLayers; ++round)
  {
    master.foreach ([&assoc, &round](BlockT* b, const diy::Master::ProxyWithLink& cp) {
      b->EnqueueAndExpand(assoc, round, cp);
    });
    master.exchange();
    master.foreach ([&assoc, &round](BlockT* b, const diy::Master::ProxyWithLink& cp) {
      b->DequeueAndExpand(assoc, round, cp);
    });
  }

  // Remove unwanted layers
  master.foreach ([this](BlockT* b, const diy::Master::ProxyWithLink&) {
    b->RemoveExcedentLayers(
      this->RemoveSeed, this->RemoveIntermediateLayers, this->NumberOfLayers - 1);
  });

  if (arrayname.empty())
  {
    arrayname = "MarkedElements";
  }
  master.foreach ([&assoc, &arrayname](BlockT* b, const diy::Master::ProxyWithLink&) {
    b->MarkedArray->SetName(arrayname.c_str());
    b->Dataset->GetAttributes(assoc)->AddArray(b->MarkedArray);
  });

  comm.barrier();
  this->CheckAbort();
  return 1;
}

//------------------------------------------------------------------------------
void vtkExpandMarkedElements::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "NumberOfLayers: " << this->NumberOfLayers << endl;
}
VTK_ABI_NAMESPACE_END
