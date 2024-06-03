// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridRangeQuery.h"

#include "vtkBoundingBox.h"
#include "vtkCellGrid.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridRangeQuery);
vtkCxxSetObjectMacro(vtkCellGridRangeQuery, CellGrid, vtkCellGrid);

vtkCellGridRangeQuery::~vtkCellGridRangeQuery()
{
  this->SetCellAttribute(nullptr);
  this->SetCellGrid(nullptr);
}

void vtkCellGridRangeQuery::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Component: " << this->Component << "\n";
  os << indent << "FiniteRange: " << (this->FiniteRange ? "ON" : "OFF") << "\n";
  os << indent << "CellGrid: " << this->CellGrid << "\n";
  os << indent << "CellAttribute: " << this->CellAttribute << "\n";
  os << indent << "Ranges:\n";
  vtkIndent i2 = indent.GetNextIndent();
  int cc = -2;
  for (const auto& range : this->Ranges)
  {
    os << i2;
    if (cc == -2)
    {
      os << "L₂-norm";
    }
    else if (cc == -1)
    {
      os << "L₁-norm";
    }
    else
    {
      os << "Component " << cc;
    }
    os << ": " << range[0] << " " << range[1] << "\n";
    ++cc;
  }
}

bool vtkCellGridRangeQuery::Initialize()
{
  bool ok = this->Superclass::Initialize();
  if (!this->CellAttribute)
  {
    vtkErrorMacro("No attribute provided for range computation.");
    return false;
  }
  // Allocate space for all components plus L₁ and L₂ norms.
  this->Ranges.resize(this->CellAttribute->GetNumberOfComponents() + 2);
  // Invalidate the range each time we run.
  for (auto& range : this->Ranges)
  {
    range[0] = vtkMath::Inf();
    range[1] = -vtkMath::Inf();
  }
  return ok;
}

bool vtkCellGridRangeQuery::Finalize()
{
  if (this->CellGrid && this->CellAttribute)
  {
    // Always overwrite the requested component:
    if (this->FiniteRange)
    {
      this->CellGrid->GetRangeCache()[this->CellAttribute][this->Component + 2].FiniteRange =
        this->Ranges[this->Component + 2];
      this->CellGrid->GetRangeCache()[this->CellAttribute][this->Component + 2]
        .FiniteRangeTime.Modified();
    }
    else
    {
      this->CellGrid->GetRangeCache()[this->CellAttribute][this->Component + 2].EntireRange =
        this->Ranges[this->Component + 2];
      this->CellGrid->GetRangeCache()[this->CellAttribute][this->Component + 2]
        .EntireRangeTime.Modified();
    }
    // If any other components have a valid range, copy them as well.
    // (Some range responders may choose to process all components of an array
    // at once instead of individually.)
    int cc = -2;
    for (auto& range : this->Ranges)
    {
      if (range[1] >= range[0])
      {
        if (this->FiniteRange)
        {
          this->CellGrid->GetRangeCache()[this->CellAttribute][cc + 2].FiniteRange = range;
          this->CellGrid->GetRangeCache()[this->CellAttribute][cc + 2].FiniteRangeTime.Modified();
        }
        else
        {
          this->CellGrid->GetRangeCache()[this->CellAttribute][cc + 2].EntireRange = range;
          this->CellGrid->GetRangeCache()[this->CellAttribute][cc + 2].EntireRangeTime.Modified();
        }
      }
      ++cc;
    }
  }
  return true;
}

void vtkCellGridRangeQuery::GetRange(int component, double* bds)
{
  if (!bds)
  {
    return;
  }

  if (component < -2 || component >= this->CellAttribute->GetNumberOfComponents())
  {
    // Provide an invalid range.
    bds[0] = 1;
    bds[1] = 0;
    return;
  }

  std::copy(this->Ranges[component].begin(), this->Ranges[component].end(), bds);
}

const std::array<double, 2>& vtkCellGridRangeQuery::GetRange(int component) const
{
  static const std::array<double, 2> invalid{ 1, 0 };
  if (component < -2 || component >= this->CellAttribute->GetNumberOfComponents())
  {
    return invalid;
  }
  return this->Ranges[component + 2];
}

void vtkCellGridRangeQuery::AddRange(const std::array<double, 2>& range)
{
  this->AddRange(this->Component, range);
}

void vtkCellGridRangeQuery::AddRange(int component, const std::array<double, 2>& range)
{
  // Ignore invalid ranges:
  if (range[1] < range[0])
  {
    return;
  }

  // Ignore invalid components:
  if (component < -2 || component >= this->CellAttribute->GetNumberOfComponents())
  {
    return;
  }

  // If this->Range is invalid, just copy the \a range.
  if (this->Ranges[component + 2][1] < this->Ranges[component + 2][0])
  {
    this->Ranges[component + 2] = range;
    return;
  }

  if (this->Ranges[component + 2][0] > range[0])
  {
    this->Ranges[component + 2][0] = range[0];
  }
  if (this->Ranges[component + 2][1] < range[1])
  {
    this->Ranges[component + 2][1] = range[1];
  }
}

VTK_ABI_NAMESPACE_END
