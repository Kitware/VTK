// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridSidesQuery.h"

#include "vtkBoundingBox.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

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
  this->Hashes.clear();
}

void vtkCellGridSidesQuery::Finalize()
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

VTK_ABI_NAMESPACE_END
