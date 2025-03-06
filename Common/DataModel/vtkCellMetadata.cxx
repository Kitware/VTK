// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellMetadata.h"

#include "vtkCellGrid.h"
#include "vtkDebugLeaks.h"

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkCellMetadata::~vtkCellMetadata()
{
  this->CellGrid = nullptr; // We don't own a reference to the cell grid... it owns us.
}

vtkSmartPointer<vtkCellMetadata> vtkCellMetadata::NewInstance(
  vtkStringToken className, vtkCellGrid* grid)
{
  vtkSmartPointer<vtkCellMetadata> result;
  auto& ctors = vtkCellMetadata::Constructors();
  auto it = ctors.find(className);
  if (it != ctors.end())
  {
    result = it->second(grid);
    if (result && grid)
    {
      result = grid->AddCellMetadata(result);
    }
  }
  return result;
}

std::unordered_set<vtkStringToken> vtkCellMetadata::CellTypes()
{
  std::unordered_set<vtkStringToken> cellTypes;
  for (const auto& ctor : vtkCellMetadata::Constructors())
  {
    cellTypes.insert(ctor.first);
  }
  return cellTypes;
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
  bool ok = vtkCellMetadata::GetResponders()->Query(this, query);
  return ok;
}

vtkCellGridResponders* vtkCellMetadata::GetResponders()
{
  auto& responders = token_NAMESPACE::singletons().get<vtkSmartPointer<vtkCellGridResponders>>();
  if (!responders)
  {
    responders = vtkSmartPointer<vtkCellGridResponders>::New();
    vtkDebugLeaks::AddFinalizer([]() { vtkCellMetadata::ClearResponders(); });
  }
  return responders;
}

void vtkCellMetadata::ClearResponders()
{
  // No matter whether we have assigned a value or not, just replace it
  // with a null pointer. This will cause any assigned object to be destroyed.
  token_NAMESPACE::singletons().erase<vtkSmartPointer<vtkCellGridResponders>>();
}

vtkCellGridResponders* vtkCellMetadata::GetCaches()
{
  (void)this; // Keep clang-tidy from complaining that this method should be static.
  return vtkCellMetadata::GetResponders();
}

vtkCellMetadata::ConstructorMap& vtkCellMetadata::Constructors()
{
  return token_NAMESPACE::singletons().get<ConstructorMap>();
}

VTK_ABI_NAMESPACE_END
