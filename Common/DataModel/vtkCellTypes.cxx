// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellTypes.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCellTypes);

//------------------------------------------------------------------------------
const char* vtkCellTypes::GetClassNameFromTypeId(int type)
{
  return vtkCellTypeUtilities::GetClassNameFromTypeId(type);
}

//------------------------------------------------------------------------------
int vtkCellTypes::GetTypeIdFromClassName(const char* classname)
{
  return vtkCellTypeUtilities::GetTypeIdFromClassName(classname);
}

//------------------------------------------------------------------------------
vtkCellTypes::vtkCellTypes()
  : TypeArray(vtkSmartPointer<vtkUnsignedCharArray>::New())
  , MaxId(-1)
{
}

//------------------------------------------------------------------------------
int vtkCellTypes::Allocate(vtkIdType sz, vtkIdType ext)
{
  this->MaxId = -1;

  if (!this->TypeArray)
  {
    this->TypeArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
  }
  this->TypeArray->Allocate(sz, ext);

  return 1;
}

//------------------------------------------------------------------------------
// VTK_DEPRECATED_IN_9_6_0
void vtkCellTypes::InsertCell(vtkIdType id, unsigned char type, vtkIdType)
{
  this->InsertCell(id, type);
}

//------------------------------------------------------------------------------
// Add a cell at specified id.
void vtkCellTypes::InsertCell(vtkIdType cellId, unsigned char type)
{
  vtkDebugMacro(<< "Insert Cell id: " << cellId);
  TypeArray->InsertValue(cellId, type);

  this->MaxId = std::max(cellId, this->MaxId);
}

//------------------------------------------------------------------------------
// VTK_DEPRECATED_IN_9_6_0
vtkIdType vtkCellTypes::InsertNextCell(unsigned char type, vtkIdType)
{
  return this->InsertNextCell(type);
}

//------------------------------------------------------------------------------
// Add a cell to the object in the next available slot.
vtkIdType vtkCellTypes::InsertNextCell(unsigned char type)
{
  vtkDebugMacro(<< "Insert Next Cell " << type);
  this->InsertCell(++this->MaxId, type);
  return this->MaxId;
}

//------------------------------------------------------------------------------
// Specify a group of cell types.
void vtkCellTypes::SetCellTypes(vtkIdType ncells, vtkUnsignedCharArray* cellTypes)
{
  this->TypeArray = cellTypes;
  this->MaxId = ncells - 1;
}

//------------------------------------------------------------------------------
int vtkCellTypes::GetDimension(unsigned char type)
{
  return vtkCellTypeUtilities::GetDimension(type);
}

//------------------------------------------------------------------------------
// Reclaim any extra memory.
void vtkCellTypes::Squeeze()
{
  this->TypeArray->Squeeze();
}

//------------------------------------------------------------------------------
// Initialize object without releasing memory.
void vtkCellTypes::Reset()
{
  this->MaxId = -1;
}

//------------------------------------------------------------------------------
unsigned long vtkCellTypes::GetActualMemorySize()
{
  unsigned long size = 0;

  if (this->TypeArray)
  {
    size += this->TypeArray->GetActualMemorySize();
  }

  return static_cast<unsigned long>(ceil(size / 1024.0)); // kibibytes
}

//------------------------------------------------------------------------------
void vtkCellTypes::DeepCopy(vtkCellTypes* src)
{
  if (!this->TypeArray)
  {
    this->TypeArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
  }
  this->TypeArray->DeepCopy(src->TypeArray);

  this->MaxId = src->MaxId;
}

//------------------------------------------------------------------------------
void vtkCellTypes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "TypeArray:\n";
  this->TypeArray->PrintSelf(os, indent.GetNextIndent());

  os << indent << "MaxId: " << this->MaxId << "\n";
}

VTK_ABI_NAMESPACE_END
