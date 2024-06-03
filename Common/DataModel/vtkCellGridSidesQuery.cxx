// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridSidesQuery.h"

#include "vtkBoundingBox.h"
#include "vtkIdTypeArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <array>
#include <sstream>

// Uncomment the next line for debug printouts.
// #define VTK_DBG_SUMMARIZE_SIDES 1

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkCellGridSidesQuery);

void vtkCellGridSidesQuery::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Hashes: " << this->Hashes.size() << "\n";
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
  this->Hashes.clear();
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
  this->Hashes.clear();
  return true;
}

#if 0
void vtkCellGridSidesQuery::SummarizeSides()
{
  this->Sides.clear();
#ifdef VTK_DBG_SUMMARIZE_SIDES
  std::cout << "Hash table\n";
  for (const auto& entry : this->Hashes)
  {
    std::cout << "  " << std::hex << entry.first << std::dec << "\n";
    for (const auto& side : entry.second.Sides)
    {
      std::cout << "    " << side.CellType.Data() << " " << side.SideShape.Data() << ": " << side.DOF << " " << side.SideId << "\n";
    }
  }
#endif
  for (const auto& entry : this->Hashes)
  {
    if (entry.second.Sides.size() % 2 == 0)
    {
      continue; // Do not output matching pairs of sides.
    }
    for (const auto& ss : entry.second.Sides)
    {
      this->Sides[ss.CellType][ss.SideShape][ss.DOF].insert(ss.SideId);
    }
  }
}
#endif // 0

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
  // XXX(c++14)
#if __cplusplus < 201400L
  if (token == "Output"_token)
  {
    return SelectionMode::Output;
  }
  return SelectionMode::Input;
#else
  switch (token.GetId())
  {
    default:
    case "Input"_hash:
      return SelectionMode::Input;
    case "Output"_hash:
      return SelectionMode::Output;
  }
#endif
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
  // XXX(c++14)
#if __cplusplus < 201400L
  if (token == "Winding"_token)
  {
    return SummaryStrategy::Winding;
  }
  else if (token == "AnyOccurrence"_token)
  {
    return SummaryStrategy::AnyOccurrence;
  }
  return SummaryStrategy::Boundary;
#else
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
#endif
}

VTK_ABI_NAMESPACE_END
