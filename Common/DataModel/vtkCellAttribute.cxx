// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellAttribute.h"

#include "vtkAbstractArray.h"
#include "vtkObjectFactory.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellAttribute);

void vtkCellAttribute::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Name: " << this->GetName().Data() << "\n";
  os << indent << "Id: " << this->Id << "\n";
  os << indent << "Type: " << this->GetAttributeType().Data() << "\n";
  os << indent << "Space: " << this->GetSpace().Data() << "\n";
  os << indent << "NumberOfComponents: " << this->GetNumberOfComponents() << "\n";
  os << indent << "Hash: " << this->GetHash() << "\n";
  os << indent << "AllArrays: (" << this->AllArrays.size() << " cell types)\n";
  vtkIndent i2 = indent.GetNextIndent();
  vtkIndent i3 = i2.GetNextIndent();
  for (const auto& arraysByCell : this->AllArrays)
  {
    os << i2 << arraysByCell.first.Data() << ":\n";
    for (const auto& arrayEntry : arraysByCell.second)
    {
      os << i3 << arrayEntry.first.Data() << ": " << arrayEntry.second->GetName() << "\n";
    }
  }
}

bool vtkCellAttribute::Initialize(
  vtkStringToken name, vtkStringToken attributeType, vtkStringToken space, int numberOfComponents)
{
  if (this->Name == name && this->AttributeType == attributeType && this->Space == space &&
    this->NumberOfComponents == numberOfComponents)
  {
    return false;
  }

  this->Name = name;
  this->AttributeType = attributeType;
  this->Space = space;
  this->NumberOfComponents = numberOfComponents;

  this->AllArrays.clear();
  this->Modified();

  return true;
}

vtkStringToken::Hash vtkCellAttribute::GetHash() const
{
  std::ostringstream str;
  str << this->GetNumberOfComponents() << "-" << this->GetName().Data() << "-"
      << this->GetAttributeType().Data() << "-" << this->GetSpace().Data();
  vtkStringToken result(str.str());
  return result.GetId();
}

vtkCellAttribute::ArraysForCellType vtkCellAttribute::GetArraysForCellType(
  vtkStringToken cellType) const
{
  auto it = this->AllArrays.find(cellType);
  if (it == this->AllArrays.end())
  {
    return {};
  }
  return it->second;
}

bool vtkCellAttribute::SetArraysForCellType(
  vtkStringToken cellType, const ArraysForCellType& arrays)
{
  auto it = this->AllArrays.find(cellType);
  if (it == this->AllArrays.end() || it->second != arrays)
  {
    this->AllArrays[cellType] = arrays;
    this->Modified();
    return true;
  }
  return false;
}

bool vtkCellAttribute::SetColormap(vtkScalarsToColors* colormap)
{
  if (colormap == this->Colormap)
  {
    return false;
  }
  this->Colormap = colormap;
  this->Modified();
  return true;
}

void vtkCellAttribute::ShallowCopy(vtkCellAttribute* other)
{
  if (!other)
  {
    return;
  }

  this->Name = other->Name;
  this->AttributeType = other->AttributeType;
  this->Space = other->Space;
  this->NumberOfComponents = other->NumberOfComponents;
  this->AllArrays = other->AllArrays;

  // Do not copy other->Id! Identifiers must be unique across attributes.

  this->Colormap = other->Colormap;
}

void vtkCellAttribute::DeepCopy(
  vtkCellAttribute* other, const std::map<vtkAbstractArray*, vtkAbstractArray*>& arrayRewrites)
{
  if (!other)
  {
    return;
  }

  this->Name = other->Name;
  this->AttributeType = other->AttributeType;
  this->Space = other->Space;
  this->NumberOfComponents = other->NumberOfComponents;

  // Copy arrays, then rewrite pointers as directed.
  this->AllArrays = other->AllArrays;
  if (!arrayRewrites.empty())
  {
    for (auto& entry : this->AllArrays)
    {
      for (auto& subentry : entry.second)
      {
        auto it = arrayRewrites.find(subentry.second.GetPointer());
        if (it != arrayRewrites.end())
        {
          subentry.second = it->second;
        }
      }
    }
  }

  // Do not copy other->Id! Identifiers must be unique across attributes.

  // Clone any colormap
  if (other->Colormap)
  {
    this->Colormap = vtkScalarsToColors::SafeDownCast(
      vtkObjectFactory::CreateInstance(other->Colormap->GetClassName()));
    if (this->Colormap)
    {
      this->Colormap->DeepCopy(other->Colormap);
    }
    else
    {
      vtkErrorMacro("Could not clone the attribute's colormap.");
    }
  }
  else
  {
    this->Colormap = nullptr;
  }
}

VTK_ABI_NAMESPACE_END
