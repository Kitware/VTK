// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGrid.h"

#include "vtkCellAttribute.h"
#include "vtkCellGridBoundsQuery.h"
#include "vtkCellGridCopyQuery.h"
#include "vtkCellMetadata.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationDataObjectKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGrid);
vtkInformationKeyMacro(vtkCellGrid, ARRAY_GROUP_IDS, IntegerVector);

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

  vtkNew<vtkCellGridCopyQuery> copier;
  copier->SetSource(src);
  copier->SetTarget(this);
  copier->CopyOnlyShapeOff();
  copier->AddAllSourceCellAttributeIds();
  copier->CopyCellsOn();
  copier->CopyArraysOn();
  copier->CopyArrayValuesOn();
  copier->DeepCopyArraysOff();
  if (!src->Query(copier))
  {
    vtkErrorMacro("Failed to copy the source " << src);
  }
}

void vtkCellGrid::DeepCopy(vtkDataObject* baseSrc)
{
  auto* src = vtkCellGrid::SafeDownCast(baseSrc);
  if (!src)
  {
    vtkErrorMacro("Cannot shallow-copy a null object or object of a different type.");
    return;
  }

  vtkNew<vtkCellGridCopyQuery> copier;
  copier->SetSource(src);
  copier->SetTarget(this);
  copier->CopyOnlyShapeOff();
  copier->AddAllSourceCellAttributeIds();
  copier->CopyCellsOn();
  copier->CopyArraysOn();
  copier->CopyArrayValuesOn();
  copier->DeepCopyArraysOn();
  if (!src->Query(copier))
  {
    vtkErrorMacro("Failed to copy the source " << src);
  }
}

bool vtkCellGrid::CopyStructure(vtkCellGrid* other, bool byReference)
{
  vtkNew<vtkCellGridCopyQuery> copier;
  copier->SetSource(other);
  copier->SetTarget(this);
  copier->CopyOnlyShapeOn();
  copier->SetDeepCopyArrays(!byReference);
  if (other->Query(copier))
  {
    return true;
  }
  return false;
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

void vtkCellGrid::MapArrayLocations(
  std::unordered_map<vtkAbstractArray*, vtkStringToken>& arrayLocations) const
{
  for (const auto& entry : this->ArrayGroups)
  {
    auto groupToken = entry.first;
    for (vtkIdType ii = 0; ii < entry.second->GetNumberOfArrays(); ++ii)
    {
      auto* arr = entry.second->GetAbstractArray(ii);
      if (arr)
      {
        arrayLocations[arr] = groupToken;
      }
    }
  }
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
  // First, see if the array is marked with a group for fast lookup.
  if (arr->HasInformation())
  {
    auto* info = arr->GetInformation();
    if (info->Has(ARRAY_GROUP_IDS()))
    {
      int numGroups = info->Length(vtkCellGrid::ARRAY_GROUP_IDS());
      int* groupIds = info->Get(vtkCellGrid::ARRAY_GROUP_IDS());
      for (int gg = 0; gg < numGroups; ++gg)
      {
        if (auto* group = this->FindAttributes(groupIds[gg]))
        {
          if (auto* array = group->GetAbstractArray(arr->GetName()))
          {
            if (array != arr)
            {
              // NB: We might update info by rewriting ARRAY_GROUP_IDS to
              //     exclude groupIds[gg], but it is possible – because
              //     arrays are shallow-copied – that they may end up in
              //     multiple groups across multiple instances of
              //     vtkCellGrid and we do not necessarily want to de-index
              //     arr across all cell-grids.
              continue;
            }
            return groupIds[gg];
          }
        }
      }
    }
  }

  // Next, search through DOF arrays:
  for (auto it = this->ArrayGroups.begin(); it != this->ArrayGroups.end(); ++it)
  {
    for (int ii = 0; ii < it->second->GetNumberOfArrays(); ++ii)
    {
      if (it->second->GetAbstractArray(ii) == arr)
      {
        // Accelerate next lookup by adding the result.
        arr->GetInformation()->Append(ARRAY_GROUP_IDS(), it->first);
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
  // Because we have added cells, clear any cell-attribute ranges cached.
  this->RangeCache.clear();
  return cellType;
}

int vtkCellGrid::AddAllCellMetadata()
{
  int numAdded = 0;
  vtkIdType numNewCells = 0;
  auto metadataTypeNames = vtkCellMetadata::CellTypes();
  for (const auto& metadataTypeName : metadataTypeNames)
  {
    auto metadata = vtkCellMetadata::NewInstance(metadataTypeName, this);
    if (metadata)
    {
      ++numAdded;
      numNewCells += metadata->GetNumberOfCells();
    }
  }
  if (numNewCells > 0)
  {
    // Because we have added cells, clear any cell-attribute ranges cached.
    this->RangeCache.clear();
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
  // Because we have removed cells, clear any cell-attribute ranges cached.
  this->RangeCache.clear();
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
    cacheIt->second.size() <= static_cast<std::size_t>(componentIndex + 2) ||
    (finiteRange && cacheIt->second[componentIndex + 2].FiniteRangeTime < attribute->GetMTime()) ||
    (!finiteRange && cacheIt->second[componentIndex + 2].EntireRangeTime < attribute->GetMTime()))
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

std::vector<vtkSmartPointer<vtkCellAttribute>> vtkCellGrid::GetCellAttributeList() const
{
  std::vector<vtkSmartPointer<vtkCellAttribute>> result;
  result.reserve(this->Attributes.size());
  for (const auto& entry : this->Attributes)
  {
    result.emplace_back(entry.second);
  };
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
  if (!query->Initialize())
  {
    ok = false;
    return ok;
  }
  do
  {
    query->StartPass();
    for (const auto& cellType : this->Cells)
    {
      ok &= cellType.second->Query(query);
    }
  } while (query->IsAnotherPassRequired());
  bool didFinalize = query->Finalize();
  ok &= didFinalize;
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

  // If we have ARRAY_GROUP_IDS, look there first.
  auto* infoA = arrayA->HasInformation() ? arrayA->GetInformation() : nullptr;
  if (infoA && infoA->Has(ARRAY_GROUP_IDS()))
  {
    int numGroups = infoA->Length(vtkCellGrid::ARRAY_GROUP_IDS());
    int* groupIds = infoA->Get(vtkCellGrid::ARRAY_GROUP_IDS());
    for (int gg = 0; gg < numGroups; ++gg)
    {
      if (auto* groupA = gridA->FindAttributes(groupIds[gg]))
      {
        if (auto* array = groupA->GetArray(arrayName))
        {
          if (array != arrayA)
          {
            continue;
          }
          if (auto* groupB = gridB->FindAttributes(groupIds[gg]))
          {
            arrayB = groupB->GetArray(arrayName);
            if (arrayB)
            {
              return arrayB;
            }
          }
        }
      }
    }
  }

  // We don't currently index arrays by their parent group.
  // Just iterate groups until we find a match.
  for (const auto& groupEntry : gridA->ArrayGroups)
  {
    auto* array = groupEntry.second->GetArray(arrayName);
    if (array != arrayA)
    {
      continue;
    }
    // The input array was not marked with a group but was present in a group; add it:
    arrayA->GetInformation()->Append(ARRAY_GROUP_IDS(), groupEntry.first);
    auto* groupB = gridB->FindAttributes(groupEntry.first);
    if (!groupB)
    {
      return arrayB;
    }
    arrayB = groupB->GetArray(arrayName);
    if (arrayB)
    {
      if (!arrayB->HasInformation() || !arrayB->GetInformation()->Has(ARRAY_GROUP_IDS()))
      {
        // Mark arrayB for fast lookup.
        arrayB->GetInformation()->Append(ARRAY_GROUP_IDS(), groupEntry.first);
      }
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
