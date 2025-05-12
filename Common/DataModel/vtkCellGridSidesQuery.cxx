// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridSidesQuery.h"

#include "vtkBoundingBox.h"
#include "vtkCellGridSidesCache.h"
#include "vtkIdTypeArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <array>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkCellGridSidesQuery);

vtkCellGridSidesQuery::~vtkCellGridSidesQuery()
{
  this->SetSideCache(nullptr);
}

void vtkCellGridSidesQuery::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SideCache: " << this->SideCache << "\n";
  os << indent << "Sides: " << this->Sides.size() << "\n";
  os << indent << "PreserveRenderableInputs: " << (this->PreserveRenderableInputs ? "Y" : "N")
     << "\n";
  os << indent
     << "OmitSidesForRenderableInputs: " << (this->OmitSidesForRenderableInputs ? "Y" : "N")
     << "\n";
  os << indent << "OutputDimensionControl: " << std::hex << this->OutputDimensionControl << std::dec
     << "\n";
  os << indent
     << "SelectionType: " << vtkCellGridSidesQuery::SelectionModeToLabel(this->SelectionType).Data()
     << "\n";
  os << indent
     << "SummaryStrategy: " << vtkCellGridSidesQuery::SummaryStrategyToLabel(this->Strategy).Data()
     << "\n";
}

bool vtkCellGridSidesQuery::Initialize()
{
  bool ok = this->Superclass::Initialize(); // Reset Pass number.
  // If we don't have a side-cache, make one as responders should be able to
  // assume it exists. But warn if we have to create one; this is really the
  // job of the filter.
  if (!this->SideCache)
  {
    this->TemporarySideCache = true;
    auto* sideCache = vtkCellGridSidesCache::New();
    this->SideCache = sideCache;
    vtkWarningMacro("No side cache was provided; creating a temporary.");
  }
  else
  {
    // If the cache is older than the query, reset the cache.
    // Otherwise, allow responders to skip hashing their sides.
    if (this->GetMTime() > this->SideCache->GetMTime())
    {
      this->SideCache->Initialize();
    }
  }

  return ok;
}

void vtkCellGridSidesQuery::StartPass()
{
  this->vtkCellGridQuery::StartPass();
  auto work = static_cast<PassWork>(this->Pass);
  switch (work)
  {
    case PassWork::HashSides:
    case PassWork::GenerateSideSets:
      // No work to do.
      break;
    case PassWork::Summarize:
      this->Sides.clear();
      break;
  }
}

bool vtkCellGridSidesQuery::IsAnotherPassRequired()
{
  return this->Pass < PassWork::GenerateSideSets;
}

bool vtkCellGridSidesQuery::Finalize()
{
  this->Sides.clear();
  if (this->TemporarySideCache)
  {
    this->SideCache->Delete();
    this->SideCache = nullptr;
  }
  return true;
}

std::vector<vtkCellGridSidesQuery::SideSetArray> vtkCellGridSidesQuery::GetSideSetArrays(
  vtkStringToken cellType)
{
  std::vector<SideSetArray> result;
  auto cellTypeIt = this->Sides.find(cellType);
  if (cellTypeIt == this->Sides.end())
  {
    return result;
  }

  for (const auto& sideShapeEntry : cellTypeIt->second)
  {
    vtkIdType sideCount = 0;
    for (const auto& entry : sideShapeEntry.second)
    {
      sideCount += entry.second.size();
    }
    auto sideArray = vtkSmartPointer<vtkIdTypeArray>::New();
    sideArray->SetName("conn");
    sideArray->SetNumberOfComponents(2); // tuples are (cell ID, side ID)
    sideArray->SetNumberOfTuples(sideCount);
    vtkIdType sideId = 0;
    std::array<vtkIdType, 2> sideTuple;
    for (const auto& entry : sideShapeEntry.second)
    {
      sideTuple[0] = entry.first;
      for (const auto& ss : entry.second)
      {
        sideTuple[1] = ss;
        sideArray->SetTypedTuple(sideId, sideTuple.data());
        ++sideId;
      }
    }
    result.push_back({ cellTypeIt->first, sideShapeEntry.first, sideArray });
  }

  return result;
}

vtkStringToken vtkCellGridSidesQuery::SelectionModeToLabel(SelectionMode mode)
{
  switch (mode)
  {
    default:
    case SelectionMode::Input:
      return "Input";
    case SelectionMode::Output:
      return "Output";
  }
}

vtkCellGridSidesQuery::SelectionMode vtkCellGridSidesQuery::SelectionModeFromLabel(
  vtkStringToken token)
{
  switch (token.GetId())
  {
    default:
    case "Input"_hash:
      return SelectionMode::Input;
    case "Output"_hash:
      return SelectionMode::Output;
  }
}

vtkStringToken vtkCellGridSidesQuery::SummaryStrategyToLabel(SummaryStrategy strategy)
{
  switch (strategy)
  {
    case SummaryStrategy::Winding:
      return "Winding";
    case SummaryStrategy::AnyOccurrence:
      return "AnyOccurrence";
    default:
    case SummaryStrategy::Boundary:
      return "Boundary";
  }
}

vtkCellGridSidesQuery::SummaryStrategy vtkCellGridSidesQuery::SummaryStrategyFromLabel(
  vtkStringToken token)
{
  switch (token.GetId())
  {
    case "Winding"_hash:
      return SummaryStrategy::Winding;
    case "AnyOccurrence"_hash:
      return SummaryStrategy::AnyOccurrence;
    default:
    case "Boundary"_hash:
      return SummaryStrategy::Boundary;
  }
}

void vtkCellGridSidesQuery::SetSideCache(vtkCellGridSidesCache* cache)
{
  if (this->SideCache == cache)
  {
    return;
  }
  if (this->SideCache)
  {
    this->SideCache->Delete();
  }
  this->SideCache = cache;
  this->TemporarySideCache = !cache;
  if (this->SideCache)
  {
    this->SideCache->Register(this);
  }
  this->Modified();
}

VTK_ABI_NAMESPACE_END
