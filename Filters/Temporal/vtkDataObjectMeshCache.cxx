// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataObjectMeshCache.h"

#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDataObjectMeshCache);

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Cache:";
  if (this->Cache)
  {
    os << endl;
    this->Cache->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "CachedOriginalMeshTime: " << this->CachedOriginalMeshTime << "\n";
  os << indent << "CachedConsumerTime: " << this->CachedConsumerTime << "\n";

  os << indent << "OriginalIdsName:\n";
  for (const auto& attribute : this->OriginalIdsName)
  {
    os << indent.GetNextIndent() << vtkDataObject::GetAssociationTypeAsString(attribute.first)
       << " " << attribute.second << "\n";
  }

  Status status = this->GetStatus();
  os << indent << "Status\n:";
  status.PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
bool vtkDataObjectMeshCache::IsSupportedData(vtkDataObject* dataobject) const
{
  bool supported = this->IsSupportedDataSet(dataobject) || this->IsSupportedComposite(dataobject);
  vtkDebugMacro(" return IsSupportedData: " << supported);
  return supported;
}

//------------------------------------------------------------------------------
bool vtkDataObjectMeshCache::IsSupportedDataSet(vtkDataObject* dataset) const
{
  return vtkPolyData::SafeDownCast(dataset) || vtkUnstructuredGrid::SafeDownCast(dataset);
}

//------------------------------------------------------------------------------
bool vtkDataObjectMeshCache::IsSupportedComposite(vtkDataObject* dataset) const
{
  auto dataObjectTree = vtkDataObjectTree::SafeDownCast(dataset);
  if (!dataObjectTree)
  {
    return false;
  }

  bool supportedLeaves = true;
  auto options =
    vtk::DataObjectTreeOptions::TraverseSubTree | vtk::DataObjectTreeOptions::VisitOnlyLeaves;
  for (auto dataLeaf : vtk::Range(dataObjectTree, options))
  {
    if (!this->IsSupportedDataSet(dataLeaf))
    {
      vtkDebugMacro("Unsupported block type: " << dataLeaf->GetClassName());
      supportedLeaves = false;
      break;
    }
  }

  return supportedLeaves;
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::SetOriginalDataObject(vtkDataObject* input)
{
  if (!input)
  {
    vtkWarningMacro("Invalid original dataobject: nullptr");
    return;
  }

  if (this->IsSupportedDataSet(input))
  {
    this->OriginalDataSet = vtkDataSet::SafeDownCast(input);
    vtkDebugMacro(" set OriginalDataSet: " << this->OriginalDataSet.GetPointer());
    this->Modified();
    return;
  }
  else if (this->IsSupportedComposite(input))
  {
    this->OriginalCompositeDataSet = vtkCompositeDataSet::SafeDownCast(input);
    vtkDebugMacro(" set OriginalCompositeDataSet: " << this->OriginalCompositeDataSet.GetPointer());
    this->Modified();
    return;
  }

  if (vtkCompositeDataSet::SafeDownCast(input))
  {
    vtkWarningMacro("Composite " << input->GetClassName() << " has unsupported block(s).");
  }
  else
  {
    vtkWarningMacro("Unsupported input type: " << input->GetClassName());
  }
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::ClearOriginalIds()
{
  this->OriginalIdsName.clear();
  vtkDebugMacro(" clear OriginalIdsName");
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::AddOriginalIds(int attribute, const std::string& name)
{
  if (attribute < 0 || attribute >= vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES)
  {
    vtkWarningMacro("Invalid attribute type: " << attribute);
    return;
  }

  this->OriginalIdsName[attribute] = name;
  vtkDebugMacro(" set OriginalIds: " << attribute << " array name to " << name.c_str());
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::RemoveOriginalIds(int attribute)
{
  if (attribute < 0 || attribute >= vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES)
  {
    vtkWarningMacro("Invalid attribute type: " << attribute);
    return;
  }

  this->OriginalIdsName.erase(attribute);
  vtkDebugMacro(" remove OriginalIdsName: " << attribute);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::UpdateCache(vtkDataObject* output)
{
  if (!output)
  {
    vtkWarningMacro("Cannot update from nullptr");
    return;
  }

  if (!this->IsSupportedDataSet(output) && !this->IsSupportedComposite(output))
  {
    vtkWarningMacro("Cannot update from unsupported data type: " << output->GetClassName());
    return;
  }

  this->Cache.TakeReference(output->NewInstance());
  this->Cache->ShallowCopy(output);
  this->CachedOriginalMeshTime = this->GetOriginalMeshTime();
  this->CachedConsumerTime = this->Consumer->GetMTime();
  vtkDebugMacro(" update Cache: " << this->Cache.GetPointer());
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::InvalidateCache()
{
  this->Cache = nullptr;
  this->CachedOriginalMeshTime = 0;
  this->CachedConsumerTime = 0;
  vtkDebugMacro(" invalidate Cache");
  this->Modified();
}

//------------------------------------------------------------------------------
vtkMTimeType vtkDataObjectMeshCache::GetMeshTime(vtkDataSet* dataset) const
{
  auto polydata = vtkPolyData::SafeDownCast(dataset);
  if (polydata)
  {
    return polydata->GetMeshMTime();
  }
  auto ugrid = vtkUnstructuredGrid::SafeDownCast(dataset);
  if (ugrid)
  {
    return ugrid->GetMeshMTime();
  }

  return 0;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkDataObjectMeshCache::GetOriginalMeshTime() const
{
  if (this->OriginalDataSet)
  {
    return this->GetMeshTime(this->OriginalDataSet);
  }

  if (this->OriginalCompositeDataSet)
  {
    return this->GetOriginalCompositeMaxMeshTime();
  }

  return 0;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkDataObjectMeshCache::GetOriginalCompositeMaxMeshTime() const
{
  vtkMTimeType maxTime = 0;
  auto outputDataTree = vtkDataObjectTree::SafeDownCast(this->OriginalCompositeDataSet);
  if (!outputDataTree)
  {
    return 0;
  }

  auto options =
    vtk::DataObjectTreeOptions::TraverseSubTree | vtk::DataObjectTreeOptions::VisitOnlyLeaves;
  for (auto dataLeaf : vtk::Range(outputDataTree, options))
  {
    auto dataSet = vtkDataSet::SafeDownCast(dataLeaf.GetDataObject());
    maxTime = std::max(maxTime, this->GetMeshTime(dataSet));
  }

  return maxTime;
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::Status::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "OriginalDataDefined: " << this->OriginalDataDefined << "\n";
  os << indent << "ConsumerDefined: " << this->ConsumerDefined << "\n";
  os << indent << "CacheDefined: " << this->CacheDefined << "\n";
  os << indent << "DependeciesUnmodified: " << this->OriginalMeshUnmodified << "\n";
  os << indent << "ConsumerUnmodified: " << this->ConsumerUnmodified << "\n";
  os << indent << "AttributesIdsDefined: " << this->AttributesIdsExists << "\n";
}

//------------------------------------------------------------------------------
vtkDataObjectMeshCache::Status vtkDataObjectMeshCache::GetStatus() const
{
  Status status;
  status.OriginalDataDefined =
    this->OriginalDataSet != nullptr || this->OriginalCompositeDataSet != nullptr;
  if (!status.OriginalDataDefined)
  {
    vtkDebugMacro("OriginalDataObject is not set.");
  }

  status.ConsumerDefined = this->Consumer != nullptr;
  if (!status.ConsumerDefined)
  {
    vtkDebugMacro("Consumer is nullptr.");
  }

  status.CacheDefined = this->Cache != nullptr;
  if (!status.CacheDefined)
  {
    vtkDebugMacro("Cache is uninitialized.");
    return status;
  }

  status.ConsumerUnmodified = this->Consumer->GetMTime() <= this->CachedConsumerTime;
  if (!status.ConsumerUnmodified)
  {
    vtkDebugMacro("Consumer modification time has changed.");
  }

  status.OriginalMeshUnmodified = this->GetOriginalMeshTime() <= this->CachedOriginalMeshTime;
  if (!status.OriginalMeshUnmodified)
  {
    vtkDebugMacro("Input mesh time has changed.");
  }

  status.AttributesIdsExists = this->CacheHasRequestedIds();
  if (!status.AttributesIdsExists)
  {
    vtkDebugMacro("Cache does not have requested ids");
  }

  vtkDebugMacro(" returning status");
  return status;
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::CopyCacheToDataObject(vtkDataObject* output)
{
  if (!output)
  {
    vtkWarningMacro("Cannot copy to nullptr");
    return;
  }
  if (!this->Cache)
  {
    vtkWarningMacro("Cannot copy from nullptr");
    return;
  }
  if (!this->IsSupportedDataSet(output) && !this->IsSupportedComposite(output))
  {
    vtkWarningMacro("Cannot copy to unsupported data type: " << output->GetClassName());
    return;
  }

  vtkDebugMacro(" copy Cache to data object");
  output->ShallowCopy(this->Cache);

  if (this->OriginalDataSet)
  {
    auto outputDataSet = vtkDataSet::SafeDownCast(output);
    auto cacheDataSet = vtkDataSet::SafeDownCast(this->Cache);
    this->ForwardAttributesToDataSet(this->OriginalDataSet, cacheDataSet, outputDataSet);
  }
  else if (this->OriginalCompositeDataSet)
  {
    auto outputComposite = vtkCompositeDataSet::SafeDownCast(output);
    this->ForwardAttributesToComposite(outputComposite);
  }
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::ForwardAttributesToDataSet(
  vtkDataSet* input, vtkDataSet* cache, vtkDataSet* outputDataSet)
{
  if (!input || !cache)
  {
    return;
  }

  for (const auto& attribute : this->OriginalIdsName)
  {
    this->ForwardAttributes(input, cache, outputDataSet, attribute.first, attribute.second);
  }

  outputDataSet->GetFieldData()->PassData(input->GetFieldData());
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::ForwardAttributesToComposite(vtkCompositeDataSet* output)
{
  auto inputDataTree = vtkDataObjectTree::SafeDownCast(this->OriginalCompositeDataSet);
  auto outputDataTree = vtkDataObjectTree::SafeDownCast(output);
  auto cacheDataTree = vtkDataObjectTree::SafeDownCast(this->Cache);

  if (!inputDataTree || !outputDataTree || !cacheDataTree)
  {
    vtkWarningMacro("Only vtkDataObjectTree are supported for now");
    return;
  }

  auto options = vtk::DataObjectTreeOptions::TraverseSubTree |
    vtk::DataObjectTreeOptions::SkipEmptyNodes | vtk::DataObjectTreeOptions::VisitOnlyLeaves;
  auto inputDataRange = vtk::Range(inputDataTree, options);
  auto outputDataRange = vtk::Range(outputDataTree, options);
  auto cacheDataRange = vtk::Range(cacheDataTree, options);

  auto inputBlock = inputDataRange.begin();
  auto outputBlock = outputDataRange.begin();
  auto cacheBlock = cacheDataRange.begin();
  while (outputBlock != outputDataRange.end() && inputBlock != inputDataRange.end() &&
    cacheBlock != cacheDataRange.end())
  {
    auto inputDataSet = vtkDataSet::SafeDownCast(*inputBlock);
    auto outputDataSet = vtkDataSet::SafeDownCast(*outputBlock);
    auto cacheDataSet = vtkDataSet::SafeDownCast(*cacheBlock);
    if (outputDataSet && cacheDataSet)
    {
      this->ForwardAttributesToDataSet(inputDataSet, cacheDataSet, outputDataSet);
    }

    cacheBlock++;
    outputBlock++;
    inputBlock++;
  }

  outputDataTree->GetFieldData()->PassData(inputDataTree->GetFieldData());
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::ForwardAttributes(
  vtkDataSet* input, vtkDataSet* cache, vtkDataSet* output, int attribute, const std::string& name)
{
  vtkDebugMacro("Forward attribute " << vtkDataObject::GetAssociationTypeAsString(attribute));

  auto inAttribute = input->GetAttributes(attribute);
  auto outAttribute = output->GetAttributes(attribute);
  auto cacheAttribute = cache->GetAttributes(attribute);

  auto originalIds = cacheAttribute->GetArray(name.c_str());
  if (!originalIds)
  {
    vtkDebugMacro(
      "Global Ids not found for " << vtkDataObject::GetAssociationTypeAsString(attribute));
    return;
  }

  outAttribute->CopyAllOn();
  outAttribute->CopyAllocate(inAttribute);

  // NOTE potential optimization:
  // this copy may be replaced by an (optional ?) use of the implicit vtkIndexedArray
  auto ptsIdsRange = vtk::DataArrayValueRange(originalIds);
  vtkIdType outId = 0;
  for (auto originalId : ptsIdsRange)
  {
    outAttribute->CopyData(inAttribute, originalId, outId);
    outId++;
  }
}

//------------------------------------------------------------------------------
bool vtkDataObjectMeshCache::HasRequestedIds(vtkDataObject* dataobject) const
{
  {
  }
}

//------------------------------------------------------------------------------
bool vtkDataObjectMeshCache::CacheHasRequestedIds() const
{
  if (this->AttributeTypes.empty())
  {
    return true;
  }

  if (this->OriginalDataSet)
  {
    return this->HasRequestedIds(this->Cache);
  }
  else if (this->OriginalCompositeDataSet)
  {
    auto cacheDataTree = vtkDataObjectTree::SafeDownCast(this->Cache);
    auto options =
      vtk::DataObjectTreeOptions::TraverseSubTree | vtk::DataObjectTreeOptions::VisitOnlyLeaves;
    auto cacheDataRange = vtk::Range(cacheDataTree, options);
    for (auto leafDataObject : cacheDataRange)
    {
      if (!this->HasRequestedIds(leafDataObject))
      {
        return false;
      }
    }

    return true;
  }

  return true;
}

VTK_ABI_NAMESPACE_END
