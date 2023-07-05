// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellMetadata.h"

#include "vtkCellGrid.h"

VTK_ABI_NAMESPACE_BEGIN

vtkCellMetadata::ConstructorMap vtkCellMetadata::Constructors;
vtkNew<vtkCellGridResponders> vtkCellMetadata::Responders;

vtkCellMetadata::~vtkCellMetadata()
{
  this->CellGrid = nullptr; // We don't own a reference to the cell grid... it owns us.
}

vtkSmartPointer<vtkCellMetadata> vtkCellMetadata::NewInstance(
  vtkStringToken className, vtkCellGrid* grid)
{
  vtkSmartPointer<vtkCellMetadata> result;
  auto it = vtkCellMetadata::Constructors.find(className);
  if (it != vtkCellMetadata::Constructors.end())
  {
    result = it->second(grid);
    if (result && grid)
    {
      result = grid->AddCellMetadata(result);
    }
  }
  return result;
}

void vtkCellMetadata::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CellGrid: " << this->CellGrid << "\n";
}

bool vtkCellMetadata::SetCellGrid(vtkCellGrid* parent)
{
  if (this->CellGrid != parent)
  {
    this->CellGrid = parent;
    return true;
  }
  return false;
}

bool vtkCellMetadata::Query(vtkCellGridQuery* query)
{
  bool ok = vtkCellMetadata::Responders->Query(this, query);
  return ok;
}

VTK_ABI_NAMESPACE_END
