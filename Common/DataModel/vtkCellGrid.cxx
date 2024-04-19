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

  os << indent << "ArrayGroups: (" << this->ArrayGroups.size() << ")\n";
  for (auto it = this->ArrayGroups.begin(); it != this->ArrayGroups.end(); ++it)
  {
    vtkStringToken attToken(static_cast<vtkStringToken::Hash>(it->first));
    auto attName = attToken.HasData() ? attToken.Data() : "";
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
    os << i2 << attrEntry.second->GetName().Data() << " (" << std::hex << attrEntry.first
       << std::dec << "):\n";
    attrEntry.second->PrintSelf(os, i3);
  }
  os << indent << "HaveShape: " << (this->HaveShape ? "Y" : "N") << "\n";
  if (this->HaveShape)
  {
    os << indent << "ShapeAttribute: " << std::hex << this->ShapeAttribute.GetId() << std::dec
       << " (" << this->ShapeAttribute.Data() << ")\n";
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
  this->RangeCache = src->GetRangeCache();
  this->ShapeAttribute = src->ShapeAttribute;
  this->HaveShape = src->HaveShape;
  this->NextAttribute = src->NextAttribute;

  // We must create new instances of vtkCellMetadata since they point back to
  // the parent vtkCellGrid.
  this->Cells.clear();
  for (const auto& cellEntry : src->Cells)
  {
    auto cellType = vtkCellMetadata::NewInstance(cellEntry.second->GetClassName(), this);
    cellType->ShallowCopy(cellEntry.second);
  }

  this->SetSchema(src->GetSchemaName(), src->GetSchemaVersion());
  this->SetContentVersion(src->GetContentVersion());

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
  this->RangeCache.clear();
  auto* srcShape = src->GetShapeAttribute();
  auto& srcRange = src->GetRangeCache();
  for (const auto& entry : src->Attributes)
  {
    vtkNew<vtkCellAttribute> attribute;
    attribute->DeepCopy(entry.second);
    // Copy any range information for the cell-attribute:
    auto cacheIt = srcRange.find(entry.second);
    if (cacheIt != srcRange.end())
    {
      this->RangeCache[attribute].resize(attribute->GetNumberOfComponents() + 2);
      std::size_t ii = 0;
      for (const auto& componentRange : cacheIt->second)
      {
        if (componentRange.FiniteRangeTime > entry.second->GetMTime())
        {
          this->RangeCache[attribute][ii].FiniteRange = cacheIt->second[ii].FiniteRange;
          this->RangeCache[attribute][ii].FiniteRangeTime.Modified();
        }
        if (componentRange.EntireRangeTime > entry.second->GetMTime())
        {
          this->RangeCache[attribute][ii].EntireRange = cacheIt->second[ii].EntireRange;
          this->RangeCache[attribute][ii].EntireRangeTime.Modified();
        }
        ++ii;
      }
    }
    // Now add the attribute (after adding the range info).
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
  this->NextAttribute = src->NextAttribute;
  this->SetSchema(src->GetSchemaName(), src->GetSchemaVersion());
  this->SetContentVersion(src->GetContentVersion());
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

vtkDataSetAttributes* vtkCellGrid::FindAttributes(vtkStringToken type) const
{
  return this->FindAttributes(type.GetId());
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

//------------------------------------------------------------------------------
bool vtkCellGrid::SupportsGhostArray(int type)
{
  if (type == CELL)
  {
    return true;
  }
  return false;
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

int vtkCellGrid::AddAllCellMetadata()
{
  int numAdded = 0;
  auto metadataTypeNames = vtkCellMetadata::CellTypes();
  for (const auto& metadataTypeName : metadataTypeNames)
  {
    auto metadata = vtkCellMetadata::NewInstance(metadataTypeName, this);
    if (metadata)
    {
      ++numAdded;
    }
  }
  return numAdded;
}

bool vtkCellGrid::RemoveCellMetadata(vtkCellMetadata* cellType)
{
  if (!cellType)
  {
    return false;
  }
  auto it = this->Cells.find(cellType->Hash());
  if (it == this->Cells.end())
  {
    return false;
  }
  this->Cells.erase(it);
  return true;
}

int vtkCellGrid::RemoveUnusedCellMetadata()
{
  int numRemoved = 0;
  std::set<vtkCellMetadata*> unused;
  for (const auto& cellEntry : this->Cells)
  {
    if (cellEntry.second->GetNumberOfCells() == 0)
    {
      unused.insert(cellEntry.second);
    }
  }
  for (const auto& cellType : unused)
  {
    if (this->RemoveCellMetadata(cellType))
    {
      ++numRemoved;
    }
  }
  return numRemoved;
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
    if (it->second != attribute)
    {
      vtkWarningMacro("Attempting to add attribute "
        << attribute << " (" << attribute->GetName().Data() << "), but " << it->second << " ("
        << it->second->GetName().Data() << ") already exists with the same hash "
        << it->second->GetHash() << ". Ignoring.");
    }
    return false;
  }
  this->Attributes[attribute->GetHash()] = attribute;
  attribute->SetId(this->NextAttribute++);
  return true;
}

bool vtkCellGrid::RemoveCellAttribute(vtkCellAttribute* attribute)
{
  if (!attribute)
  {
    return false;
  }
  // Do not allow the shape attribute to be removed:
  if (this->ShapeAttribute.GetId() == attribute->GetHash())
  {
    return false;
  }
  auto it = this->Attributes.find(attribute->GetHash());
  if (it == this->Attributes.end())
  {
    return false;
  }
  // Remove any cache for this cell-attribute's ranges.
  this->RangeCache.erase(attribute);
  // Now unhook the cell-attribute:
  this->Attributes.erase(it);
  return true;
}

bool vtkCellGrid::GetCellAttributeRange(
  vtkCellAttribute* attribute, int componentIndex, double range[2], bool finiteRange) const
{
  // Invalidate the range so early returns indicate we could not compute one.
  range[0] = 1.;
  range[1] = 0.;

  if (!attribute || componentIndex < -2 || componentIndex >= attribute->GetNumberOfComponents())
  {
    return false;
  }

  // If attribute does not belong to this vtkCellGrid, we cannot proceed.
  auto attIt = this->Attributes.find(attribute->GetHash());
  if (attIt == this->Attributes.end())
  {
    return false;
  }

  // If the cache does not exist, is not up to date, or is not the right size
  // (i.e., because someone forgot to call Modified() on the attribute), then
  // recompute the range.
  auto cacheIt = this->RangeCache.find(attribute);
  if (cacheIt == this->RangeCache.end() ||
    (finiteRange &&
      (cacheIt->second[componentIndex + 2].FiniteRangeTime < attribute->GetMTime() ||
        cacheIt->second[componentIndex + 2].FiniteRange.size() <=
          static_cast<std::size_t>(componentIndex))) ||
    (!finiteRange &&
      (cacheIt->second[componentIndex + 2].EntireRangeTime < attribute->GetMTime() ||
        cacheIt->second[componentIndex + 2].EntireRange.size() <=
          static_cast<std::size_t>(componentIndex))))
  {
    if (!this->ComputeRangeInternal(attribute, componentIndex, finiteRange))
    {
      return false;
    }
    cacheIt = this->RangeCache.find(attribute);
    if (cacheIt == this->RangeCache.end())
    {
      return false;
    }
  }

  // Copy the cache into our result.
  if (finiteRange)
  {
    for (int ii = 0; ii < 2; ++ii)
    {
      range[ii] = cacheIt->second[componentIndex + 2].FiniteRange[ii];
    }
  }
  else
  {
    for (int ii = 0; ii < 2; ++ii)
    {
      range[ii] = cacheIt->second[componentIndex + 2].EntireRange[ii];
    }
  }
  return true;
}

std::set<int> vtkCellGrid::GetCellAttributeIds() const
{
  std::set<int> attributeIds;
  for (const auto& entry : this->Attributes)
  {
    attributeIds.insert(entry.second->GetId());
  }
  return attributeIds;
}

std::vector<int> vtkCellGrid::GetUnorderedCellAttributeIds() const
{
  std::set<int> attributeIds;
  for (const auto& entry : this->Attributes)
  {
    attributeIds.insert(entry.second->GetId());
  }
  std::vector<int> result(attributeIds.begin(), attributeIds.end());
  return result;
}

vtkCellAttribute* vtkCellGrid::GetCellAttribute(vtkStringToken::Hash hash)
{
  auto it = this->Attributes.find(hash);
  if (it == this->Attributes.end())
  {
    return nullptr;
  }
  return it->second;
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

vtkCellAttribute* vtkCellGrid::GetCellAttributeByNameAndType(
  const std::string& name, vtkStringToken attType)
{
  for (const auto& entry : this->Attributes)
  {
    if (entry.second->GetName() == name && entry.second->GetAttributeType() == attType)
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
  do
  {
    query->StartPass();
    for (const auto& cellType : this->Cells)
    {
      ok &= cellType.second->Query(query);
    }
  } while (query->IsAnotherPassRequired());
  query->Finalize();
  return ok;
}

void vtkCellGrid::SetSchema(vtkStringToken name, vtkTypeUInt32 version)
{
  if (name == this->SchemaName && version == this->SchemaVersion)
  {
    return;
  }
  this->Modified();
  this->SchemaName = name;
  this->SchemaVersion = version;
}

vtkCellGrid* vtkCellGrid::GetData(vtkInformation* info)
{
  return info ? vtkCellGrid::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

vtkCellGrid* vtkCellGrid::GetData(vtkInformationVector* v, int i)
{
  return vtkCellGrid::GetData(v->GetInformationObject(i));
}

vtkDataArray* vtkCellGrid::CorrespondingArray(
  vtkCellGrid* gridA, vtkDataArray* arrayA, vtkCellGrid* gridB)
{
  vtkDataArray* arrayB = nullptr;
  if (!gridA || !gridB || !arrayA || !arrayA->GetName() || !arrayA->GetName()[0])
  {
    return arrayB;
  }

  const char* arrayName = arrayA->GetName();

  // We don't currently index arrays by their parent group.
  // Just iterate groups until we find a match.
  for (const auto& groupEntry : gridA->ArrayGroups)
  {
    auto* array = groupEntry.second->GetArray(arrayName);
    if (array != arrayA)
    {
      continue;
    }
    auto* groupB = gridB->FindAttributes(groupEntry.first);
    if (!groupB)
    {
      return arrayB;
    }
    arrayB = groupB->GetArray(arrayName);
    if (arrayB)
    {
      return arrayB;
    }
    // Continue, hoping arrayA is in multiple array groups…
  }
  return arrayB;
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

bool vtkCellGrid::ComputeRangeInternal(
  vtkCellAttribute* attribute, int component, bool finiteRange) const
{
  auto* self = const_cast<vtkCellGrid*>(this);
  auto& cache = this->RangeCache[attribute]; // This inserts a blank entry if none exists.
  // Ensure the range vectors are the proper size:
  if (cache.size() != static_cast<std::size_t>(attribute->GetNumberOfComponents() + 2))
  {
    cache.resize(attribute->GetNumberOfComponents() + 2);
  }
  vtkNew<vtkCellGridRangeQuery> rangeQuery;
  rangeQuery->SetComponent(component);
  rangeQuery->SetFiniteRange(finiteRange);
  rangeQuery->SetCellGrid(self);
  rangeQuery->SetCellAttribute(attribute);
  if (!self->Query(rangeQuery))
  {
    vtkWarningMacro("Range computation for \"" << attribute->GetName().Data() << "\" "
                                               << "(" << component << ") was partial at best.");
  }
  return true;
}

VTK_ABI_NAMESPACE_END
