// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridSidesQuery.h"

#include "vtkBoundingBox.h"
#include "vtkIdTypeArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <array>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridSidesQuery);

void vtkCellGridSidesQuery::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Hashes: " << this->Hashes.size() << "\n";
  os << indent << "Sides: " << this->Sides.size() << "\n";
}

void vtkCellGridSidesQuery::Initialize()
{
  this->Superclass::Initialize(); // Reset Pass number.
  this->Hashes.clear();
}

void vtkCellGridSidesQuery::StartPass()
{
  this->vtkCellGridQuery::StartPass();
  auto work = static_cast<PassWork>(this->Pass);
  switch (work)
  {
    case PassWork::HashSides:
      // No work to do.
      break;
    case PassWork::GenerateSideSets:
      // Create summary data from output of the HashSides pass.
      this->SummarizeSides();
      break;
  }
}

bool vtkCellGridSidesQuery::IsAnotherPassRequired()
{
  return this->Pass < PassWork::GenerateSideSets;
}

void vtkCellGridSidesQuery::Finalize()
{
  this->Sides.clear();
  this->Hashes.clear();
}

void vtkCellGridSidesQuery::SummarizeSides()
{
  this->Sides.clear();
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

VTK_ABI_NAMESPACE_END
