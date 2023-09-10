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
  os << indent << "Range: " << this->Range[0] << " " << this->Range[1] << "\n";
}

void vtkCellGridRangeQuery::Initialize()
{
  // Invalidate the range each time we run.
  this->Range[0] = vtkMath::Inf();
  this->Range[1] = -vtkMath::Inf();
}

void vtkCellGridRangeQuery::Finalize()
{
  if (this->CellGrid && this->CellAttribute)
  {
    if (this->FiniteRange)
    {
      this->CellGrid->GetRangeCache()[this->CellAttribute][this->Component + 2].FiniteRange =
        this->Range;
      this->CellGrid->GetRangeCache()[this->CellAttribute][this->Component + 2]
        .FiniteRangeTime.Modified();
    }
    else
    {
      this->CellGrid->GetRangeCache()[this->CellAttribute][this->Component + 2].EntireRange =
        this->Range;
      this->CellGrid->GetRangeCache()[this->CellAttribute][this->Component + 2]
        .EntireRangeTime.Modified();
    }
  }
}

void vtkCellGridRangeQuery::GetRange(double* bds)
{
  if (!bds)
  {
    return;
  }

  std::copy(this->Range.begin(), this->Range.end(), bds);
}

void vtkCellGridRangeQuery::AddRange(const std::array<double, 2>& range)
{
  // Ignore invalid ranges:
  if (range[1] < range[0])
  {
    return;
  }

  // If this->Range is invalid, just copy the \a range.
  if (this->Range[1] < this->Range[0])
  {
    this->Range = range;
    return;
  }

  if (this->Range[0] > range[0])
  {
    this->Range[0] = range[0];
  }
  if (this->Range[1] < range[1])
  {
    this->Range[1] = range[1];
  }
}

VTK_ABI_NAMESPACE_END
