// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGrid.h"

#include "vtkCellAttribute.h"
#include "vtkCellGridBoundsQuery.h"
#include "vtkCellMetadata.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationDataObjectKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStringManager.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGrid);

vtkCellGrid::vtkCellGrid() = default;
vtkCellGrid::~vtkCellGrid() = default;

void vtkCellGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  vtkIndent i2 = indent.GetNextIndent();
  vtkIndent i3 = i2.GetNextIndent();

  os << indent << "Cells: (" << this->Cells.size() << " types)\n";
  for (auto& cellRec : this->Cells)
  {
    os << i2 << cellRec.second->GetClassName() << " (" << cellRec.first.Data() << ")\n";
    cellRec.second->PrintSelf(os, i3);
  }

  auto* smgr = vtkStringToken::GetManager();
  os << indent << "ArrayGroups: (" << this->ArrayGroups.size() << ")\n";
  for (auto it = this->ArrayGroups.begin(); it != this->ArrayGroups.end(); ++it)
  {
    auto attName = smgr ? smgr->Value(it->first) : "";
    if (attName.empty())
    {
      os << i2 << it->first << ": " << it->second << " " << it->second->GetNumberOfArrays()
         << " arrays\n";
    }
    else
    {
      os << i2 << attName << ": " << it->second << " " << it->second->GetNumberOfArrays()
         << " arrays\n";
    }
    it->second->PrintSelf(os, i3);
  }

  os << indent << "Attributes (" << this->Attributes.size() << ")\n";
  for (const auto& attrEntry : this->Attributes)
  {
    os << i2 << attrEntry.first << "\n";
    attrEntry.second->PrintSelf(os, i3);
  }
  os << indent << "HaveShape: " << (this->HaveShape ? "Y" : "N") << "\n";
  if (this->HaveShape)
  {
    os << indent << "ShapeAttribute: " << this->ShapeAttribute.GetId() << " ("
       << this->ShapeAttribute.Data() << ")\n";
  }
  os << indent << "NextAttribute: " << this->NextAttribute << "\n";
}

void vtkCellGrid::Initialize()
{
  this->Superclass::Initialize();
  this->ArrayGroups.clear();
  this->Attributes.clear();
  this->HaveShape = false;
  this->Cells.clear();
}

unsigned long vtkCellGrid::GetActualMemorySize()
{
  auto result = this->Superclass::GetActualMemorySize();
  for (const auto& dsa : this->ArrayGroups)
  {
    result += dsa.second->GetActualMemorySize();
  }
  result += sizeof(*this) / 1024;
  return result;
}

void vtkCellGrid::ShallowCopy(vtkDataObject* baseSrc)
{
  auto* src = vtkCellGrid::SafeDownCast(baseSrc);
  if (!src)
  {
    vtkErrorMacro("Cannot shallow-copy a null object or object of a different type.");
    return;
  }

  this->ArrayGroups.clear();
  for (const auto& entry : src->ArrayGroups)
  {
    auto* dsa = this->GetAttributes(entry.first);
    dsa->ShallowCopy(entry.second);
  }

  // Copy attributes by reference. This works because we shallow-copy the array groups above,
  // so all the arrays from src are referenced by us as well.
  this->Attributes = src->Attributes;
  this->ShapeAttribute = src->ShapeAttribute;
  this->HaveShape = src->HaveShape;

  // We must create new instances of vtkCellMetadata since they point back to
  // the parent vtkCellGrid.
  this->Cells.clear();
  for (const auto& cellEntry : src->Cells)
  {
    auto cellType = vtkCellMetadata::NewInstance(cellEntry.second->GetClassName(), this);
    cellType->ShallowCopy(cellEntry.second);
  }

  this->Modified();
}

void vtkCellGrid::DeepCopy(vtkDataObject* baseSrc)
{
  auto* src = vtkCellGrid::SafeDownCast(baseSrc);
  if (!src)
  {
    vtkErrorMacro("Cannot shallow-copy a null object or object of a different type.");
    return;
  }

  this->Initialize();

  std::map<vtkAbstractArray*, vtkAbstractArray*> arrayRewrites;
  const auto& srcMap = src->GetArrayGroups();
  for (auto it = srcMap.begin(); it != srcMap.end(); ++it)
  {
    auto* dsa = this->GetAttributes(it->first);
    dsa->DeepCopy(it->second);
    if (it->second->GetNumberOfArrays() != dsa->GetNumberOfArrays())
    {
      vtkErrorMacro("Arrays for group "
        << it->first << " cannot be mapped. Cell attributes will reference wrong arrays.");
    }
    else
    {
      for (int ii = 0; ii < dsa->GetNumberOfArrays(); ++ii)
      {
        arrayRewrites[it->second->GetAbstractArray(ii)] = dsa->GetAbstractArray(ii);
      }
    }
  }

  this->Attributes.clear();
  auto* srcShape = src->GetShapeAttribute();
  for (const auto& entry : src->Attributes)
  {
    vtkNew<vtkCellAttribute> attribute;
    attribute->DeepCopy(entry.second);
    this->AddCellAttribute(attribute);
    if (srcShape == entry.second)
    {
      this->SetShapeAttribute(attribute);
    }
  }

  this->Cells.clear();
  for (const auto& cellEntry : src->Cells)
  {
    auto cellType = vtkCellMetadata::NewInstance(cellEntry.second->GetClassName(), this);
    cellType->DeepCopy(cellEntry.second);
  }
  this->Modified();
}

vtkDataSetAttributes* vtkCellGrid::GetAttributes(int type)
{
  auto it = this->ArrayGroups.find(type);
  if (it == this->ArrayGroups.end())
  {
    it =
      this->ArrayGroups.insert(std::make_pair(type, vtkSmartPointer<vtkDataSetAttributes>::New()))
        .first;
  }
  return it->second;
}

vtkDataSetAttributes* vtkCellGrid::GetAttributes(vtkStringToken type)
{
  int key = static_cast<int>(type.GetId());
  return this->GetAttributes(key);
}

vtkDataSetAttributes* vtkCellGrid::FindAttributes(int type) const
{
  auto it = this->ArrayGroups.find(type);
  return it == this->ArrayGroups.end() ? nullptr : it->second;
}

vtkUnsignedCharArray* vtkCellGrid::GetGhostArray(int type)
{
  vtkUnsignedCharArray* result = nullptr;
  auto* dsa = this->FindAttributes(type);
  if (!dsa)
  {
    return result;
  }
  result =
    vtkArrayDownCast<vtkUnsignedCharArray>(dsa->GetArray(vtkDataSetAttributes::GhostArrayName()));
  return result;
}

int vtkCellGrid::GetAttributeTypeForArray(vtkAbstractArray* arr)
{
  // First, search through DOF arrays:
  for (auto it = this->ArrayGroups.begin(); it != this->ArrayGroups.end(); ++it)
  {
    for (int ii = 0; ii < it->second->GetNumberOfArrays(); ++ii)
    {
      if (it->second->GetAbstractArray(ii) == arr)
      {
        return it->first;
      }
    }
  }
  // If not a DOF array, perhaps it is field data:
  for (int ii = 0; ii < this->FieldData->GetNumberOfArrays(); ++ii)
  {
    if (this->FieldData->GetAbstractArray(ii) == arr)
    {
      return FIELD;
    }
  }
  // No such array is owned by this vtkCellGrid:
  return -1;
}

vtkIdType vtkCellGrid::GetNumberOfElements(int type)
{
  auto* dsa = this->FindAttributes(type);
  if (type == CELL)
  {
    return this->GetNumberOfCells();
  }
  else if (!dsa && type == FIELD)
  {
    return this->FieldData->GetNumberOfTuples();
  }
  else if (dsa)
  {
    return dsa->GetNumberOfTuples();
  }
  return 0;
}

vtkIdType vtkCellGrid::GetNumberOfCells()
{
  vtkIdType result = 0;
  for (const auto& cellType : this->Cells)
  {
    result += cellType.second->GetNumberOfCells();
  }
  return result;
}

void vtkCellGrid::GetBounds(double bounds[6])
{
  if (this->CachedBoundsTime < this->GetMTime())
  {
    this->ComputeBoundsInternal();
  }
  for (int ii = 0; ii < 6; ++ii)
  {
    bounds[ii] = this->CachedBounds[ii];
  }
}

vtkCellMetadata* vtkCellGrid::AddCellMetadata(vtkCellMetadata* cellType)
{
  if (!cellType)
  {
    return cellType;
  }
  auto it = this->Cells.find(cellType->Hash());
  if (it != this->Cells.end())
  {
    // Do not take ownership of cellType; return the instance we own.
    return it->second;
  }
  // OK, we don't already have this type... insert it.
  vtkSmartPointer<vtkCellMetadata> owner = cellType;
  this->Cells[owner->Hash()] = owner;
  owner->SetCellGrid(this);
  return cellType;
}

const vtkCellMetadata* vtkCellGrid::GetCellType(vtkStringToken cellTypeName) const
{
  auto it = this->Cells.find(cellTypeName);
  if (it == this->Cells.end())
  {
    return nullptr;
  }
  return it->second;
}

vtkCellMetadata* vtkCellGrid::GetCellType(vtkStringToken cellTypeName)
{
  auto it = this->Cells.find(cellTypeName);
  if (it == this->Cells.end())
  {
    return nullptr;
  }
  return it->second;
}

bool vtkCellGrid::AddCellAttribute(vtkCellAttribute* attribute)
{
  if (!attribute)
  {
    return false;
  }
  auto it = this->Attributes.find(attribute->GetHash());
  if (it != this->Attributes.end())
  {
    // Either we have a hash collision or the attribute already exists.
    return false;
  }
  this->Attributes[attribute->GetHash()] = attribute;
  attribute->SetId(this->NextAttribute++);
  return true;
}

vtkCellAttribute* vtkCellGrid::GetCellAttributeById(int attributeId)
{
  for (const auto& entry : this->Attributes)
  {
    if (entry.second->GetId() == attributeId)
    {
      return entry.second;
    }
  }
  return nullptr;
}

vtkCellAttribute* vtkCellGrid::GetCellAttributeByName(const std::string& name)
{
  for (const auto& entry : this->Attributes)
  {
    if (entry.second->GetName() == name)
    {
      return entry.second;
    }
  }
  return nullptr;
}

vtkCellAttribute* vtkCellGrid::GetShapeAttribute()
{
  if (!this->HaveShape)
  {
    return nullptr;
  }
  auto it = this->Attributes.find(this->ShapeAttribute.GetId());
  if (it == this->Attributes.end())
  {
    return nullptr;
  }
  return it->second.GetPointer();
}

bool vtkCellGrid::SetShapeAttribute(vtkCellAttribute* shape)
{
  if (!shape)
  {
    if (this->HaveShape)
    {
      this->HaveShape = false;
      this->Modified();
      return true;
    }
    return false;
  }
  if (shape->GetHash() == this->ShapeAttribute.GetId() && this->HaveShape)
  {
    return false; // No change.
  }
  // If we don't already own this attribute, add it to this->Attributes:
  auto it = this->Attributes.find(shape->GetHash());
  if (it == this->Attributes.end())
  {
    this->Attributes[shape->GetHash()] = shape;
  }
  else if (it->second != shape)
  {
    vtkErrorMacro("Hash collision for shape attribute. Ignoring call to SetShapeAttribute().");
    return false;
  }
  this->HaveShape = true;
  this->ShapeAttribute = shape->GetHash();
  this->Modified();
  return true;
}

bool vtkCellGrid::Query(vtkCellGridQuery* query)
{
  if (!query)
  {
    return false;
  }

  bool ok = true;
  query->Initialize();
  for (const auto& cellType : this->Cells)
  {
    ok &= cellType.second->Query(query);
  }
  query->Finalize();
  return ok;
}

vtkCellGrid* vtkCellGrid::GetData(vtkInformation* info)
{
  return info ? vtkCellGrid::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

vtkCellGrid* vtkCellGrid::GetData(vtkInformationVector* v, int i)
{
  return vtkCellGrid::GetData(v->GetInformationObject(i));
}

bool vtkCellGrid::ComputeBoundsInternal()
{
  if (!this->HaveShape || !this->GetShapeAttribute())
  {
    vtkMath::UninitializeBounds(this->CachedBounds.data());
    this->CachedBoundsTime.Modified();
  }
  vtkNew<vtkCellGridBoundsQuery> bq;
  if (this->Query(bq))
  {
    bq->GetBounds(this->CachedBounds.data());
    this->CachedBoundsTime.Modified();
    return true;
  }
  return false;
}

VTK_ABI_NAMESPACE_END
