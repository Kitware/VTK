// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkNativePartitioningStrategy.h"

#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkDIYKdTreeUtilities.h"
#include "vtkDIYUtilities.h"
#include "vtkDataSetAttributes.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkKdNode.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPartitioningStrategy.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkUnsignedCharArray.h"

namespace
{
constexpr double BOUNDING_BOX_LENGTH_TOLERANCE = 0.01;
constexpr double BOUNDING_BOX_INFLATION_RATIO = 0.01;

vtkBoundingBox GetGlobalBounds(vtkDataObject* dobj, diy::mpi::communicator& comm)
{
  auto lbounds = vtkDIYUtilities::GetLocalBounds(dobj);
  vtkDIYUtilities::AllReduce(comm, lbounds);
  return lbounds;
}

struct PartitionDistributionWorklet
{
  vtkPartitioningStrategy::PartitionInformation* Res;
  vtkDataSet* DS;
  const int MaxCellSize;
  const std::vector<vtkBoundingBox>* Cuts;
  const std::vector<std::vector<int>>* Regions;
  struct LocalDataT
  {
    vtkSmartPointer<vtkGenericCell> GenCell;
    std::vector<vtkIdType> BoundaryNeighborParts;
  };
  vtkSMPThreadLocal<LocalDataT> LocalData;

  PartitionDistributionWorklet(vtkPartitioningStrategy::PartitionInformation* res,
    vtkDataSet* dataset, const std::vector<vtkBoundingBox>* cuts,
    const std::vector<std::vector<int>>* regions)
    : Res(res)
    , DS(dataset)
    , MaxCellSize(dataset->GetMaxCellSize())
    , Cuts(cuts)
    , Regions(regions)
  {
    this->Res->TargetEntity = vtkPartitioningStrategy::CELLS;
    this->Res->NumberOfPartitions = this->Cuts->size();
    this->Res->TargetPartitions->SetNumberOfComponents(1);
    this->Res->TargetPartitions->SetNumberOfTuples(this->DS->GetNumberOfCells());
    this->Res->TargetPartitions->Fill(-1);
  }

  void Initialize() { this->LocalData.Local().GenCell = vtkSmartPointer<vtkGenericCell>::New(); }

  void operator()(vtkIdType first, vtkIdType last)
  {
    vtkGenericCell* gcell = this->LocalData.Local().GenCell;
    std::vector<double> weights(static_cast<size_t>(this->MaxCellSize));
    for (vtkIdType cellId = first; cellId < last; ++cellId)
    {
      auto itC = this->Regions->begin() + cellId;
      if (itC->empty())
      {
        this->Res->TargetPartitions->SetValue(cellId, -1);
        continue;
      }
      if (itC->size() > 1)
      {
        this->DS->GetCell(cellId, gcell);
        double pcenter[3], center[3];
        int subId = gcell->GetParametricCenter(pcenter);
        gcell->EvaluateLocation(subId, pcenter, center, weights.data());
        for (int cutId = 0; cutId < static_cast<int>(itC->size()); ++cutId)
        {
          const auto& bbox = this->Cuts->at(itC->at(cutId));
          if (bbox.ContainsPoint(center))
          {
            this->Res->TargetPartitions->SetValue(cellId, itC->at(cutId));
          }
          else
          {
            this->LocalData.Local().BoundaryNeighborParts.emplace_back(cellId);
            this->LocalData.Local().BoundaryNeighborParts.emplace_back(itC->at(cutId));
          }
        }
        continue;
      }
      this->Res->TargetPartitions->SetValue(cellId, itC->at(0));
    }
  }

  void Reduce()
  {
    vtkIdType totSize = 0;
    std::for_each(this->LocalData.begin(), this->LocalData.end(),
      [&totSize](LocalDataT& ld) { totSize += ld.BoundaryNeighborParts.size(); });
    totSize /= 2;
    this->Res->BoundaryNeighborPartitions->SetNumberOfComponents(2);
    this->Res->BoundaryNeighborPartitions->SetNumberOfTuples(totSize);
    vtkIdType cellId = 0;
    for (auto itLD = this->LocalData.begin(); itLD != this->LocalData.end(); ++itLD)
    {
      for (auto itBNP = itLD->BoundaryNeighborParts.begin();
           itBNP != itLD->BoundaryNeighborParts.end(); itBNP += 2, ++cellId)
      {
        vtkIdType tup[2] = { *itBNP, *(itBNP + 1) };
        this->Res->BoundaryNeighborPartitions->SetTypedTuple(cellId, tup);
      }
    }
  }
};

/*
 * Fill the partition information from the cuts information
 */
vtkPartitioningStrategy::PartitionInformation CutsToPartition(
  vtkDataSet* dataset, const std::vector<vtkBoundingBox>& cuts)
{
  if (!dataset || cuts.empty() || dataset->GetNumberOfCells() == 0)
  {
    vtkWarningWithObjectMacro(nullptr, "Either dataset or cuts are empty");
    return vtkPartitioningStrategy::PartitionInformation();
  }

  auto ghostCells = vtkUnsignedCharArray::SafeDownCast(
    dataset->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName()));

  const auto numCells = dataset->GetNumberOfCells();
  std::vector<std::vector<int>> cellRegions(numCells, std::vector<int>());

  // call GetCell/GetCellBounds once to make it thread safe (see vtkDataSet::GetCell).
  vtkNew<vtkGenericCell> dummyCell;
  dataset->GetCell(0, dummyCell);
  double bds[6];
  dataset->GetCellBounds(0, bds);

  // vtkKdNode helps us do fast cell/cut intersections. So convert each cut to a
  // vtkKdNode.
  std::vector<vtkSmartPointer<vtkKdNode>> kdnodes;
  for (const auto& bbox : cuts)
  {
    auto kdnode = vtkSmartPointer<vtkKdNode>::New();
    kdnode->SetDim(-1); // leaf.

    double cut_bounds[6];
    bbox.GetBounds(cut_bounds);
    kdnode->SetBounds(cut_bounds);
    kdnodes.emplace_back(std::move(kdnode));
  }
  vtkSMPThreadLocalObject<vtkGenericCell> gcellLO;
  vtkSMPThreadLocal<std::vector<double>> weightsLO;
  const int maxCellSize = dataset->GetMaxCellSize();
  vtkSMPTools::For(0, numCells,
    [&](vtkIdType first, vtkIdType last)
    {
      auto gcell = gcellLO.Local();
      auto weights = weightsLO.Local();
      weights.resize(static_cast<size_t>(maxCellSize));
      for (vtkIdType cellId = first; cellId < last; ++cellId)
      {
        if (ghostCells != nullptr &&
          ((ghostCells->GetTypedComponent(cellId, 0) & vtkDataSetAttributes::DUPLICATECELL) != 0))
        {
          // skip ghost cells, they will not be extracted since they will be
          // extracted on ranks where they are not marked as ghosts.
          continue;
        }
        dataset->GetCell(cellId, gcell);
        double cellBounds[6];
        dataset->GetCellBounds(cellId, cellBounds);
        for (int cutId = 0; cutId < static_cast<int>(kdnodes.size()); ++cutId)
        {
          if (kdnodes[cutId]->IntersectsCell(
                gcell, /*useDataBounds*/ 0, /*cellRegion*/ -1, cellBounds))
          {
            cellRegions[cellId].emplace_back(cutId);
          }
        }
      }
    });

  vtkPartitioningStrategy::PartitionInformation res;
  ::PartitionDistributionWorklet worker(&res, dataset, &cuts, &cellRegions);
  vtkSMPTools::For(0, numCells, worker);
  return res;
}
}

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkNativePartitioningStrategy);

//------------------------------------------------------------------------------
void vtkNativePartitioningStrategy::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent.GetNextIndent() << "UseExplicitCuts: " << (this->UseExplicitCuts ? "True" : "False")
     << std::endl;
  if (this->UseExplicitCuts)
  {
    os << indent.GetNextIndent() << "Number Of Explicit Cuts: " << this->ExplicitCuts.size()
       << std::endl;
    os << indent.GetNextIndent()
       << "Expand Explicit Cuts: " << (this->ExpandExplicitCuts ? "True" : "False") << std::endl;
  }
  else
  {
    os << indent.GetNextIndent() << "Number Of Cuts: " << this->Cuts.size() << std::endl;
  }
}

//------------------------------------------------------------------------------
void vtkNativePartitioningStrategy::SetExplicitCuts(const std::vector<vtkBoundingBox>& boxes)
{
  if (this->ExplicitCuts != boxes)
  {
    this->ExplicitCuts = boxes;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkNativePartitioningStrategy::RemoveAllExplicitCuts()
{
  if (!this->ExplicitCuts.empty())
  {
    this->ExplicitCuts.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkNativePartitioningStrategy::AddExplicitCut(const vtkBoundingBox& bbox)
{
  if (bbox.IsValid() &&
    std::find(this->ExplicitCuts.begin(), this->ExplicitCuts.end(), bbox) ==
      this->ExplicitCuts.end())
  {
    this->ExplicitCuts.emplace_back(bbox);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkNativePartitioningStrategy::AddExplicitCut(const double bounds[6])
{
  vtkBoundingBox bbox(bounds);
  this->AddExplicitCut(bbox);
}

//------------------------------------------------------------------------------
int vtkNativePartitioningStrategy::GetNumberOfExplicitCuts() const
{
  return static_cast<int>(this->ExplicitCuts.size());
}

//------------------------------------------------------------------------------
const vtkBoundingBox& vtkNativePartitioningStrategy::GetExplicitCut(int index) const
{
  if (index >= 0 && index < this->GetNumberOfExplicitCuts())
  {
    return this->ExplicitCuts[index];
  }

  static vtkBoundingBox nullbox;
  return nullbox;
}

//------------------------------------------------------------------------------
std::vector<vtkPartitioningStrategy::PartitionInformation>
vtkNativePartitioningStrategy::ComputePartition(vtkPartitionedDataSetCollection* collection)
{
  std::vector<PartitionInformation> res;
  if (!collection)
  {
    vtkErrorMacro("Collection is nullptr!");
    return res;
  }

  if (this->LoadBalanceAcrossAllBlocks)
  {
    // since we're load balancing across all blocks, build cuts using the whole
    // input dataset.
    this->InitializeCuts(collection);
  }

  for (unsigned int part = 0, max = collection->GetNumberOfPartitionedDataSets(); part < max;
       ++part)
  {
    auto inputPTD = collection->GetPartitionedDataSet(part);
    if (!inputPTD)
    {
      vtkWarningMacro("Found nullptr partitioned data set");
      continue;
    }

    if (!this->LoadBalanceAcrossAllBlocks)
    {
      // when not load balancing globally, initialize cuts per partitioned
      // dataset.
      this->InitializeCuts(inputPTD);
    }

    for (unsigned int cc = 0; cc < inputPTD->GetNumberOfPartitions(); ++cc)
    {
      auto ds = inputPTD->GetPartition(cc);
      if (ds && (ds->GetNumberOfPoints() > 0 || ds->GetNumberOfCells() > 0))
      {
        res.emplace_back(::CutsToPartition(ds, this->Cuts));
      }
      else
      {
        res.emplace_back();
      }
    }
    auto controller = this->GetController();
    if (controller && controller->GetNumberOfProcesses() > 1)
    {
      vtkIdType locsize = static_cast<vtkIdType>(res.size());
      vtkIdType allsize = 0;
      controller->AllReduce(&locsize, &allsize, 1, vtkCommunicator::MAX_OP);
      res.resize(allsize);
    }
  }

  auto controller = this->GetController();
  if (controller && controller->GetNumberOfProcesses() > 1)
  {
    vtkIdType allsize = res.size();
    std::vector<vtkIdType> nParts(allsize);
    std::transform(res.begin(), res.end(), nParts.begin(),
      [](PartitionInformation& info) { return info.NumberOfPartitions; });
    std::vector<vtkIdType> globNParts(allsize);
    controller->AllReduce(nParts.data(), globNParts.data(), allsize, vtkCommunicator::MAX_OP);
    for (vtkIdType iP = 0; iP < allsize; ++iP)
    {
      res[iP].NumberOfPartitions = globNParts[iP];
    }
  }

  return res;
}

//------------------------------------------------------------------------------
bool vtkNativePartitioningStrategy::InitializeCuts(vtkDataObjectTree* input)
{
  if (!(vtkPartitionedDataSet::SafeDownCast(input) ||
        vtkPartitionedDataSetCollection::SafeDownCast(input)))
  {
    vtkErrorMacro("Input must be a PartitionedDataSet or PartitionedDataSetCollection");
    return false;
  }

  auto comm = vtkDIYUtilities::GetCommunicator(this->Controller);
  auto gbounds = ::GetGlobalBounds(input, comm);

  // Step 1:
  // Generate cuts (or use existing cuts).
  if (this->UseExplicitCuts && this->ExpandExplicitCuts && gbounds.IsValid())
  {
    auto bbox = gbounds;
    double xInflate = bbox.GetLength(0) < ::BOUNDING_BOX_LENGTH_TOLERANCE
      ? ::BOUNDING_BOX_LENGTH_TOLERANCE
      : ::BOUNDING_BOX_INFLATION_RATIO * bbox.GetLength(0);
    double yInflate = bbox.GetLength(1) < ::BOUNDING_BOX_LENGTH_TOLERANCE
      ? ::BOUNDING_BOX_LENGTH_TOLERANCE
      : ::BOUNDING_BOX_INFLATION_RATIO * bbox.GetLength(1);
    double zInflate = bbox.GetLength(2) < ::BOUNDING_BOX_LENGTH_TOLERANCE
      ? ::BOUNDING_BOX_LENGTH_TOLERANCE
      : ::BOUNDING_BOX_INFLATION_RATIO * bbox.GetLength(2);
    bbox.Inflate(xInflate, yInflate, zInflate);

    this->Cuts = vtkNativePartitioningStrategy::ExpandCuts(this->ExplicitCuts, bbox);
  }
  else if (this->UseExplicitCuts)
  {
    this->Cuts = this->ExplicitCuts;
  }
  else
  {
    this->Cuts = this->GenerateCuts(input);
  }
  return true;
}

//------------------------------------------------------------------------------
std::vector<vtkBoundingBox> vtkNativePartitioningStrategy::GenerateCuts(vtkDataObject* dobj)
{
  auto controller = this->GetController();
  const int num_partitions = (controller && this->GetNumberOfPartitions() < 0)
    ? controller->GetNumberOfProcesses()
    : this->GetNumberOfPartitions();
  auto bbox = vtkDIYUtilities::GetLocalBounds(dobj);

  if (bbox.IsValid())
  {
    double xInflate = bbox.GetLength(0) < ::BOUNDING_BOX_LENGTH_TOLERANCE
      ? ::BOUNDING_BOX_LENGTH_TOLERANCE
      : ::BOUNDING_BOX_INFLATION_RATIO * bbox.GetLength(0);
    double yInflate = bbox.GetLength(1) < ::BOUNDING_BOX_LENGTH_TOLERANCE
      ? ::BOUNDING_BOX_LENGTH_TOLERANCE
      : ::BOUNDING_BOX_INFLATION_RATIO * bbox.GetLength(1);
    double zInflate = bbox.GetLength(2) < ::BOUNDING_BOX_LENGTH_TOLERANCE
      ? ::BOUNDING_BOX_LENGTH_TOLERANCE
      : ::BOUNDING_BOX_INFLATION_RATIO * bbox.GetLength(2);
    bbox.Inflate(xInflate, yInflate, zInflate);
  }

  double bds[6];
  bbox.GetBounds(bds);
  return vtkDIYKdTreeUtilities::GenerateCuts(
    dobj, std::max(1, num_partitions), /*use_cell_centers=*/true, controller, bds);
}

//------------------------------------------------------------------------------
std::vector<vtkBoundingBox> vtkNativePartitioningStrategy::ExpandCuts(
  const std::vector<vtkBoundingBox>& cuts, const vtkBoundingBox& bounds)
{
  vtkBoundingBox cutsBounds;
  for (const auto& bbox : cuts)
  {
    cutsBounds.AddBox(bbox);
  }

  if (!bounds.IsValid() || !cutsBounds.IsValid() || cutsBounds.Contains(bounds))
  {
    // nothing to do.
    return cuts;
  }

  std::vector<vtkBoundingBox> result = cuts;
  for (auto& bbox : result)
  {
    if (!bbox.IsValid())
    {
      continue;
    }

    double bds[6];
    bbox.GetBounds(bds);
    for (int face = 0; face < 6; ++face)
    {
      if (bds[face] == cutsBounds.GetBound(face))
      {
        bds[face] = (face % 2 == 0) ? std::min(bds[face], bounds.GetBound(face))
                                    : std::max(bds[face], bounds.GetBound(face));
      }
    }
    bbox.SetBounds(bds);
    assert(bbox.IsValid()); // input valid implies output is valid too.
  }

  return result;
}

VTK_ABI_NAMESPACE_END
