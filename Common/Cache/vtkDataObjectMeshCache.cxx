// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataObjectMeshCache.h"

#include "vtkAffineArray.h"
#include "vtkAlgorithm.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkIndexedArray.h"
#include "vtkLogger.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkSetGet.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDataObjectMeshCache);

/**
 * Add a Log entry only if this->Debug is On.
 *
 * We do not use vtkDebugMacro because we want to have logs
 * even with release build (we are in performance-oriented code)
 * We use this->Debug to control log because with time lot
 * of Cache instances can be used around a program.
 */
#define vtkCacheLog(_verbosity, _msg)                                                              \
  vtkLogIf(_verbosity, this->Debug, << this->GetObjectDescription() << " " << _msg)

namespace
{
constexpr const char* DEFAULT_ORIGINAL_IDS = "__original_ids__";

//----------------------------------------------------------------------------
void AddTemporaryIds(vtkDataSetAttributes* attributes, vtkIdType size)
{
  vtkNew<vtkAffineArray<vtkIdType>> ids;
  ids->SetBackend(std::make_shared<vtkAffineImplicitBackend<vtkIdType>>(1, 0));
  ids->SetNumberOfTuples(size);
  ids->SetName(::DEFAULT_ORIGINAL_IDS);
  attributes->AddArray(ids);
}

}

/**
 * Interface to dispatch work over every contained vtkDataSet.
 * If input is a vtkDataSet subclass, forward it directly to
 * ComputeDataSet.
 * If input is a vtkDataObjectTree subclass, iterate over
 * inner non empty vtkDataSet leaves.
 */
struct GenericDataObjectWorker
{
  virtual ~GenericDataObjectWorker() = default;

  /**
   * Entry point. In the end, call ComputeDataSet for every
   * contained vtkDataSet.
   */
  void Compute(vtkDataObject* dataobject)
  {
    this->SkippedData = false;
    auto dataset = vtkDataSet::SafeDownCast(dataobject);
    if (dataset)
    {
      this->ComputeDataSet(dataset);
      return;
    }
    auto composite = vtkDataObjectTree::SafeDownCast(dataobject);
    if (composite)
    {
      this->ComputeComposite(composite);
      return;
    }

    this->SkippedData = true;
  }

  /**
   * Iterate over inner vtkDataSet to call ComputeDataSet
   */
  void ComputeComposite(vtkDataObjectTree* composite)
  {
    auto options = vtk::DataObjectTreeOptions::TraverseSubTree |
      vtk::DataObjectTreeOptions::SkipEmptyNodes | vtk::DataObjectTreeOptions::VisitOnlyLeaves;
    for (auto dataLeaf : vtk::Range(composite, options))
    {
      this->CurrentFlatIndex = dataLeaf.GetFlatIndex();
      auto dataset = vtkDataSet::SafeDownCast(dataLeaf);
      if (dataset)
      {
        this->ComputeDataSet(dataset);
      }
      else
      {
        this->SkippedData = true;
      }
    }
  }

  /**
   * To be reimplemented to do the actual work.
   * Will be called multiple times for composite.
   */
  virtual void ComputeDataSet(vtkDataSet* dataset) = 0;

  bool SkippedData = false;
  unsigned int CurrentFlatIndex = 0;
};

/**
 * Worker to compute mesh mtime.
 * For composite, return the max value.
 */
struct MeshMTimeWorker : public GenericDataObjectWorker
{
  ~MeshMTimeWorker() override = default;

  void ComputeDataSet(vtkDataSet* dataset) override
  {
    auto leafTime = dataset->GetMeshMTime();
    this->MaxMeshTime = std::max(this->MaxMeshTime, leafTime);
    this->MeshesTimes[this->CurrentFlatIndex] = leafTime;
  }

  vtkMTimeType MaxMeshTime = 0;
  std::map<unsigned int, vtkMTimeType> MeshesTimes;
};

/**
 * Worker to verify if data is supported.
 * If input is not a dataset, Supported() will return false.
 * If any inner dataset is unsupported, Supported() will return false.
 * Otherwise, Supported() return true;
 */
struct SupportedDataWorker : public GenericDataObjectWorker
{
  ~SupportedDataWorker() override = default;

  bool Supported() { return !this->SkippedData; }

  void ComputeDataSet(vtkDataSet* vtkNotUsed(dataset)) override {}
};

/**
 * Worker to verify that data has requested arrays.
 * OriginalIdsName is the map of requested array names per attribute type.
 * HasRequestedIds is set to false if a requested array is not found.
 */
struct RequestedIdsWorker : public GenericDataObjectWorker
{
  ~RequestedIdsWorker() override = default;

  void ComputeDataSet(vtkDataSet* dataset) override
  {
    for (const auto& attribute : this->OriginalIdsName)
    {
      vtkDataSetAttributes* field = dataset->GetAttributes(attribute.first);
      if (!field)
      {
        this->HasRequestedIds = false;
        continue;
      }

      if (!field->GetArray(attribute.second.c_str()))
      {
        this->HasRequestedIds = false;
      }
    }
  }

  std::map<int, std::string> OriginalIdsName;
  bool HasRequestedIds = true;
};

/**
 * Worker to clear dataset attributes from data.
 */
struct ClearAttributesWorker : public GenericDataObjectWorker
{
  ~ClearAttributesWorker() override = default;
  ClearAttributesWorker(const std::set<std::string>& arrays)
    : PreservedArrays(arrays)
  {
  }

  void ComputeDataSet(vtkDataSet* dataset) override
  {
    for (int attribute = vtkDataObject::POINT; attribute < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES;
         attribute++)
    {
      vtkFieldData* field = dataset->GetAttributesAsFieldData(attribute);
      if (field)
      {
        vtkSmartPointer<vtkFieldData> newField = vtk::TakeSmartPointer(field->NewInstance());
        // enforce ALLCOPY: this is the default for DataSetAttributes but not for FieldData
        // leading to unexpected behavior.
        newField->CopyAllOff(vtkDataSetAttributes::ALLCOPY);
        for (const auto& name : this->PreservedArrays)
        {
          newField->CopyFieldOn(name.c_str());
        }
        newField->PassData(field);
        field->Initialize();
        field->PassData(newField);
      }
    }
  }

  const std::set<std::string>& PreservedArrays;
};

struct AddTemporaryIdsWorker : public GenericDataObjectWorker
{
  ~AddTemporaryIdsWorker() override = default;

  void ComputeDataSet(vtkDataSet* leafDataSet) override
  {
    ::AddTemporaryIds(leafDataSet->GetPointData(), leafDataSet->GetNumberOfPoints());
    ::AddTemporaryIds(leafDataSet->GetCellData(), leafDataSet->GetNumberOfCells());
  }
};

struct RemoveTemporaryIdsWorker : public GenericDataObjectWorker
{
  ~RemoveTemporaryIdsWorker() override = default;

  void ComputeDataSet(vtkDataSet* leafDataSet) override
  {
    leafDataSet->GetPointData()->RemoveArray(::DEFAULT_ORIGINAL_IDS);
    leafDataSet->GetCellData()->RemoveArray(::DEFAULT_ORIGINAL_IDS);
  }
};

/**
 * Worker to count number of datasets.
 */
struct NumberOfDataSetWorker : public GenericDataObjectWorker
{
  ~NumberOfDataSetWorker() override = default;

  void ComputeDataSet(vtkDataSet* vtkNotUsed(dataset)) override { this->NumberOfDataSets++; }
  vtkIdType NumberOfDataSets = 0;
};

//------------------------------------------------------------------------------
std::string vtkDataObjectMeshCache::GetDefaultIdsName()
{
  return ::DEFAULT_ORIGINAL_IDS;
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Consumer: ";
  if (this->Consumer)
  {
    os << "\n";
    this->Consumer->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "OriginalDataObject: ";
  if (this->GetOriginalDataObject())
  {
    os << "\n";
    this->GetOriginalDataObject()->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

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

  os << indent << "CachedConsumerTime: " << this->CachedConsumerTime << "\n";
  os << indent << "CachedOriginalLeavesTime:\n";
  for (const auto& time : this->CachedOriginalLeavesTime)
  {
    os << indent.GetNextIndent() << time.first << " " << time.second << "\n";
  }

  os << indent << "OriginalIdsName:\n";
  for (const auto& attribute : this->OriginalIdsName)
  {
    os << indent.GetNextIndent() << vtkDataObject::GetAssociationTypeAsString(attribute.first)
       << " " << attribute.second << "\n";
  }

  os << indent << "PreservedInputAttributes:\n";
  for (const auto& attribute : this->PreservedInputAttributes)
  {
    os << indent.GetNextIndent() << vtkDataObject::GetAssociationTypeAsString(attribute) << "\n";
  }

  os << indent << "PreservedOutputArrays:\n";
  for (const auto& array : this->PreservedCachedArrays)
  {
    os << indent.GetNextIndent() << array << "\n";
  }

  Status status = this->GetStatus();
  os << indent << "Status\n:";
  status.PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
bool vtkDataObjectMeshCache::IsSupportedData(vtkDataObject* dataobject) const
{
  SupportedDataWorker supportWorker;
  supportWorker.Compute(dataobject);

  vtkCacheLog(INFO, "Return IsSupportedData: " << supportWorker.Supported());
  return supportWorker.Supported();
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::SetOriginalDataObject(vtkDataObject* input)
{
  if (!input)
  {
    vtkWarningMacro("Invalid original dataobject: nullptr");
    return;
  }

  if (this->IsSupportedData(input))
  {
    if (this->OriginalCompositeDataSet &&
      strcmp(this->OriginalCompositeDataSet->GetClassName(), input->GetClassName()) == 0)
    {
      this->InvalidateCache();
    }

    this->OriginalDataSet = vtkDataSet::SafeDownCast(input);
    this->OriginalCompositeDataSet = vtkCompositeDataSet::SafeDownCast(input);
    vtkCacheLog(INFO, "Set OriginalDataObject: " << input);
    this->Modified();
    return;
  }

  // Clear existing dataset ptrs
  this->OriginalCompositeDataSet = nullptr;
  this->OriginalDataSet = nullptr;

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
  vtkCacheLog(INFO, "Clear OriginalIdsName");
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

  if (this->OriginalIdsName[attribute] != name)
  {
    this->OriginalIdsName[attribute] = name;
    vtkCacheLog(INFO, "Set OriginalIds: " << attribute << " array name to " << name.c_str());
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::ForwardAttribute(int attribute)
{
  this->AddOriginalIds(attribute, ::DEFAULT_ORIGINAL_IDS);
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
  vtkCacheLog(INFO, "Remove OriginalIdsName: " << attribute);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkDataObjectMeshCache::CreateTemporaryOriginalIdsArrays(vtkDataObject* object)
{
  AddTemporaryIdsWorker worker;
  worker.Compute(object);
}

//----------------------------------------------------------------------------
void vtkDataObjectMeshCache::CleanupTemporaryOriginalIds(vtkDataObject* object)
{
  RemoveTemporaryIdsWorker worker;
  worker.Compute(object);
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::UpdateCache(vtkDataObject* output)
{
  if (!output)
  {
    vtkWarningMacro("Cannot update from nullptr");
    return;
  }

  bool isSupported = this->IsSupportedData(output);
  if (!isSupported)
  {
    vtkWarningMacro("Cannot update from unsupported data type: " << output->GetClassName());
    return;
  }

  this->Cache.TakeReference(output->NewInstance());
  this->Cache->ShallowCopy(output);
  this->CachedOriginalLeavesTime =
    vtkDataObjectMeshCache::GetDataObjectMeshMTimes(this->GetOriginalDataObject());
  this->CachedConsumerTime = this->Consumer->GetMTime();

  vtkCacheLog(INFO, "Update Cache: " << this->Cache.GetPointer());
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::InvalidateCache()
{
  this->Cache = nullptr;
  this->CachedOriginalLeavesTime.clear();
  this->CachedConsumerTime = 0;
  vtkCacheLog(INFO, "Invalidate Cache");
  this->Modified();
}

//------------------------------------------------------------------------------
vtkIdType vtkDataObjectMeshCache::GetNumberOfDataSets(vtkDataObject* dataobject) const
{
  NumberOfDataSetWorker countWorker;
  countWorker.Compute(dataobject);
  return countWorker.NumberOfDataSets;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkDataObjectMeshCache::GetOriginalMeshTime() const
{
  return vtkDataObjectMeshCache::GetDataObjectMeshMTime(this->GetOriginalDataObject());
}

//------------------------------------------------------------------------------
vtkDataObject* vtkDataObjectMeshCache::GetOriginalDataObject() const
{
  if (this->OriginalDataSet)
  {
    return this->OriginalDataSet;
  }

  if (this->OriginalCompositeDataSet)
  {
    return this->OriginalCompositeDataSet;
  }

  return nullptr;
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::Status::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "OriginalDataDefined: " << this->OriginalDataDefined << "\n";
  os << indent << "ConsumerDefined: " << this->ConsumerDefined << "\n";
  os << indent << "CacheDefined: " << this->CacheDefined << "\n";
  os << indent << "OriginalMeshUnmodified: " << this->OriginalMeshUnmodified << "\n";
  os << indent << "ConsumerUnmodified: " << this->ConsumerUnmodified << "\n";
  os << indent << "AttributesIdsExists: " << this->AttributesIdsExists << "\n";
}

//------------------------------------------------------------------------------
vtkDataObjectMeshCache::Status vtkDataObjectMeshCache::GetStatus() const
{
  Status status;
  status.OriginalDataDefined =
    this->OriginalDataSet != nullptr || this->OriginalCompositeDataSet != nullptr;
  if (!status.OriginalDataDefined)
  {
    vtkCacheLog(INFO, "OriginalDataObject is not set.");
  }

  status.ConsumerDefined = this->Consumer != nullptr;
  if (!status.ConsumerDefined)
  {
    vtkCacheLog(INFO, "Consumer is nullptr.");
  }

  status.CacheDefined = this->Cache != nullptr;
  if (!status.CacheDefined)
  {
    vtkCacheLog(INFO, "Cache is uninitialized.");
    return status;
  }

  status.ConsumerUnmodified = this->Consumer->GetMTime() <= this->CachedConsumerTime;
  if (!status.ConsumerUnmodified)
  {
    vtkCacheLog(INFO, "Consumer modification time has changed.");
  }

  auto originalMeshesTime =
    vtkDataObjectMeshCache::GetDataObjectMeshMTimes(this->GetOriginalDataObject());

  status.OriginalMeshUnmodified = this->CachedOriginalLeavesTime == originalMeshesTime;
  if (!status.OriginalMeshUnmodified)
  {
    vtkCacheLog(INFO, "Input mesh time has changed.");
  }

  status.AttributesIdsExists = this->CacheHasRequestedIds();
  if (!status.AttributesIdsExists)
  {
    vtkCacheLog(INFO, "Cache does not have requested ids");
  }

  vtkCacheLog(INFO, "Returning status");
  return status;
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::AddPreservedCachedArray(const std::string& arrayName)
{
  this->PreservedCachedArrays.insert(arrayName);
  vtkCacheLog(INFO, "AddPreserveArray: " << arrayName);
  this->Modified();
}

//------------------------------------------------------------------------------
std::set<std::string> vtkDataObjectMeshCache::GetPreservedCachedArrays()
{
  return this->PreservedCachedArrays;
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::ClearPreservedCachedArray()
{
  this->PreservedCachedArrays.clear();
  vtkCacheLog(INFO, "ClearPreservedCachedArray");
  this->Modified();
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

  if (!this->IsSupportedData(output))
  {
    vtkWarningMacro("Cannot copy to unsupported data type: " << output->GetClassName());
    return;
  }
  vtkSmartPointer<vtkDataObject> input = nullptr;
  if (this->OriginalDataSet)
  {
    input = this->OriginalDataSet.Get();
  }
  else if (this->OriginalCompositeDataSet)
  {
    input = this->OriginalCompositeDataSet.Get();
  }

  vtkCacheLog(INFO, " Copy Cache to data object");
  output->ShallowCopy(this->Cache);
  this->ClearAttributes(output);

  auto outputDataSet = vtkDataSet::SafeDownCast(output);
  auto outputComposite = vtkCompositeDataSet::SafeDownCast(output);
  if (outputDataSet)
  {
    auto cacheDataSet = vtkDataSet::SafeDownCast(this->Cache);
    auto inputDataSet = vtkDataSet::SafeDownCast(input);
    this->ForwardAttributesToDataSet(inputDataSet, cacheDataSet, outputDataSet);
  }
  else if (outputComposite)
  {
    auto inputComposite = vtkCompositeDataSet::SafeDownCast(input);
    this->ForwardAttributesToComposite(inputComposite, outputComposite);
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
void vtkDataObjectMeshCache::ForwardAttributesToComposite(
  vtkCompositeDataSet* input, vtkCompositeDataSet* output)
{
  auto inputDataTree = vtkDataObjectTree::SafeDownCast(input);
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
void vtkDataObjectMeshCache::AddPreservedInputAttributes(int attr)
{
  this->PreservedInputAttributes.insert(attr);
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::ClearPreservedInputAttributes()
{
  this->PreservedInputAttributes.clear();
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::PreservedInputAllAttributes()
{
  for (int attribute = vtkDataObject::POINT; attribute < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES;
       attribute++)
  {
    this->PreservedInputAttributes.insert(attribute);
  }
}

//------------------------------------------------------------------------------
std::set<int> vtkDataObjectMeshCache::GetPreservedInputAttributes()
{
  return this->PreservedInputAttributes;
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::ForwardAttributes(
  vtkDataSet* input, vtkDataSet* cache, vtkDataSet* output, int attribute, const std::string& name)
{
  vtkCacheLog(INFO, "Forward attribute " << vtkDataObject::GetAssociationTypeAsString(attribute));

  auto inAttribute = input->GetAttributes(attribute);
  auto outAttribute = output->GetAttributes(attribute);
  auto cacheAttribute = cache->GetAttributes(attribute);

  if (this->PreservedInputAttributes.count(attribute) > 0)
  {
    vtkCacheLog(INFO, "ShallowCopy Attribute");
    outAttribute->DeepCopy(inAttribute);
    return;
  }

  auto originalIds = cacheAttribute->GetArray(name.c_str());
  if (!originalIds)
  {
    vtkCacheLog(
      INFO, "Global Ids not found for " << vtkDataObject::GetAssociationTypeAsString(attribute));
    return;
  }

  outAttribute->CopyAllOn();
  outAttribute->CopyAllocate(inAttribute);
  outAttribute->SetNumberOfTuples(cacheAttribute->GetNumberOfTuples());

  // NOTE potential optimization:
  // this copy may be replaced by an use of SMPTools (or optionally the implicit vtkIndexedArray?)
  auto ptsIdsRange = vtk::DataArrayValueRange(originalIds);
  vtkSMPTools::For(0, outAttribute->GetNumberOfTuples(),
    [&](vtkIdType startId, vtkIdType endId)
    {
      for (auto id = startId; id < endId; id++)
      {
        outAttribute->CopyData(inAttribute, ptsIdsRange[id], id);
      }
    });
}

//------------------------------------------------------------------------------
bool vtkDataObjectMeshCache::CacheHasRequestedIds() const
{
  if (this->OriginalIdsName.empty())
  {
    return true;
  }

  RequestedIdsWorker idsWorker;
  idsWorker.OriginalIdsName = this->OriginalIdsName;
  idsWorker.Compute(this->Cache);
  return idsWorker.HasRequestedIds;
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::ClearAttributes(vtkDataObject* dataobject)
{
  ClearAttributesWorker clearWorker{ this->PreservedCachedArrays };
  clearWorker.Compute(dataobject);
}

//------------------------------------------------------------------------------
void vtkDataObjectMeshCache::SetConsumer(vtkAlgorithm* consumer)
{
  vtkSetSmartPointerBodyMacro(Consumer, vtkAlgorithm, consumer);
}

//------------------------------------------------------------------------------
bool vtkDataObjectMeshCache::HasConsumerNoInputPort() const
{
  return false;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkDataObjectMeshCache::GetDataObjectMeshMTime(vtkDataObject* object)
{
  MeshMTimeWorker meshtime;
  meshtime.Compute(object);
  return meshtime.MaxMeshTime;
}

//------------------------------------------------------------------------------
std::map<unsigned int, vtkMTimeType> vtkDataObjectMeshCache::GetDataObjectMeshMTimes(
  vtkDataObject* object)
{
  MeshMTimeWorker meshtime;
  meshtime.Compute(object);
  return meshtime.MeshesTimes;
}

VTK_ABI_NAMESPACE_END
