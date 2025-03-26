// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridCopyQuery.h"

#include "vtkAbstractArray.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridCopyQuery);
vtkCxxSetObjectMacro(vtkCellGridCopyQuery, Source, vtkCellGrid);
vtkCxxSetObjectMacro(vtkCellGridCopyQuery, Target, vtkCellGrid);

vtkCellGridCopyQuery::~vtkCellGridCopyQuery()
{
  this->SetSource(nullptr);
  this->SetTarget(nullptr);
  this->ResetCellAttributeIds();
}

void vtkCellGridCopyQuery::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Source: " << this->Source << "\n";
  os << indent << "Target: " << this->Target << "\n";
  os << indent << "CopyCells: " << (this->CopyCells ? "Y" : "N") << "\n";
  os << indent << "CopyOnlyShape: " << (this->CopyOnlyShape ? "Y" : "N") << "\n";
  os << indent << "CopyArrays: " << (this->CopyArrays ? "Y" : "N") << "\n";
  os << indent << "CopyArrayValues: " << (this->CopyArrayValues ? "Y" : "N") << "\n";
  os << indent << "DeepCopyArrays: " << (this->DeepCopyArrays ? "Y" : "N") << "\n";
  os << indent << "CopySchema: " << (this->CopySchema ? "Y" : "N") << "\n";
  os << indent << "CellAttributeIds: ";
  for (const auto& id : this->CellAttributeIds)
  {
    os << " " << id;
  }
  if (this->CellAttributeIds.empty())
  {
    os << " (empty)";
  }
  os << "\n";
  vtkIndent i2 = indent.GetNextIndent();
  os << indent << "ArrayMap: " << this->ArrayMap.size() << " entries\n";
  for (const auto& entry : this->ArrayMap)
  {
    if (!entry.first || !entry.second)
    {
      os << i2 << entry.first << ": " << entry.second << "\n";
    }
    else
    {
      os << i2 << entry.first << " (" << entry.first->GetName() << "): " << entry.second << " ("
         << entry.second->GetName() << ")\n";
    }
  }
  os << indent << "AttributeMap: " << this->AttributeMap.size() << " entries\n";
  for (const auto& entry : this->AttributeMap)
  {
    if (!entry.first || !entry.second)
    {
      os << i2 << entry.first << ": " << entry.second << "\n";
    }
    else
    {
      os << i2 << entry.first << " (" << entry.first->GetName().Data() << "): " << entry.second
         << " (" << entry.second->GetName().Data() << ")\n";
    }
  }
}

bool vtkCellGridCopyQuery::Initialize()
{
  bool ok = this->Superclass::Initialize();
  this->ArrayMap.clear();
  this->AttributeMap.clear();
  return ok;
}

bool vtkCellGridCopyQuery::Finalize()
{
  this->ArrayMap.clear();
  this->AttributeMap.clear();

  if (this->CopySchema)
  {
    this->Target->SetSchema(this->Source->GetSchemaName(), this->Source->GetSchemaVersion());
  }

  if (this->Source->GetContentVersion() > 0)
  {
    this->Target->SetContentVersion(this->Source->GetContentVersion() + 1);
  }

  // Do not copy the integer attribute counter:
  // this->Target->NextAttribute = this->Source->NextAttribute;
  return true;
}

bool vtkCellGridCopyQuery::AddSourceCellAttributeId(int attributeId)
{
  return this->CellAttributeIds.insert(attributeId).second;
}

bool vtkCellGridCopyQuery::RemoveSourceCellAttributeId(int attributeId)
{
  return this->CellAttributeIds.erase(attributeId) > 0;
}

bool vtkCellGridCopyQuery::AddAllSourceCellAttributeIds()
{
  if (this->Source)
  {
    std::size_t nn = this->CellAttributeIds.size();
    auto allIds = this->Source->GetCellAttributeIds();
    this->CellAttributeIds.insert(allIds.begin(), allIds.end());
    return nn != this->CellAttributeIds.size();
  }
  return false;
}

void vtkCellGridCopyQuery::GetCellAttributeIds(vtkIdList* ids) const
{
  if (!ids)
  {
    vtkErrorMacro("Null ids passed to " << __func__ << ".");
    return;
  }
  ids->Initialize();
  ids->SetNumberOfIds(this->CellAttributeIds.size());
  int ii = 0;
  for (const auto& id : this->CellAttributeIds)
  {
    ids->SetId(ii, id);
    ++ii;
  }
}

void vtkCellGridCopyQuery::ResetCellAttributeIds()
{
  this->CellAttributeIds.clear();
}

void vtkCellGridCopyQuery::CopyAttributeArrays(vtkCellAttribute* srcAtt, vtkStringToken cellType)
{
  if (!this->CopyArrays)
  {
    return;
  }
  if (!srcAtt)
  {
    vtkErrorMacro("Null source attribute.");
    return;
  }
  auto srcCellTypeInfo = srcAtt->GetCellTypeInfo(cellType);
  auto& arraysByRole = srcCellTypeInfo.ArraysByRole;
  for (const auto& roleEntry : arraysByRole)
  {
    auto srcArr = roleEntry.second;
    int arrType = this->Source->GetAttributeTypeForArray(srcArr);
    auto* tgtGroup = this->Target->GetAttributes(arrType);
    if (this->CopyArrayValues && !this->DeepCopyArrays)
    {
      // We should copy by referencing the original array;
      // just add the source array to the destination.
      tgtGroup->AddArray(srcArr);
    }
    else
    {
      // Deep-copy the source array as needed.
      // I. See if we've already copied the array:
      auto mapIt = this->ArrayMap.find(srcArr);
      if (mapIt != this->ArrayMap.end())
      {
        // No need to re-create the array; we've already copied it.
        // Just add it to the array grouping.
        tgtGroup->AddArray(mapIt->second);
        continue;
      }
      // II. We need to create an array.
      auto* tgtArr = vtkAbstractArray::CreateArray(srcArr->GetDataType());
      if (this->CopyArrayValues)
      {
        // Create a deep copy including the values:
        tgtArr->DeepCopy(srcArr);
      }
      else
      {
        // Copy array "metadata" only.
        if (srcArr->HasInformation())
        {
          tgtArr->CopyInformation(srcArr->GetInformation(), /*deep*/ 1);
        }
        tgtArr->SetName(srcArr->GetName());
        tgtArr->SetNumberOfComponents(srcArr->GetNumberOfComponents());
        tgtArr->CopyComponentNames(srcArr);
      }
      tgtGroup->AddArray(tgtArr);
      this->ArrayMap[srcArr] = tgtArr;
      tgtArr->Delete();
    }
  }
}

vtkCellAttribute* vtkCellGridCopyQuery::CopyOrUpdateAttributeRecord(
  vtkCellAttribute* srcAtt, vtkStringToken cellType)
{
  // If we already have the attribute, use it.  Otherwise, create it.
  vtkCellAttribute* targetAttribute = nullptr;
  auto amit = this->AttributeMap.find(srcAtt);
  if (amit == this->AttributeMap.end())
  {
    // We need to create the attribute.
    vtkNew<vtkCellAttribute> tgtAtt;
    tgtAtt->ShallowCopy(srcAtt, /* do not copy arrays for cell types*/ false);
    targetAttribute = tgtAtt;

    // Copy cached range data if we are copying cells.
    if (this->GetCopyCells() && this->GetCopyArrayValues())
    {
      const auto& srcRange = this->Source->GetRangeCache();
      auto& tgtRange = this->Target->GetRangeCache();

      auto cacheIt = srcRange.find(srcAtt);
      if (cacheIt != srcRange.end())
      {
        tgtRange[tgtAtt].resize(tgtAtt->GetNumberOfComponents() + 2);
        std::size_t ii = 0;
        for (const auto& componentRange : cacheIt->second)
        {
          if (componentRange.FiniteRangeTime > srcAtt->GetMTime())
          {
            tgtRange[tgtAtt][ii].FiniteRange = cacheIt->second[ii].FiniteRange;
            tgtRange[tgtAtt][ii].FiniteRangeTime.Modified();
          }
          if (componentRange.EntireRangeTime > srcAtt->GetMTime())
          {
            tgtRange[tgtAtt][ii].EntireRange = cacheIt->second[ii].EntireRange;
            tgtRange[tgtAtt][ii].EntireRangeTime.Modified();
          }
          ++ii;
        }
      }
    }

    this->Target->AddCellAttribute(tgtAtt);
    this->AttributeMap[srcAtt] = tgtAtt;
    if (this->Source->GetShapeAttribute() == srcAtt)
    {
      this->Target->SetShapeAttribute(tgtAtt);
    }
  }
  else
  {
    targetAttribute = amit->second;
  }

  // Regardless of whether the attribute pre-existed or not,
  // add arrays for each cell type.
  auto oldCellTypeInfo = srcAtt->GetCellTypeInfo(cellType);
  auto& oldArraysForCellType = oldCellTypeInfo.ArraysByRole;
  vtkCellAttribute::CellTypeInfo newCellTypeInfo;
  auto& arraysForCellType = newCellTypeInfo.ArraysByRole;

  newCellTypeInfo.DOFSharing = oldCellTypeInfo.DOFSharing;
  newCellTypeInfo.FunctionSpace = oldCellTypeInfo.FunctionSpace;
  newCellTypeInfo.Basis = oldCellTypeInfo.Basis;
  newCellTypeInfo.Order = oldCellTypeInfo.Order;
  for (const auto& entry : oldArraysForCellType)
  {
    auto it = this->ArrayMap.find(entry.second);
    if (it != this->ArrayMap.end())
    {
      arraysForCellType[entry.first] = it->second;
    }
    else if (this->CopyArrays && this->CopyArrayValues && !this->DeepCopyArrays)
    {
      arraysForCellType[entry.first] = entry.second;
    }
  }
  targetAttribute->SetCellTypeInfo(cellType, newCellTypeInfo);

  return targetAttribute;
}

VTK_ABI_NAMESPACE_END
