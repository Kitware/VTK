// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIOSSModel.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDummyController.h"
#include "vtkIOSSUtilities.h"
#include "vtkIOSSWriter.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <vtksys/MD5.h>

// Ioss includes
#include <vtk_ioss.h>
// clang-format off
#include VTK_IOSS(Ioss_Assembly.h)
#include VTK_IOSS(Ioss_CodeTypes.h)
#include VTK_IOSS(Ioss_DatabaseIO.h)
#include VTK_IOSS(Ioss_EdgeBlock.h)
#include VTK_IOSS(Ioss_EdgeSet.h)
#include VTK_IOSS(Ioss_ElementBlock.h)
#include VTK_IOSS(Ioss_ElementSet.h)
#include VTK_IOSS(Ioss_ElementTopology.h)
#include VTK_IOSS(Ioss_FaceBlock.h)
#include VTK_IOSS(Ioss_FaceSet.h)
#include VTK_IOSS(Ioss_IOFactory.h)
#include VTK_IOSS(Ioss_NodeBlock.h)
#include VTK_IOSS(Ioss_NodeSet.h)
#include VTK_IOSS(Ioss_Region.h)
#include VTK_IOSS(Ioss_SideBlock.h)
#include VTK_IOSS(Ioss_SideSet.h)
#include VTK_IOSS(Ioss_StructuredBlock.h)
// clang-format on

#include <map>
#include <numeric>
#include <set>
#include <unordered_map>

namespace
{
constexpr char VTK_IOSS_MODEL_OLD_GLOBAL_IDS_ARRAY_NAME[] = "__vtk_old_global_ids__";
constexpr char VTK_IOSS_MODEL_GLOBAL_IDS_ARRAY_NAME[] = "__vtk_global_ids__";
constexpr char VTK_IOSS_MODEL_OLD_ELEMENT_SIDE_ARRAY_NAME[] = "__vtk_old_element_side__";

/**
 * Struct to store information to handle errors for global IDs and element_side.
 */
struct ErrorHandleInformation
{
  bool NeedToBeCreated;
  bool Created;
  bool NeedToBeModified;
  bool Modified;

  ErrorHandleInformation()
    : NeedToBeCreated(false)
    , Created(false)
    , NeedToBeModified(false)
    , Modified(false)
  {
  }

  bool GetCreated() const { return this->Created; }

  bool GetModified() const { return this->Modified; }

  bool GetCouldNotBeCreated() const { return this->NeedToBeCreated != this->Created; }

  bool GetCouldNotBeModified() const { return this->NeedToBeModified != this->Modified; }

  bool GetHadIssues() const
  {
    return this->GetCouldNotBeCreated() || this->GetCouldNotBeModified();
  }

  void PrintSelf(ostream& os, vtkIndent indent)
  {
    os << indent << "NeedToBeCreated: " << this->NeedToBeCreated << std::endl;
    os << indent << "Created: " << this->Created << std::endl;
    os << indent << "NeedToBeModified: " << this->NeedToBeModified << std::endl;
    os << indent << "Modified: " << this->Modified << std::endl;
  }
};

//=============================================================================
ErrorHandleInformation HandleGlobalIds(vtkPartitionedDataSetCollection* pdc, int association,
  std::set<unsigned int> indicesToIgnore, vtkMultiProcessController* controller,
  vtkIOSSWriter* writer)
{
  ErrorHandleInformation globalIdsInfo;
  std::vector<std::vector<vtkDataSet*>> datasets(pdc->GetNumberOfPartitionedDataSets());
  for (unsigned int i = 0; i < pdc->GetNumberOfPartitionedDataSets(); ++i)
  {
    datasets[i] = vtkCompositeDataSet::GetDataSets<vtkDataSet>(pdc->GetPartitionedDataSet(i));
  }
  // check if global IDs are present, if they are not present, create them, if they are present and
  // valid, use them, if they are present and invalid, create them, and save the old ones.
  int hasGlobalIds = true;
  for (unsigned int i = 0; i < pdc->GetNumberOfPartitionedDataSets(); ++i)
  {
    if (indicesToIgnore.find(i) != indicesToIgnore.end())
    {
      continue;
    }
    for (auto& ds : datasets[i])
    {
      if (ds->GetNumberOfElements(association) == 0)
      {
        continue;
      }
      auto* gids = vtkIdTypeArray::SafeDownCast(ds->GetAttributes(association)->GetGlobalIds());
      if (!gids)
      {
        hasGlobalIds = false;
        break;
      }
    }
  }
  if (controller->GetNumberOfProcesses() > 1)
  {
    int globalHasGlobalIds;
    controller->AllReduce(&hasGlobalIds, &globalHasGlobalIds, 1, vtkCommunicator::MIN_OP);
    hasGlobalIds = globalHasGlobalIds;
  }
  globalIdsInfo.NeedToBeCreated = !hasGlobalIds;

  // check if global IDs are valid
  bool hasValidGlobalIds = true;
  if (hasGlobalIds)
  {
    vtkIdType maxGlobalId = 0;
    vtkIdType numElements = 0;
    for (unsigned int i = 0; i < pdc->GetNumberOfPartitionedDataSets(); ++i)
    {
      if (indicesToIgnore.find(i) != indicesToIgnore.end())
      {
        continue;
      }
      for (auto& ds : datasets[i])
      {
        if (ds->GetNumberOfElements(association) == 0)
        {
          continue;
        }
        auto gids = vtkIdTypeArray::SafeDownCast(ds->GetAttributes(association)->GetGlobalIds());
        maxGlobalId = std::max(maxGlobalId, gids->GetValueRange()[1]);
        numElements += ds->GetNumberOfElements(association);
      }
    }
    if (controller->GetNumberOfProcesses() > 1)
    {
      // find max global id and sum of number of elements.
      vtkIdType globalMaxGlobalId;
      controller->AllReduce(&maxGlobalId, &globalMaxGlobalId, 1, vtkCommunicator::MAX_OP);
      maxGlobalId = globalMaxGlobalId;
      vtkIdType globalNumElements;
      controller->AllReduce(&numElements, &globalNumElements, 1, vtkCommunicator::SUM_OP);
      numElements = globalNumElements;
    }
    if (association == vtkDataObject::POINT)
    {
      // for points the max global id can be less than the numElement since the same points
      // can be used by many blocks
      if (numElements > 0 && maxGlobalId + writer->GetOffsetGlobalIds() > numElements)
      {
        // global IDs are not valid, so we need to recreate them.
        hasValidGlobalIds = false;
      }
    }
    else // if (association == vtkDataObject::Cell)
    {
      if (numElements > 0 && maxGlobalId + writer->GetOffsetGlobalIds() != numElements)
      {
        // global IDs are not valid, so we need to recreate them.
        hasValidGlobalIds = false;
      }
    }
    if (!hasValidGlobalIds)
    {
      // if they are invalid global IDs, we change the name of the existing
      // global IDs to something else so that we can create new ones.
      for (unsigned int i = 0; i < pdc->GetNumberOfPartitionedDataSets(); ++i)
      {
        if (indicesToIgnore.find(i) != indicesToIgnore.end())
        {
          continue;
        }
        for (auto& ds : datasets[i])
        {
          if (ds->GetNumberOfElements(association) == 0)
          {
            continue;
          }
          // save the old global IDs in a new array.
          vtkNew<vtkIdTypeArray> oldGids;
          oldGids->ShallowCopy(
            vtkIdTypeArray::SafeDownCast(ds->GetAttributes(association)->GetGlobalIds()));
          oldGids->SetName(VTK_IOSS_MODEL_OLD_GLOBAL_IDS_ARRAY_NAME);
          // remove the old global IDs.
          ds->GetAttributes(association)->SetGlobalIds(nullptr);
          // add the old global IDs back.
          ds->GetAttributes(association)->AddArray(oldGids);
        }
      }
    }
  }
  globalIdsInfo.NeedToBeModified = !hasValidGlobalIds;
  // create global IDs assuming uniqueness if they are not present or if they are invalid
  if (!hasGlobalIds || !hasValidGlobalIds)
  {
    if (!hasGlobalIds)
    {
      globalIdsInfo.Created = true;
    }
    else // if (!hasValidGlobalIds)
    {
      globalIdsInfo.Modified = true;
    }
    vtkIdType numElements = 0;
    for (unsigned int i = 0; i < pdc->GetNumberOfPartitionedDataSets(); ++i)
    {
      if (indicesToIgnore.find(i) != indicesToIgnore.end())
      {
        continue;
      }
      for (auto& ds : datasets[i])
      {
        numElements += ds->GetNumberOfElements(association);
      }
    }

    vtkIdType startId = 1; // start with 1 since Exodus ids start with 1.
    if (controller->GetNumberOfProcesses() > 1)
    {
      vtkNew<vtkIdTypeArray> sourceNumberOfElements;
      sourceNumberOfElements->InsertNextValue(numElements);
      vtkNew<vtkIdTypeArray> resultNumberOfElementsPerCore;
      controller->AllGatherV(sourceNumberOfElements, resultNumberOfElementsPerCore);

      startId = std::accumulate(resultNumberOfElementsPerCore->GetPointer(0),
        resultNumberOfElementsPerCore->GetPointer(controller->GetLocalProcessId()), startId);
    }
    for (unsigned int i = 0; i < pdc->GetNumberOfPartitionedDataSets(); ++i)
    {
      if (indicesToIgnore.find(i) != indicesToIgnore.end())
      {
        continue;
      }
      for (auto& ds : datasets[i])
      {
        if (ds->GetNumberOfElements(association) == 0)
        {
          continue;
        }
        const auto numberOfElements = ds->GetNumberOfElements(association);
        vtkNew<vtkIdTypeArray> globalIds;
        globalIds->SetName(VTK_IOSS_MODEL_GLOBAL_IDS_ARRAY_NAME);
        globalIds->SetNumberOfComponents(1);
        globalIds->SetNumberOfTuples(numberOfElements);
        vtkSMPTools::For(0, numberOfElements, [&](vtkIdType begin, vtkIdType end) {
          auto globalIdsPtr = globalIds->GetPointer(0);
          for (vtkIdType j = begin; j < end; ++j)
          {
            globalIdsPtr[j] = startId + j;
          }
        });
        ds->GetAttributes(association)->SetGlobalIds(globalIds);
        startId += numberOfElements;
      }
    }
  }

  return globalIdsInfo;
}

//=============================================================================
ErrorHandleInformation HandleElementSide(vtkPartitionedDataSetCollection* pdc,
  ErrorHandleInformation globalIdsInfo, std::set<unsigned int> setIndicesWithElementSide,
  std::set<unsigned int> blockIndices, vtkMultiProcessController* controller, vtkIOSSWriter* writer)
{
  ErrorHandleInformation elementSideInfo;
  std::vector<std::vector<vtkDataSet*>> datasets(pdc->GetNumberOfPartitionedDataSets());
  for (unsigned int i = 0; i < pdc->GetNumberOfPartitionedDataSets(); ++i)
  {
    datasets[i] = vtkCompositeDataSet::GetDataSets<vtkDataSet>(pdc->GetPartitionedDataSet(i));
  }
  // check if element_side is present, if it is not present, stop, if it is present and
  // global IDs have not been modified, use it, if it is present and global IDs have been
  // modified, re-create it using the old global IDs, and save the old one.
  int hasElementSide = true;
  for (unsigned int i = 0; i < pdc->GetNumberOfPartitionedDataSets(); ++i)
  {
    if (setIndicesWithElementSide.find(i) == setIndicesWithElementSide.end())
    {
      continue;
    }
    for (auto& ds : datasets[i])
    {
      if (ds->GetNumberOfCells() == 0)
      {
        continue;
      }
      auto* elementSide = vtkIntArray::SafeDownCast(ds->GetCellData()->GetArray("element_side"));
      if (!elementSide)
      {
        hasElementSide = false;
        break;
      }
    }
  }
  if (controller->GetNumberOfProcesses() > 1)
  {
    int globalHasElementSide;
    controller->AllReduce(&hasElementSide, &globalHasElementSide, 1, vtkCommunicator::MIN_OP);
    hasElementSide = globalHasElementSide;
  }
  elementSideInfo.NeedToBeCreated = !hasElementSide;

  bool hasValidElementSide = true;
  // if element_side is present but global IDs have been created and not modified, element_side
  // must be invalid. It can be a left-over from a filter, e.g. clip.
  if (hasElementSide && globalIdsInfo.GetCreated())
  {
    hasValidElementSide = false;
  }
  // if element_side is present, and global IDs have not been created, we need to try and check
  // if the element_side is valid
  else if (hasElementSide)
  {
    // we check if the maximum element_side id, and if it is greater than the maximum
    // global id
    int maxElementSideId = 0;
    for (unsigned int i = 0; i < pdc->GetNumberOfPartitionedDataSets(); ++i)
    {
      if (setIndicesWithElementSide.find(i) == setIndicesWithElementSide.end())
      {
        continue;
      }
      for (auto& ds : datasets[i])
      {
        if (ds->GetNumberOfCells() == 0)
        {
          continue;
        }
        auto elementSide = vtkIntArray::SafeDownCast(ds->GetCellData()->GetArray("element_side"));
        maxElementSideId = std::max(maxElementSideId, elementSide->GetValueRange(0)[1]);
      }
    }
    if (controller->GetNumberOfProcesses() > 1)
    {
      int globalMaxElementSideId;
      controller->AllReduce(&maxElementSideId, &globalMaxElementSideId, 1, vtkCommunicator::MAX_OP);
      maxElementSideId = globalMaxElementSideId;
    }
    vtkIdType maxGlobalId = 0;
    for (unsigned int i = 0; i < pdc->GetNumberOfPartitionedDataSets(); ++i)
    {
      if (blockIndices.find(i) == blockIndices.end())
      {
        continue;
      }
      for (auto& ds : datasets[i])
      {
        auto globalIds = vtkIdTypeArray::SafeDownCast(ds->GetCellData()->GetGlobalIds());
        maxGlobalId = std::max(maxGlobalId, globalIds->GetValueRange(0)[1]);
      }
    }
    if (controller->GetNumberOfProcesses() > 1)
    {
      vtkIdType globalMaxGlobalId;
      controller->AllReduce(&maxGlobalId, &globalMaxGlobalId, 1, vtkCommunicator::MAX_OP);
      maxGlobalId = globalMaxGlobalId;
    }
    // if the maximum element_side id is greater than the maximum global id, that's an issue
    if (maxElementSideId > maxGlobalId)
    {
      hasValidElementSide = false;
    }
  }
  elementSideInfo.NeedToBeModified = !hasValidElementSide;

  // if element_side is present, and has invalid values, and global IDs have been modified
  // which means that global IDs are guaranteed to be present, we need to re-create it.
  // if possible. It's possible if all element_side ids point to present old global IDs.
  if (hasElementSide && !hasValidElementSide && globalIdsInfo.GetModified())
  {
    // before we recreate the element_side, we change the name of the existing
    // element_side to something else so that we can create new one.
    for (unsigned int i = 0; i < pdc->GetNumberOfPartitionedDataSets(); ++i)
    {
      if (setIndicesWithElementSide.find(i) == setIndicesWithElementSide.end())
      {
        continue;
      }
      for (auto& ds : datasets[i])
      {
        if (ds->GetNumberOfCells() == 0)
        {
          continue;
        }
        // rename the element_side to old element_side.
        vtkIntArray::SafeDownCast(ds->GetCellData()->GetArray("element_side"))
          ->SetName(VTK_IOSS_MODEL_OLD_ELEMENT_SIDE_ARRAY_NAME);
      }
    }
    // we need to create a map from old global IDs to new global IDs.
    std::unordered_map<vtkIdType, vtkIdType> oldToNewGlobalIds;
    for (unsigned int i = 0; i < pdc->GetNumberOfPartitionedDataSets(); ++i)
    {
      if (blockIndices.find(i) == blockIndices.end())
      {
        continue;
      }
      for (auto& ds : datasets[i])
      {
        if (ds->GetNumberOfCells() == 0)
        {
          continue;
        }
        auto* oldGlobalIds = vtkIdTypeArray::SafeDownCast(
          ds->GetCellData()->GetArray(VTK_IOSS_MODEL_OLD_GLOBAL_IDS_ARRAY_NAME));
        auto* globalIds = vtkIdTypeArray::SafeDownCast(ds->GetCellData()->GetGlobalIds());
        for (vtkIdType j = 0; j < oldGlobalIds->GetNumberOfTuples(); ++j)
        {
          oldToNewGlobalIds[oldGlobalIds->GetValue(j)] = globalIds->GetValue(j);
        }
      }
    }

    // check if all old element_side ids point to present old global IDs.
    int hasValidOldElementSide = true;
    for (unsigned int i = 0; i < pdc->GetNumberOfPartitionedDataSets(); ++i)
    {
      if (setIndicesWithElementSide.find(i) == setIndicesWithElementSide.end())
      {
        continue;
      }
      for (auto& ds : datasets[i])
      {
        if (ds->GetNumberOfCells() == 0)
        {
          continue;
        }
        auto* oldElementSide = vtkIntArray::SafeDownCast(
          ds->GetCellData()->GetArray(VTK_IOSS_MODEL_OLD_ELEMENT_SIDE_ARRAY_NAME));
        const auto numTuples = oldElementSide->GetNumberOfTuples();
        std::atomic<int> localHasValidOldElementSide(true);
        vtkSMPTools::For(0, numTuples, [&](vtkIdType begin, vtkIdType end) {
          if (!localHasValidOldElementSide)
          {
            return;
          }
          for (vtkIdType j = begin; j < end; ++j)
          {
            if (oldToNewGlobalIds.find(oldElementSide->GetValue(2 * j)) == oldToNewGlobalIds.end())
            {
              localHasValidOldElementSide = false;
              break;
            }
          }
        });
        if (!localHasValidOldElementSide)
        {
          hasValidOldElementSide &= localHasValidOldElementSide.load();
        }
      }
    }
    if (controller->GetNumberOfProcesses() > 1)
    {
      int globalHasValidOldElementSide;
      controller->AllReduce(
        &hasValidOldElementSide, &globalHasValidOldElementSide, 1, vtkCommunicator::MIN_OP);
      hasValidOldElementSide = globalHasValidOldElementSide;
    }
    // if there are no invalid old element_side ids, we can create the new element_side.
    if (hasValidOldElementSide)
    {
      elementSideInfo.Modified = true;

      // then we create the new element_side.
      for (unsigned int i = 0; i < pdc->GetNumberOfPartitionedDataSets(); ++i)
      {
        if (setIndicesWithElementSide.find(i) == setIndicesWithElementSide.end())
        {
          continue;
        }
        for (auto& ds : datasets[i])
        {
          if (ds->GetNumberOfCells() == 0)
          {
            continue;
          }
          auto* oldElementSide = vtkIntArray::SafeDownCast(
            ds->GetCellData()->GetArray(VTK_IOSS_MODEL_OLD_ELEMENT_SIDE_ARRAY_NAME));
          vtkNew<vtkIntArray> elementSide;
          elementSide->SetName("element_side");
          elementSide->SetNumberOfComponents(oldElementSide->GetNumberOfComponents());
          elementSide->SetNumberOfTuples(oldElementSide->GetNumberOfTuples());
          const auto globalIdOffset = writer->GetOffsetGlobalIds();
          const auto numTuples = oldElementSide->GetNumberOfTuples();
          vtkSMPTools::For(0, numTuples, [&](vtkIdType begin, vtkIdType end) {
            int oldElementSideTuple[2];
            int elementSideTuple[2];
            for (vtkIdType j = begin; j < end; ++j)
            {
              oldElementSide->GetTypedTuple(j, oldElementSideTuple);
              elementSideTuple[0] = oldToNewGlobalIds[oldElementSideTuple[0]] + globalIdOffset;
              elementSideTuple[1] = oldElementSideTuple[1];
              elementSide->SetTypedTuple(j, elementSideTuple);
            }
          });
          ds->GetCellData()->AddArray(elementSide);
        }
      }
    }
  }
  return elementSideInfo;
}

//=============================================================================
std::set<unsigned int> GetDatasetIndices(vtkDataAssembly* assembly, std::set<std::string> paths)
{
  if (assembly && assembly->GetRootNodeName())
  {
    std::vector<int> indices;
    for (const auto& path : paths)
    {
      const auto idx = assembly->GetFirstNodeByPath(path.c_str());
      if (idx != -1)
      {
        indices.push_back(idx);
      }
    }
    const auto vector = assembly->GetDataSetIndices(indices);
    return std::set<unsigned int>{ vector.begin(), vector.end() };
  }
  return {};
}

//=============================================================================
std::map<unsigned char, int64_t> GetElementCounts(
  vtkPartitionedDataSet* pd, vtkMultiProcessController* controller)
{
  std::set<unsigned char> cellTypes;
  auto datasets = vtkCompositeDataSet::GetDataSets<vtkDataSet>(pd);
  for (auto& ds : datasets)
  {
    switch (ds->GetDataObjectType())
    {
      case VTK_UNSTRUCTURED_GRID:
      {
        auto ug = vtkUnstructuredGrid::SafeDownCast(ds);
        auto distinctCellTypes = ug->GetDistinctCellTypesArray();
        auto range = vtk::DataArrayValueRange(distinctCellTypes);
        std::copy(range.begin(), range.end(), std::inserter(cellTypes, cellTypes.end()));
        break;
      }
      case VTK_POLY_DATA:
      case VTK_UNSTRUCTURED_GRID_BASE:
      {
        vtkNew<vtkCellTypes> cellTypesOfUnstructuredData;
        ds->GetCellTypes(cellTypesOfUnstructuredData);
        auto range = vtk::DataArrayValueRange(cellTypesOfUnstructuredData->GetCellTypesArray());
        std::copy(range.begin(), range.end(), std::inserter(cellTypes, cellTypes.end()));
        break;
      }
      case VTK_IMAGE_DATA:
      case VTK_STRUCTURED_POINTS:
      case VTK_UNIFORM_GRID:
      case VTK_RECTILINEAR_GRID:
      case VTK_STRUCTURED_GRID:
      case VTK_EXPLICIT_STRUCTURED_GRID:
      {
        if (ds->GetNumberOfCells() > 0)
        {
          cellTypes.insert(ds->GetCellType(0));
          // this is added in case there is an empty cell.
          if (ds->GetCellGhostArray())
          {
            cellTypes.insert(VTK_EMPTY_CELL);
          }
        }
        break;
      }
      default:
        vtkLogF(ERROR, "Unsupported data set type: %s", ds->GetClassName());
        break;
    }
  }

  // now reduce this across all ranks as well.
  if (controller->GetNumberOfProcesses() > 1)
  {
    vtkNew<vtkUnsignedCharArray> source;
    source->SetNumberOfTuples(cellTypes.size());
    std::copy(cellTypes.begin(), cellTypes.end(), source->GetPointer(0));

    vtkNew<vtkUnsignedCharArray> result;
    controller->AllGatherV(source, result);

    auto range = vtk::DataArrayValueRange(result);
    std::copy(range.begin(), range.end(), std::inserter(cellTypes, cellTypes.end()));
  }

  // compute element counts
  std::atomic<int64_t> elementCounts[VTK_NUMBER_OF_CELL_TYPES];
  std::fill_n(elementCounts, VTK_NUMBER_OF_CELL_TYPES, 0);

  for (auto& ds : datasets)
  {
    vtkSMPTools::For(0, ds->GetNumberOfCells(), [&](vtkIdType start, vtkIdType end) {
      for (vtkIdType cc = start; cc < end; ++cc)
      {
        // memory_order_relaxed is safe here, since we're not using the atomics for
        // synchronization.
        elementCounts[ds->GetCellType(cc)].fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  // convert element counts to a map
  std::map<unsigned char, int64_t> elementCountsMap;
  for (unsigned char i = 0; i < VTK_NUMBER_OF_CELL_TYPES; ++i)
  {
    if (elementCounts[i] > 0)
    {
      elementCountsMap[i] = elementCounts[i];
    }
  }
  return elementCountsMap;
}

//=============================================================================
Ioss::Field::BasicType GetFieldType(vtkDataArray* array)
{
  if (array->GetDataType() == VTK_DOUBLE || array->GetDataType() == VTK_FLOAT)
  {
    return Ioss::Field::DOUBLE;
  }
  else if (array->GetDataTypeSize() <= 32)
  {
    return Ioss::Field::INT32;
  }
  else
  {
    return Ioss::Field::INT64;
  }
}

//=============================================================================
std::vector<std::tuple<std::string, Ioss::Field::BasicType, int>> GetFields(int association,
  bool chooseArraysToWrite, vtkDataArraySelection* arraySelection, vtkCompositeDataSet* cds,
  vtkMultiProcessController* controller)
{
  std::vector<std::tuple<std::string, Ioss::Field::BasicType, int>> fields;
  vtkDataSetAttributesFieldList fieldList;
  for (auto& ds : vtkCompositeDataSet::GetDataSets<vtkDataSet>(cds))
  {
    if (ds->GetNumberOfElements(association) == 0)
    {
      continue;
    }
    fieldList.IntersectFieldList(ds->GetAttributes(association));
  }

  auto tmpDA = vtkSmartPointer<vtkDataSetAttributes>::New();
  tmpDA->CopyAllocate(fieldList, 1);
  tmpDA->SetNumberOfTuples(1);
  if (tmpDA->GetGlobalIds())
  {
    // we don't want to add global IDs again.
    tmpDA->RemoveArray(tmpDA->GetGlobalIds()->GetName());
  }
  if (tmpDA->HasArray("element_side"))
  {
    // we don't want to add element_side again.
    tmpDA->RemoveArray("element_side");
  }
  if (tmpDA->HasArray("object_id"))
  {
    // skip "object_id". that's an array added by Ioss reader.
    tmpDA->RemoveArray("object_id");
  }
  if (tmpDA->HasArray("original_object_id"))
  {
    // skip "original_object_id". that's an array added by Ioss reader.
    tmpDA->RemoveArray("original_object_id");
  }
  if (controller->GetNumberOfProcesses() > 1)
  {
    // gather the number of elements from all ranks.
    vtkNew<vtkIdTypeArray> sendNumberOfElements;
    sendNumberOfElements->InsertNextValue(cds->GetNumberOfElements(association));
    vtkNew<vtkIdTypeArray> recvNumberOfElements;
    controller->AllGather(sendNumberOfElements, recvNumberOfElements);
    // create a table to pack the tmpDA
    auto send = vtkSmartPointer<vtkTable>::New();
    send->GetRowData()->ShallowCopy(tmpDA);
    // now gather all field data from all ranks.
    std::vector<vtkSmartPointer<vtkDataObject>> globalRecv;
    controller->AllGather(send, globalRecv);
    // now intersect all row data to get the common fields.
    vtkDataSetAttributesFieldList globalFieldList;
    for (size_t i = 0; i < globalRecv.size(); ++i)
    {
      const auto table = vtkTable::SafeDownCast(globalRecv[i]);
      const auto numberOfElements = recvNumberOfElements->GetValue(i);
      // skip empty datasets.
      if (table && numberOfElements > 0)
      {
        // intersect row data with current field list.
        globalFieldList.IntersectFieldList(table->GetRowData());
      }
    }
    auto globalTmpDA = vtkSmartPointer<vtkDataSetAttributes>::New();
    globalTmpDA->CopyAllocate(globalFieldList, 1);
    // re-write localDA with globalDA
    tmpDA = globalTmpDA;
  }
  for (int idx = 0, max = tmpDA->GetNumberOfArrays(); idx < max; ++idx)
  {
    auto array = tmpDA->GetArray(idx);
    if (array && (!chooseArraysToWrite || arraySelection->ArrayIsEnabled(array->GetName())))
    {
      const auto type = ::GetFieldType(array);
      fields.emplace_back(array->GetName(), type, array->GetNumberOfComponents());
    }
  }
  return fields;
}

//=============================================================================
template <typename T>
struct PutFieldWorker
{
  std::vector<std::vector<T>> SOAData;
  std::vector<T> AOSData;
  size_t Offset{ 0 };
  const std::vector<vtkIdType>* SourceIds = nullptr;
  int NumComponents{ 0 };
  bool CreateAOS{ true };
  PutFieldWorker(int numComponents, size_t targetSize, bool createAOS)
    : NumComponents(numComponents)
    , CreateAOS(createAOS)
  {
    if (createAOS)
    {
      this->AOSData.resize(static_cast<size_t>(numComponents * targetSize));
    }
    else
    {
      this->SOAData.resize(numComponents);
      for (int cc = 0; cc < numComponents; ++cc)
      {
        this->SOAData[cc].resize(targetSize);
      }
    }
  }

  void SetSourceIds(const std::vector<vtkIdType>* ids) { this->SourceIds = ids; }

  template <typename ArrayType>
  void operator()(ArrayType* array)
  {
    using SourceT = vtk::GetAPIType<ArrayType>;
    vtkSMPThreadLocal<std::vector<SourceT>> tlTuple;
    vtkSMPTools::For(0, this->SourceIds->size(), [&](vtkIdType start, vtkIdType end) {
      auto tuple = tlTuple.Local();
      tuple.resize(this->NumComponents);
      if (this->CreateAOS)
      {
        for (vtkIdType cc = start; cc < end; ++cc)
        {
          array->GetTypedTuple((*this->SourceIds)[cc], tuple.data());
          for (int comp = 0; comp < this->NumComponents; ++comp)
          {
            this->AOSData[(this->Offset + cc) * this->NumComponents + comp] =
              static_cast<T>(tuple[comp]);
          }
        }
      }
      else
      {
        for (vtkIdType cc = start; cc < end; ++cc)
        {
          array->GetTypedTuple((*this->SourceIds)[cc], tuple.data());
          for (int comp = 0; comp < this->NumComponents; ++comp)
          {
            this->SOAData[comp][this->Offset + cc] = static_cast<T>(tuple[comp]);
          }
        }
      }
    });

    this->Offset += this->SourceIds->size();
  }

  void ImplicitPointsOperator(vtkDataSet* ds)
  {
    vtkSMPThreadLocal<std::vector<double>> tlTuple;
    vtkSMPTools::For(0, this->SourceIds->size(), [&](vtkIdType start, vtkIdType end) {
      auto tuple = tlTuple.Local();
      tuple.resize(this->NumComponents);
      for (vtkIdType cc = start; cc < end; ++cc)
      {
        ds->GetPoint((*this->SourceIds)[cc], tuple.data());
        for (int comp = 0; comp < this->NumComponents; ++comp)
        {
          this->SOAData[comp][this->Offset + cc] = static_cast<T>(tuple[comp]);
        }
      }
    });
  }
};

template <typename T>
struct DisplacementWorker
{
  std::vector<std::vector<T>>& Data;
  size_t Offset{ 0 };
  double Magnitude;
  const std::vector<vtkIdType>* SourceIds = nullptr;
  DisplacementWorker(std::vector<std::vector<T>>& data, double magnitude)
    : Data(data)
    , Magnitude(magnitude)
  {
  }

  void SetSourceIds(const std::vector<vtkIdType>* ids) { this->SourceIds = ids; }

  template <typename ArrayType>
  void operator()(ArrayType* array)
  {
    using SourceT = vtk::GetAPIType<ArrayType>;
    vtkSMPTools::For(0, this->SourceIds->size(), [&](vtkIdType start, vtkIdType end) {
      SourceT* displ = new SourceT[this->Data.size()];
      for (vtkIdType cc = start; cc < end; ++cc)
      {
        array->GetTypedTuple((*this->SourceIds)[cc], displ);
        for (size_t comp = 0; comp < this->Data.size(); ++comp)
        {
          this->Data[comp][this->Offset + cc] -= (displ[comp] * this->Magnitude);
        }
      }
      delete[] displ;
    });

    this->Offset += this->SourceIds->size();
  }
};

//=============================================================================
struct vtkGroupingEntity
{
  vtkIOSSWriter* Writer = nullptr;
  vtkGroupingEntity(vtkIOSSWriter* writer)
    : Writer(writer)
  {
  }
  virtual ~vtkGroupingEntity() = default;
  virtual Ioss::EntityType GetIOSSEntityType() const
  {
    try
    {
      return vtkIOSSUtilities::GetIOSSEntityType(this->GetEntityType());
    }
    catch (std::runtime_error&)
    {
      return Ioss::EntityType::INVALID_TYPE;
    }
  }
  virtual vtkIOSSWriter::EntityType GetEntityType() const
  {
    return vtkIOSSWriter::EntityType::NUMBER_OF_ENTITY_TYPES;
  }
  virtual void DefineModel(Ioss::Region& region) const = 0;
  virtual void Model(Ioss::Region& region) const = 0;
  virtual void DefineTransient(Ioss::Region& region) const = 0;
  virtual void Transient(Ioss::Region& region) const = 0;
  virtual void AppendMD5(vtksysMD5* md5) const = 0;

protected:
  template <typename IossGroupingEntityT, typename DatasetT>
  void PutFields(IossGroupingEntityT* block,
    const std::vector<std::tuple<std::string, Ioss::Field::BasicType, int>>& fields,
    const std::vector<std::vector<vtkIdType>>& lIds, const std::vector<DatasetT*>& datasets,
    int association) const
  {
    for (const auto& field : fields)
    {
      const auto& name = std::get<0>(field);
      const auto& type = std::get<1>(field);
      const auto& numComponents = std::get<2>(field);
      switch (type)
      {
        case Ioss::Field::DOUBLE:
          this->PutField<double>(block, name, numComponents, lIds, datasets, association);
          break;

        case Ioss::Field::INT32:
          this->PutField<int32_t>(block, name, numComponents, lIds, datasets, association);
          break;

        case Ioss::Field::INT64:
          this->PutField<int64_t>(block, name, numComponents, lIds, datasets, association);
          break;

        default:
          vtkLogF(TRACE, "Unsupported field type. Skipping %s", name.c_str());
          break;
      }
    }
  }

  template <typename T, typename IossGroupingEntityT, typename DatasetT>
  void PutField(IossGroupingEntityT* block, const std::string& name, int numComponents,
    const std::vector<std::vector<vtkIdType>>& lIds, const std::vector<DatasetT*>& datasets,
    int association) const
  {
    assert(datasets.size() == lIds.size());
    const size_t totalSize = std::accumulate(lIds.begin(), lIds.end(), static_cast<size_t>(0),
      [](size_t sum, const std::vector<vtkIdType>& ids) { return sum + ids.size(); });

    using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;
    const bool createAOS = numComponents <= 3;
    PutFieldWorker<T> worker(numComponents, totalSize, createAOS);
    for (size_t dsIndex = 0; dsIndex < datasets.size(); ++dsIndex)
    {
      auto& ds = datasets[dsIndex];
      auto& lids = lIds[dsIndex];
      worker.SetSourceIds(&lids);
      if (auto array = ds->GetAttributes(association)->GetArray(name.c_str()))
      {
        if (!Dispatcher::Execute(array, worker))
        {
          vtkLogF(ERROR, "Failed to dispatch array %s", name.c_str());
        }
      }
    }

    if (createAOS)
    {
      block->put_field_data(name, worker.AOSData);
    }
    else
    {
      for (int comp = 0; comp < numComponents; ++comp)
      {
        const auto compName = name + std::to_string(comp + 1);
        block->put_field_data(compName, worker.SOAData[comp]);
      }
    }
  }

  void DefineFields(Ioss::GroupingEntity* block,
    const std::vector<std::tuple<std::string, Ioss::Field::BasicType, int>>& fields,
    Ioss::Field::RoleType role, int64_t elementCount) const
  {
    for (const auto& field : fields)
    {
      const auto& name = std::get<0>(field);
      const auto& type = std::get<1>(field);
      const auto& numComponents = std::get<2>(field);
      switch (numComponents)
      {
        // fancier variable type names can be found in Ioss_ConcreteVariableType.C
        case 1:
        {
          block->field_add(Ioss::Field(name, type, IOSS_SCALAR(), role, elementCount));
          break;
        }
        case 2:
        {
          block->field_add(Ioss::Field(name, type, IOSS_VECTOR_2D(), role, elementCount));
          break;
        }
        case 3:
        {
          block->field_add(Ioss::Field(name, type, IOSS_VECTOR_3D(), role, elementCount));
          break;
        }
        default:
        {
          for (int comp = 0; comp < numComponents; ++comp)
          {
            const auto compName = name + std::to_string(comp + 1);
            block->field_add(Ioss::Field(compName, type, IOSS_SCALAR(), role, elementCount));
          }
        }
      }
    }
  }
};

} // end namespace {}

VTK_ABI_NAMESPACE_BEGIN
/**
 * Builds an Ioss::NodeBlock. Since an exodus file has a single common node
 * block, we need to build one based on all points from all blocks.
 *
 * Another thing to handle is displacements. If input dataset is coming from
 * IOSS reader, the point coordinates may have been displaced using the
 * displacement vectors in the dataset.
 */
struct vtkNodeBlock : vtkGroupingEntity
{
  const std::vector<vtkDataSet*> DataSets;
  const std::string Name;

  // build a map of ds idx, gid, and lid and use that later.
  std::vector<int32_t> Ids;
  std::vector<std::vector<vtkIdType>> IdsRaw;

  std::vector<std::tuple<std::string, Ioss::Field::BasicType, int>> Fields;

  vtkNodeBlock(vtkPartitionedDataSetCollection* pdc, const std::string& name,
    vtkMultiProcessController* controller, vtkIOSSWriter* writer)
    : vtkGroupingEntity(writer)
    , DataSets(vtkCompositeDataSet::GetDataSets<vtkDataSet>(pdc))
    , Name(name)
  {
    this->IdsRaw.reserve(this->DataSets.size());

    std::set<int32_t> id_set;
    for (auto& ds : this->DataSets)
    {
      auto* gids = vtkIdTypeArray::SafeDownCast(ds->GetPointData()->GetGlobalIds());
      if (!gids && ds->GetNumberOfPoints() != 0)
      {
        throw std::runtime_error("point global IDs missing.");
      }

      const auto numPoints = ds->GetNumberOfPoints();
      if (gids)
      {
        assert(gids->GetNumberOfTuples() == numPoints);
      }

      this->Ids.reserve(this->Ids.size() + numPoints);
      this->IdsRaw.emplace_back();
      this->IdsRaw.back().reserve(numPoints);
      const vtkIdType gidOffset = writer->GetOffsetGlobalIds() ? 1 : 0;
      for (vtkIdType cc = 0; cc < numPoints; ++cc)
      {
        const auto gid = gids->GetValue(cc);
        if (id_set.insert(gid).second)
        {
          this->Ids.push_back(gid + gidOffset);
          this->IdsRaw.back().push_back(cc);
        }
      }
    }

    assert(this->DataSets.size() == this->IdsRaw.size());
    this->Fields = ::GetFields(vtkDataObject::POINT, writer->GetChooseFieldsToWrite(),
      writer->GetNodeBlockFieldSelection(), pdc, controller);
  }

  vtkIOSSWriter::EntityType GetEntityType() const override
  {
    return vtkIOSSWriter::EntityType::NODEBLOCK;
  }

  void AppendMD5(vtksysMD5* md5) const override
  {
    vtksysMD5_Append(md5, reinterpret_cast<const unsigned char*>(this->Ids.data()),
      static_cast<int>(sizeof(int32_t) * this->Ids.size()));
  }

  void DefineModel(Ioss::Region& region) const override
  {
    auto* nodeBlock = new Ioss::NodeBlock(region.get_database(), this->Name, this->Ids.size(), 3);
    nodeBlock->property_add(Ioss::Property("id", 1)); // block id.
    region.add(nodeBlock);
  }

  void DefineTransient(Ioss::Region& region) const override
  {
    auto* nodeBlock = region.get_node_block(this->Name);
    this->DefineFields(nodeBlock, this->Fields, Ioss::Field::TRANSIENT, this->Ids.size());
  }

  void Model(Ioss::Region& region) const override
  {
    auto* nodeBlock = region.get_node_block(this->Name);
    nodeBlock->put_field_data("ids", this->Ids);

    // add mesh coordinates
    using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
    PutFieldWorker<double> worker(3, this->Ids.size(), false /* createAOS */);
    for (size_t dsIndex = 0; dsIndex < this->DataSets.size(); ++dsIndex)
    {
      auto& ds = this->DataSets[dsIndex];
      auto& lids = this->IdsRaw[dsIndex];
      worker.SetSourceIds(&lids);
      if (auto ps = vtkPointSet::SafeDownCast(ds))
      {
        if (ps->GetPoints())
        {
          if (!Dispatcher::Execute(ps->GetPoints()->GetData(), worker))
          {
            vtkLog(ERROR, "Failed to dispatch points.");
          }
        }
      }
      else
      {
        worker.ImplicitPointsOperator(ds);
      }
    }

    // if displacement array is present, offset the mesh coordinates by the
    // provided displacement.
    const auto displMagnitude =
      this->DataSets.empty() ? 0.0 : this->Writer->GetDisplacementMagnitude();
    const std::string displName =
      displMagnitude > 0 ? vtkIOSSUtilities::GetDisplacementFieldName(this->DataSets.front()) : "";
    if (!displName.empty() && displMagnitude > 0.0)
    {
      DisplacementWorker<double> dworker(worker.SOAData, displMagnitude);
      for (size_t dsIndex = 0; dsIndex < this->DataSets.size(); ++dsIndex)
      {
        auto& ds = this->DataSets[dsIndex];
        auto& lids = this->IdsRaw[dsIndex];
        dworker.SetSourceIds(&lids);
        if (auto dispArray = ds->GetPointData()->GetArray(displName.c_str()))
        {
          if (!Dispatcher::Execute(dispArray, dworker))
          {
            vtkLog(ERROR, "Failed to dispatch displacements.");
          }
        }
      }
    }

    nodeBlock->put_field_data("mesh_model_coordinates_x", worker.SOAData[0]);
    nodeBlock->put_field_data("mesh_model_coordinates_y", worker.SOAData[1]);
    nodeBlock->put_field_data("mesh_model_coordinates_z", worker.SOAData[2]);
  }

  void Transient(Ioss::Region& region) const override
  {
    auto* nodeBlock = region.get_node_block(this->Name);
    this->PutFields(nodeBlock, this->Fields, this->IdsRaw, this->DataSets, vtkDataObject::POINT);
  }
};

/**
 * Builds an Ioss::(*)Block from a vtkPartitionedDataSet. The differences
 * between the Ioss and VTK data model for the two are handled as follows:
 *
 * * We only support vtkPartitionedDataSet comprising of one or more vtkDataSets.
 *   All other dataset types are simply ignored.
 *
 * * A Block cannot have multiple "pieces" in the same file. So if a
 *   vtkPartitionedDataSet has multiple datasets, we need to "combine" them into
 *   one.
 *
 * * A Block cannot have elements of different types. However,
 *   vtkDataSet supports heterogeneous cells. So if all
 *   vtkDataSets in the vtkPartitionedDataSet have more than 1 cell type,
 *   we create multiple blocks. Each Block is uniquely named by
 *   using the given block name and the element type as a suffix.
 *
 *   In MPI world, the cell types are gathered across all ranks to ensure each
 *   ranks creates identical blocks / block names.
 *
 */
struct vtkEntityBlock : public vtkGroupingEntity
{
  const std::vector<vtkDataSet*> DataSets;
  std::string RootName;
  int BlockId;
  int StartSplitElementBlockId;

  std::map<unsigned char, int64_t> ElementCounts;
  std::vector<std::tuple<std::string, Ioss::Field::BasicType, int>> Fields;

  vtkEntityBlock(vtkPartitionedDataSet* pds, vtkIOSSWriter::EntityType entityType,
    const std::string& name, const int blockId, int startSplitElementBlockId,
    vtkMultiProcessController* controller, vtkIOSSWriter* writer)
    : vtkGroupingEntity(writer)
    , DataSets(vtkCompositeDataSet::GetDataSets<vtkDataSet>(pds))
    , RootName(name)
    , BlockId(blockId)
    , StartSplitElementBlockId(startSplitElementBlockId)
  {
    for (auto& ds : this->DataSets)
    {
      auto* gids = vtkIdTypeArray::SafeDownCast(ds->GetCellData()->GetGlobalIds());
      if (!gids && ds->GetNumberOfCells() != 0)
      {
        throw std::runtime_error("cell global IDs missing!");
      }
    }

    this->ElementCounts = ::GetElementCounts(pds, controller);
    this->Fields = ::GetFields(vtkDataObject::CELL, writer->GetChooseFieldsToWrite(),
      writer->GetFieldSelection(entityType), pds, controller);
  }

  void AppendMD5(vtksysMD5* md5) const override
  {
    vtksysMD5_Append(md5, reinterpret_cast<const unsigned char*>(this->RootName.c_str()), -1);
    for (auto& pair : this->ElementCounts)
    {
      vtksysMD5_Append(md5, reinterpret_cast<const unsigned char*>(&pair.first),
        static_cast<int>(sizeof(pair.first)));
      vtksysMD5_Append(md5, reinterpret_cast<const unsigned char*>(&pair.second),
        static_cast<int>(sizeof(pair.second)));
    }
  }

  /**
   * Get the block name and id of a given element in a block.
   */
  std::pair<int, std::string> GetSubElementBlockInfo(
    unsigned char vtkCellType, std::string elementType) const
  {
    const bool preservedStructure = this->ElementCounts.size() == 1;
    if (preservedStructure)
    {
      return std::make_pair(this->BlockId, this->RootName);
    }
    else
    {
      const int splitElementBlockId =
        this->StartSplitElementBlockId + static_cast<int>(vtkCellType);
      const std::string blockName = this->RootName + "_" + elementType;
      return std::make_pair(splitElementBlockId, blockName);
    }
  }

  virtual Ioss::EntityBlock* CreateEntity(Ioss::DatabaseIO* db, const std::string& blockName,
    const std::string& elementType, int64_t elementCount) const = 0;

  virtual void AddEntity(Ioss::Region& region, Ioss::EntityBlock* entityBlock) const = 0;

  virtual Ioss::EntityBlock* GetEntity(
    Ioss::Region& region, const std::string& blockName) const = 0;

  void DefineModel(Ioss::Region& region) const override
  {
    for (const auto& element : this->ElementCounts)
    {
      const int64_t elementCount = element.second;
      const unsigned char vtk_cell_type = element.first;

      const auto* elementTopology = vtkIOSSUtilities::GetElementTopology(vtk_cell_type);
      const auto& elementType = elementTopology->name();
      const auto blockInfo = this->GetSubElementBlockInfo(vtk_cell_type, elementType);

      auto entityBlock =
        this->CreateEntity(region.get_database(), blockInfo.second, elementType, elementCount);
      entityBlock->property_add(Ioss::Property("id", blockInfo.first));
      if (this->Writer->GetPreserveOriginalIds())
      {
        entityBlock->property_add(
          Ioss::Property("original_id", this->BlockId, Ioss::Property::ATTRIBUTE));
      }
      this->AddEntity(region, entityBlock);
    }
  }

  void DefineTransient(Ioss::Region& region) const override
  {
    for (const auto& element : this->ElementCounts)
    {
      const int64_t elementCount = element.second;
      const unsigned char vtk_cell_type = element.first;

      const auto* elementTopology = vtkIOSSUtilities::GetElementTopology(vtk_cell_type);
      const auto& elementType = elementTopology->name();
      const auto blockName = this->GetSubElementBlockInfo(vtk_cell_type, elementType).second;

      auto* entityBlock = this->GetEntity(region, blockName);
      this->DefineFields(entityBlock, this->Fields, Ioss::Field::TRANSIENT, elementCount);
    }
  }

  // IOSS cells and VTK cells need not have same point ordering. If that's the
  // case, we need to transform them. vtkIOSSUtilities::GetConnectivity() shows how to transform
  // point ordering from IOSS to VTK. Here, we are defining the transformation from VTK based
  // ordering to IOSS based ordering. To do that, we assume the IOSS has the original ordering
  // O = { o_i = i | i in [1, n] }. Then, vtkIOSSUtilities::GetConnectivity() defines the
  // transformation T = { t_i | i in [1, n] }. So now the IOSS has been converted to VTK ordering.
  // To convert VTK ordering to IOSS ordering, we find the transformation that converts T to O.
  // The below function returns that transformation.
  // ref: https://sandialabs.github.io/seacas-docs/html/md_include_exodus_element_types.html
  static bool NeedsIdsTransformation(
    const unsigned char vtkCellType, std::vector<int>& orderingTransformation)
  {
    switch (vtkCellType)
    {
      case VTK_WEDGE:
      {
        orderingTransformation = std::vector<int>{ 4, 5, 6, 1, 2, 3 };
        break;
      }
      case VTK_QUADRATIC_WEDGE:
      {
        // clang-format off
        orderingTransformation = std::vector<int>{
          4, 5, 6, 1, 2, 3,
          10, 11, 12,
          13, 14, 15,
          7, 8, 9,
        };
        // clang-format on
        break;
      }
      case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
      {
        // clang-format off
        orderingTransformation = std::vector<int>{
          4, 5, 6, 1, 2, 3,
          10, 11, 12,
          13, 14, 15,
          7, 8, 9,
          16, 17, 18
        };
        // clang-format on
        break;
      }
      case VTK_QUADRATIC_HEXAHEDRON:
      {
        // clang-format off
        orderingTransformation = std::vector<int>{
          /* 8 corners */
          1, 2, 3, 4,
          5, 6, 7, 8,

          /* 12 mid-edge nodes */
          9, 10, 11, 12,
          17, 18, 19, 20,
          13, 14, 15, 16
        };
        // clang-format on
        break;
      }
      case VTK_TRIQUADRATIC_HEXAHEDRON:
      {
        // clang-format off
        orderingTransformation = std::vector<int>{
          1, 2, 3, 4,
          5, 6, 7, 8,
          9, 10, 11, 12,
          17, 18, 19, 20,
          13, 14, 15, 16,
          27, 25, 26, 21,
          22, 23, 24
        };
        // clang-format on
        break;
      }
      case VTK_LAGRANGE_WEDGE:
      {
        // We only handle 21-node wedges for now.
        // The caller checks whether our returned size matches.
        // clang-format off
        orderingTransformation = std::vector<int>{
            // nodes
          4, 5, 6, 1, 2, 3,
            // edge mid-points
         10, 11, 12,
         13, 14, 15,
         7, 8, 9,
           // body center
         21,
           // triangle faces
         17, 16,
           // quad faces
         19, 20, 18
        };
        // clang-format on
        break;
      }
      default:
        break;
    }
    const bool needsTransformation = !orderingTransformation.empty();
    if (needsTransformation)
    {
      // offset by 1 to make 0-based.
      for (auto& val : orderingTransformation)
      {
        val -= 1;
      }
    }
    return needsTransformation;
  }

  void Model(Ioss::Region& region) const override
  {
    for (const auto& element : this->ElementCounts)
    {
      const int64_t elementCount = element.second;
      const unsigned char vtk_cell_type = element.first;

      const auto* elementTopology = vtkIOSSUtilities::GetElementTopology(vtk_cell_type);
      const auto& elementType = elementTopology->name();
      const int nodeCount = elementTopology->number_nodes();
      const auto blockName = this->GetSubElementBlockInfo(vtk_cell_type, elementType).second;

      auto* entityBlock = this->GetEntity(region, blockName);

      std::vector<int> orderingTransformation;
      const bool needsIdsTransformation =
        NeedsIdsTransformation(vtk_cell_type, orderingTransformation);
      // populate ids.
      std::vector<int32_t> elementIds; // these are global IDs.
      elementIds.reserve(elementCount);

      std::vector<int32_t> connectivity;
      connectivity.reserve(elementCount * nodeCount);

      const int32_t gidOffset = this->Writer->GetOffsetGlobalIds() ? 1 : 0;
      const bool removeGhosts = this->Writer->GetRemoveGhosts();
      for (auto& ds : this->DataSets)
      {
        vtkUnsignedCharArray* ghost = ds->GetCellGhostArray();
        auto* gids = vtkIdTypeArray::SafeDownCast(ds->GetCellData()->GetGlobalIds());
        auto* pointGIDs = vtkIdTypeArray::SafeDownCast(ds->GetPointData()->GetGlobalIds());

        vtkNew<vtkIdList> tempCellPointIds;
        for (vtkIdType cc = 0, max = ds->GetNumberOfCells(); cc < max; ++cc)
        {
          const bool process = !removeGhosts || !ghost || ghost->GetValue(cc) == 0;
          if (process && ds->GetCellType(cc) == vtk_cell_type)
          {
            elementIds.push_back(gidOffset + gids->GetValue(cc));

            vtkIdType numPts;
            vtkIdType const* cellPoints;
            ds->GetCellPoints(cc, numPts, cellPoints, tempCellPointIds);
            assert(numPts == nodeCount);

            if (!needsIdsTransformation)
            {
              // map cell's point to global IDs for those points.
              std::transform(cellPoints, cellPoints + numPts, std::back_inserter(connectivity),
                [&](vtkIdType ptid) { return gidOffset + pointGIDs->GetValue(ptid); });
            }
            else
            {
              if (orderingTransformation.size() != static_cast<size_t>(numPts))
              {
                vtkGenericWarningMacro("Cell of type "
                  << vtk_cell_type << " has " << numPts
                  << "entries but order transformation expects " << orderingTransformation.size()
                  << " entries. Skipping transform.");
                std::transform(cellPoints, cellPoints + numPts, std::back_inserter(connectivity),
                  [&](vtkIdType ptid) { return gidOffset + pointGIDs->GetValue(ptid); });
              }
              else
              {
                std::transform(orderingTransformation.begin(), orderingTransformation.end(),
                  std::back_inserter(connectivity), [&](int localId) {
                    return gidOffset + pointGIDs->GetValue(cellPoints[localId]);
                  });
              }
            }
          }
        }
      }
      assert(elementIds.size() == static_cast<size_t>(elementCount));
      assert(connectivity.size() == static_cast<size_t>(elementCount * nodeCount));
      entityBlock->put_field_data("ids", elementIds);
      entityBlock->put_field_data("connectivity", connectivity);
    }
  }

  void Transient(Ioss::Region& region) const override
  {
    for (const auto& element : this->ElementCounts)
    {
      const unsigned char vtk_cell_type = element.first;

      const auto* elementTopology = vtkIOSSUtilities::GetElementTopology(vtk_cell_type);
      const auto& elementType = elementTopology->name();
      const auto blockName = this->GetSubElementBlockInfo(vtk_cell_type, elementType).second;

      auto* entityBlock = this->GetEntity(region, blockName);

      // populate ids.
      std::vector<std::vector<vtkIdType>> lIds; // these are local ids.
      const bool removeGhosts = this->Writer->GetRemoveGhosts();
      for (auto& ds : this->DataSets)
      {
        vtkUnsignedCharArray* ghost = ds->GetCellGhostArray();
        lIds.emplace_back();
        lIds.back().reserve(ds->GetNumberOfCells());
        for (vtkIdType cc = 0, max = ds->GetNumberOfCells(); cc < max; ++cc)
        {
          const bool process = !removeGhosts || !ghost || ghost->GetValue(cc) == 0;
          if (process && ds->GetCellType(cc) == vtk_cell_type)
          {
            lIds.back().push_back(cc);
          }
        }
      }

      // add fields.
      this->PutFields(entityBlock, this->Fields, lIds, this->DataSets, vtkDataObject::CELL);
    }
  }
};

//=============================================================================
struct vtkEdgeBlock : public vtkEntityBlock
{
  vtkEdgeBlock(vtkPartitionedDataSet* pds, const std::string& name, const int blockId,
    int startSplitElementBlockId, vtkMultiProcessController* controller, vtkIOSSWriter* writer)
    : vtkEntityBlock(
        pds, this->GetEntityType(), name, blockId, startSplitElementBlockId, controller, writer)
  {
  }

  vtkIOSSWriter::EntityType GetEntityType() const override
  {
    return vtkIOSSWriter::EntityType::EDGEBLOCK;
  }

  Ioss::EntityBlock* CreateEntity(Ioss::DatabaseIO* db, const std::string& blockName,
    const std::string& elementType, const int64_t elementCount) const override
  {
    return new Ioss::EdgeBlock(db, blockName, elementType, elementCount);
  }

  void AddEntity(Ioss::Region& region, Ioss::EntityBlock* entityBlock) const override
  {
    region.add(dynamic_cast<Ioss::EdgeBlock*>(entityBlock));
  }

  Ioss::EntityBlock* GetEntity(Ioss::Region& region, const std::string& blockName) const override
  {
    return region.get_edge_block(blockName);
  }
};

//=============================================================================
struct vtkFaceBlock : public vtkEntityBlock
{
  vtkFaceBlock(vtkPartitionedDataSet* pds, const std::string& name, const int blockId,
    int startSplitElementBlockId, vtkMultiProcessController* controller, vtkIOSSWriter* writer)
    : vtkEntityBlock(
        pds, this->GetEntityType(), name, blockId, startSplitElementBlockId, controller, writer)
  {
  }

  vtkIOSSWriter::EntityType GetEntityType() const override
  {
    return vtkIOSSWriter::EntityType::FACEBLOCK;
  }

  Ioss::EntityBlock* CreateEntity(Ioss::DatabaseIO* db, const std::string& blockName,
    const std::string& elementType, const int64_t elementCount) const override
  {
    return new Ioss::FaceBlock(db, blockName, elementType, elementCount);
  }

  void AddEntity(Ioss::Region& region, Ioss::EntityBlock* entityBlock) const override
  {
    region.add(dynamic_cast<Ioss::FaceBlock*>(entityBlock));
  }

  Ioss::EntityBlock* GetEntity(Ioss::Region& region, const std::string& blockName) const override
  {
    return region.get_face_block(blockName);
  }
};

//=============================================================================
struct vtkElementBlock : public vtkEntityBlock
{
  vtkElementBlock(vtkPartitionedDataSet* pds, const std::string& name, const int blockId,
    int startSplitElementBlockId, vtkMultiProcessController* controller, vtkIOSSWriter* writer)
    : vtkEntityBlock(
        pds, this->GetEntityType(), name, blockId, startSplitElementBlockId, controller, writer)
  {
  }

  vtkIOSSWriter::EntityType GetEntityType() const override
  {
    return vtkIOSSWriter::EntityType::ELEMENTBLOCK;
  }

  Ioss::EntityBlock* CreateEntity(Ioss::DatabaseIO* db, const std::string& blockName,
    const std::string& elementType, const int64_t elementCount) const override
  {
    return new Ioss::ElementBlock(db, blockName, elementType, elementCount);
  }

  void AddEntity(Ioss::Region& region, Ioss::EntityBlock* entityBlock) const override
  {
    region.add(dynamic_cast<Ioss::ElementBlock*>(entityBlock));
  }

  Ioss::EntityBlock* GetEntity(Ioss::Region& region, const std::string& blockName) const override
  {
    return region.get_element_block(blockName);
  }
};

//=============================================================================
struct vtkNodeSet : public vtkGroupingEntity
{
  const std::vector<vtkDataSet*> DataSets;
  std::string Name;
  int BlockId;
  int64_t Count;

  std::vector<std::tuple<std::string, Ioss::Field::BasicType, int>> Fields;

  vtkNodeSet(vtkPartitionedDataSet* pds, const std::string& name, int blockId,
    vtkMultiProcessController* controller, vtkIOSSWriter* writer)
    : vtkGroupingEntity(writer)
    , DataSets{ vtkCompositeDataSet::GetDataSets<vtkDataSet>(pds) }
    , Name(name)
    , BlockId(blockId)
    , Count(0)
  {
    for (auto& ds : this->DataSets)
    {
      auto* gids = vtkIdTypeArray::SafeDownCast(ds->GetPointData()->GetGlobalIds());
      if (!gids && ds->GetNumberOfPoints() != 0)
      {
        throw std::runtime_error("missing point global IDs for nodesets.");
      }
      const auto numPoints = ds->GetNumberOfPoints();
      if (gids)
      {
        assert(gids->GetNumberOfTuples() == numPoints);
      }
      this->Count += numPoints;
    }

    // in a nodeSet, number of points == number of cells, because cells are vertices
    this->Fields = ::GetFields(vtkDataObject::CELL, writer->GetChooseFieldsToWrite(),
      writer->GetNodeSetFieldSelection(), pds, controller);
  }

  vtkIOSSWriter::EntityType GetEntityType() const override
  {
    return vtkIOSSWriter::EntityType::NODESET;
  }

  void AppendMD5(vtksysMD5* md5) const override
  {
    vtksysMD5_Append(md5, reinterpret_cast<const unsigned char*>(this->Name.c_str()), -1);
    vtksysMD5_Append(md5, reinterpret_cast<const unsigned char*>(&this->Count),
      static_cast<int>(sizeof(this->Count)));
  }

  void DefineModel(Ioss::Region& region) const override
  {
    auto* nodeSet = new Ioss::NodeSet(region.get_database(), this->Name, this->Count);
    nodeSet->property_add(Ioss::Property("id", this->BlockId));
    region.add(nodeSet);
  }

  void DefineTransient(Ioss::Region& region) const override
  {
    auto nodeSet = region.get_nodeset(this->Name);
    this->DefineFields(nodeSet, this->Fields, Ioss::Field::TRANSIENT, this->Count);
  }

  void Model(Ioss::Region& region) const override
  {
    auto* nodeSet = region.get_nodeset(this->Name);
    // create global IDs.
    std::vector<int32_t> ids;
    ids.reserve(static_cast<size_t>(this->Count));
    for (const auto& ds : this->DataSets)
    {
      auto gids = vtkIdTypeArray::SafeDownCast(ds->GetPointData()->GetGlobalIds());
      const vtkIdType gidOffset = this->Writer->GetOffsetGlobalIds() ? 1 : 0;
      for (vtkIdType ptId = 0, max = ds->GetNumberOfPoints(); ptId < max; ++ptId)
      {
        ids.push_back(gidOffset + gids->GetValue(ptId));
      }
    }
    nodeSet->put_field_data("ids", ids);
  }

  void Transient(Ioss::Region& region) const override
  {
    auto* nodeSet = region.get_nodeset(this->Name);
    // create local ids.
    std::vector<std::vector<vtkIdType>> idsRaw;
    idsRaw.reserve(this->DataSets.size());
    for (const auto& ds : this->DataSets)
    {
      idsRaw.emplace_back();
      idsRaw.back().reserve(static_cast<size_t>(ds->GetNumberOfPoints()));
      for (vtkIdType ptId = 0, max = ds->GetNumberOfPoints(); ptId < max; ++ptId)
      {
        idsRaw.back().push_back(ptId);
      }
    }
    this->PutFields(nodeSet, this->Fields, idsRaw, this->DataSets, vtkDataObject::CELL);
  }
};

//=============================================================================
struct vtkEntitySet : public vtkGroupingEntity
{
  const std::vector<vtkDataSet*> DataSets;
  std::string Name;
  int BlockId;
  int64_t Count;

  std::vector<std::tuple<std::string, Ioss::Field::BasicType, int>> Fields;

  vtkEntitySet(vtkPartitionedDataSet* pds, vtkIOSSWriter::EntityType entityType,
    const std::string& name, int blockId, vtkMultiProcessController* controller,
    vtkIOSSWriter* writer)
    : vtkGroupingEntity(writer)
    , DataSets(vtkCompositeDataSet::GetDataSets<vtkDataSet>(pds))
    , Name(name)
    , BlockId(blockId)
    , Count(0)
  {
    for (auto& ds : this->DataSets)
    {
      // no need to check for global IDs
      auto elementSide = vtkIntArray::SafeDownCast(ds->GetCellData()->GetArray("element_side"));
      if (!elementSide && ds->GetNumberOfCells() != 0)
      {
        throw std::runtime_error("missing 'element_side' cell array.");
      }

      this->Count += ds->GetNumberOfCells();
    }
    this->Fields = ::GetFields(vtkDataObject::CELL, writer->GetChooseFieldsToWrite(),
      writer->GetFieldSelection(entityType), pds, controller);
  }

  void AppendMD5(vtksysMD5* md5) const override
  {
    vtksysMD5_Append(md5, reinterpret_cast<const unsigned char*>(this->Name.c_str()), -1);
    vtksysMD5_Append(md5, reinterpret_cast<const unsigned char*>(&this->Count),
      static_cast<int>(sizeof(this->Count)));
  }

  virtual Ioss::GroupingEntity* CreateEntity(
    Ioss::DatabaseIO* db, const std::string& blockName, int64_t elementCount) const = 0;

  virtual void AddEntity(Ioss::Region& region, Ioss::GroupingEntity* entitySet) const = 0;

  virtual Ioss::GroupingEntity* GetEntity(
    Ioss::Region& region, const std::string& blockName) const = 0;

  void DefineModel(Ioss::Region& region) const override
  {
    auto entitySet = this->CreateEntity(region.get_database(), this->Name, this->Count);
    entitySet->property_add(Ioss::Property("id", this->BlockId));
    this->AddEntity(region, entitySet);
  }

  void DefineTransient(Ioss::Region& region) const override
  {
    auto entity = this->GetEntity(region, this->Name);
    this->DefineFields(entity, this->Fields, Ioss::Field::TRANSIENT, this->Count);
  }

  void Model(Ioss::Region& region) const override
  {
    auto entity = this->GetEntity(region, this->Name);

    std::vector<int32_t> elementSide;
    elementSide.reserve(this->Count * 2);

    const bool removeGhosts = this->Writer->GetRemoveGhosts();
    for (auto& ds : this->DataSets)
    {
      if (ds->GetNumberOfCells() == 0)
      {
        continue;
      }
      auto elemSideArray = vtkIntArray::SafeDownCast(ds->GetCellData()->GetArray("element_side"));
      vtkUnsignedCharArray* ghost = ds->GetCellGhostArray();
      const auto elementSideRange = vtk::DataArrayTupleRange(elemSideArray);
      for (vtkIdType cc = 0; cc < elementSideRange.size(); ++cc)
      {
        const bool process = !removeGhosts || !ghost || ghost->GetValue(cc) == 0;
        if (process)
        {
          for (const auto& comp : elementSideRange[cc])
          {
            elementSide.push_back(comp);
          }
        }
      }
    }

    assert(elementSide.size() == static_cast<size_t>(this->Count * 2));
    entity->put_field_data("element_side", elementSide);
  }

  void Transient(Ioss::Region& region) const override
  {
    auto entity = this->GetEntity(region, this->Name);

    // populate ids.
    std::vector<std::vector<vtkIdType>> lIds; // these are local ids.
    const bool removeGhosts = this->Writer->GetRemoveGhosts();
    for (auto& ds : this->DataSets)
    {
      vtkUnsignedCharArray* ghost = ds->GetCellGhostArray();
      lIds.emplace_back();
      lIds.back().reserve(static_cast<size_t>(ds->GetNumberOfCells()));
      for (vtkIdType cc = 0, max = ds->GetNumberOfCells(); cc < max; ++cc)
      {
        const bool process = !removeGhosts || !ghost || ghost->GetValue(cc) == 0;
        if (process)
        {
          lIds.back().push_back(cc);
        }
      }
    }

    // add fields.
    this->PutFields(entity, this->Fields, lIds, this->DataSets, vtkDataObject::CELL);
  }
};

//=============================================================================
struct vtkEdgeSet : public vtkEntitySet
{
  vtkEdgeSet(vtkPartitionedDataSet* pds, const std::string& name, int blockId,
    vtkMultiProcessController* controller, vtkIOSSWriter* writer)
    : vtkEntitySet(pds, this->GetEntityType(), name, blockId, controller, writer)
  {
  }

  vtkIOSSWriter::EntityType GetEntityType() const override
  {
    return vtkIOSSWriter::EntityType::EDGESET;
  }

  Ioss::GroupingEntity* CreateEntity(
    Ioss::DatabaseIO* db, const std::string& blockName, const int64_t elementCount) const override
  {
    return new Ioss::EdgeSet(db, blockName, elementCount);
  }

  void AddEntity(Ioss::Region& region, Ioss::GroupingEntity* entitySet) const override
  {
    region.add(dynamic_cast<Ioss::EdgeSet*>(entitySet));
  }

  Ioss::GroupingEntity* GetEntity(Ioss::Region& region, const std::string& blockName) const override
  {
    return region.get_edgeset(blockName);
  }
};

//=============================================================================
struct vtkFaceSet : public vtkEntitySet
{
  vtkFaceSet(vtkPartitionedDataSet* pds, const std::string& name, int blockId,
    vtkMultiProcessController* controller, vtkIOSSWriter* writer)
    : vtkEntitySet(pds, this->GetEntityType(), name, blockId, controller, writer)
  {
  }

  vtkIOSSWriter::EntityType GetEntityType() const override
  {
    return vtkIOSSWriter::EntityType::FACESET;
  }

  Ioss::GroupingEntity* CreateEntity(
    Ioss::DatabaseIO* db, const std::string& blockName, const int64_t elementCount) const override
  {
    return new Ioss::FaceSet(db, blockName, elementCount);
  }

  void AddEntity(Ioss::Region& region, Ioss::GroupingEntity* entitySet) const override
  {
    region.add(dynamic_cast<Ioss::FaceSet*>(entitySet));
  }

  Ioss::GroupingEntity* GetEntity(Ioss::Region& region, const std::string& blockName) const override
  {
    return region.get_faceset(blockName);
  }
};

//=============================================================================
struct vtkElementSet : public vtkEntitySet
{
  vtkElementSet(vtkPartitionedDataSet* pds, const std::string& name, int blockId,
    vtkMultiProcessController* controller, vtkIOSSWriter* writer)
    : vtkEntitySet(pds, this->GetEntityType(), name, blockId, controller, writer)
  {
  }

  vtkIOSSWriter::EntityType GetEntityType() const override
  {
    return vtkIOSSWriter::EntityType::ELEMENTSET;
  }

  Ioss::GroupingEntity* CreateEntity(
    Ioss::DatabaseIO* db, const std::string& blockName, const int64_t elementCount) const override
  {
    return new Ioss::ElementSet(db, blockName, elementCount);
  }

  void AddEntity(Ioss::Region& region, Ioss::GroupingEntity* entitySet) const override
  {
    region.add(dynamic_cast<Ioss::ElementSet*>(entitySet));
  }

  Ioss::GroupingEntity* GetEntity(Ioss::Region& region, const std::string& blockName) const override
  {
    return region.get_elementset(blockName);
  }
};

//=============================================================================
struct vtkSideSet : public vtkEntitySet
{
  vtkSideSet(vtkPartitionedDataSet* pds, const std::string& name, int blockId,
    vtkMultiProcessController* controller, vtkIOSSWriter* writer)
    : vtkEntitySet(pds, this->GetEntityType(), name, blockId, controller, writer)
  {
  }

  vtkIOSSWriter::EntityType GetEntityType() const override
  {
    return vtkIOSSWriter::EntityType::SIDESET;
  }

  Ioss::GroupingEntity* CreateEntity(
    Ioss::DatabaseIO* db, const std::string& blockName, const int64_t elementCount) const override
  {
    // for mixed topology blocks, IOSS uses "unknown"
    const auto* mixed_topo = Ioss::ElementTopology::factory("unknown");
    const auto& elementType = mixed_topo->name();
    auto* sideBlock =
      new Ioss::SideBlock(db, "sideblock_0", elementType, elementType, elementCount);
    auto* sideSet = new Ioss::SideSet(db, blockName);
    sideSet->add(sideBlock);
    return sideSet;
  }

  void AddEntity(Ioss::Region& region, Ioss::GroupingEntity* entitySet) const override
  {
    region.add(dynamic_cast<Ioss::SideSet*>(entitySet));
  }

  Ioss::GroupingEntity* GetEntity(Ioss::Region& region, const std::string& blockName) const override
  {
    return region.get_sideset(blockName)->get_side_block("sideblock_0");
  }
};

//=============================================================================
class vtkIOSSModel::vtkInternals
{
public:
  vtkSmartPointer<vtkMultiProcessController> Controller;
  vtkSmartPointer<vtkPartitionedDataSetCollection> DataSet;
  std::multimap<Ioss::EntityType, std::shared_ptr<vtkGroupingEntity>> EntityGroups;
  ErrorHandleInformation PointInfo;
  ErrorHandleInformation CellInfo;
  ErrorHandleInformation ElementSideInfo;
};

//----------------------------------------------------------------------------
vtkIOSSModel::vtkIOSSModel(vtkPartitionedDataSetCollection* pdc, vtkIOSSWriter* writer)
  : Internals(new vtkIOSSModel::vtkInternals())
{
  auto& internals = (*this->Internals);
  auto& dataset = internals.DataSet;
  internals.Controller = writer->GetController()
    ? vtk::MakeSmartPointer(writer->GetController())
    : vtk::TakeSmartPointer(vtkMultiProcessController::SafeDownCast(vtkDummyController::New()));
  const auto& controller = internals.Controller;
  auto& entityGroups = internals.EntityGroups;

  // shallow copy the dataset. because we might need to add global IDs to it.
  dataset = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
  dataset->CopyStructure(pdc);
  dataset->ShallowCopy(pdc);

  // detect which vtkPartitionedDataSets are element blocks, node sets, and side sets.
  const auto assemblyName = writer->GetAssemblyName();
  vtkSmartPointer<vtkDataAssembly> assembly;
  if (assemblyName && strcmp(assemblyName, "Assembly") == 0)
  {
    assembly = dataset->GetDataAssembly();
  }
  else // if (strcmp(assemblyName, "vtkDataAssemblyUtilities::HierarchyName()") == 0)
  {
    assembly = vtkSmartPointer<vtkDataAssembly>::New();
    if (!vtkDataAssemblyUtilities::GenerateHierarchy(dataset, assembly))
    {
      vtkErrorWithObjectMacro(writer, "Failed to generate hierarchy.");
      return;
    }
  }
  using EntityType = vtkIOSSWriter::EntityType;
  std::map<EntityType, std::set<unsigned int>> entityIndices;
  for (int i = EntityType::EDGEBLOCK; i < EntityType::NUMBER_OF_ENTITY_TYPES; ++i)
  {
    const auto entityType = static_cast<EntityType>(i);
    entityIndices[entityType] = ::GetDatasetIndices(assembly, writer->GetSelectors(entityType));
  }
  // write the above for loop with one line
  const bool indicesEmpty = std::all_of(entityIndices.begin(), entityIndices.end(),
    [](const std::pair<EntityType, std::set<unsigned int>>& indices) {
      return indices.second.empty();
    });
  if (indicesEmpty)
  {
    // if no indices are specified, then all blocks will be processed as element blocks
    // but, if the dataset was read from vtkIOSSReader, then we can deduce the type of the block
    const auto dataAssembly = dataset->GetDataAssembly();
    const bool isIOSS = (dataAssembly && dataAssembly->GetRootNodeName() &&
      strcmp(dataAssembly->GetRootNodeName(), "IOSS") == 0);
    if (isIOSS)
    {
      for (int i = EntityType::EDGEBLOCK; i < EntityType::NUMBER_OF_ENTITY_TYPES; ++i)
      {
        const auto entityType = static_cast<EntityType>(i);
        const auto iossEntitySelector =
          std::string("/IOSS/") + vtkIOSSReader::GetDataAssemblyNodeNameForEntityType(i);
        entityIndices[entityType] = ::GetDatasetIndices(dataAssembly, { iossEntitySelector });
      }
    }
    else
    {
      // all blocks are element blocks
      entityIndices[EntityType::ELEMENTBLOCK] = ::GetDatasetIndices(assembly, { "/" });
    }
  }

  // create sets used for handling global IDs and element ids
  std::set<unsigned int> setIndices;
  for (int i = EntityType::SET_START; i < EntityType::SET_END; ++i)
  {
    const auto entityType = static_cast<EntityType>(i);
    setIndices.insert(entityIndices[entityType].begin(), entityIndices[entityType].end());
  }
  std::set<unsigned int> setIndicesWithElementSide;
  for (int i = EntityType::EDGESET; i < EntityType::SET_END; ++i)
  {
    const auto entityType = static_cast<EntityType>(i);
    setIndicesWithElementSide.insert(
      entityIndices[entityType].begin(), entityIndices[entityType].end());
  }
  std::set<unsigned int> blockIndices;
  for (int i = EntityType::EDGEBLOCK; i < EntityType::BLOCK_END; ++i)
  {
    const auto entityType = static_cast<EntityType>(i);
    blockIndices.insert(entityIndices[entityType].begin(), entityIndices[entityType].end());
  }

  // create global point ids if needed
  internals.PointInfo = ::HandleGlobalIds(dataset, vtkDataObject::POINT, {}, controller, writer);
  // create global cell ids if needed (sets should not have global cell ids)
  internals.CellInfo =
    ::HandleGlobalIds(dataset, vtkDataObject::CELL, setIndices, controller, writer);
  // create element_side if needed and if it's possible
  if (!setIndicesWithElementSide.empty())
  {
    internals.ElementSideInfo = ::HandleElementSide(
      dataset, internals.CellInfo, setIndicesWithElementSide, blockIndices, controller, writer);
  }

  // extract the names and ids of the blocks.
  std::vector<std::string> blockNames(dataset->GetNumberOfPartitionedDataSets());
  std::vector<int> blockIds(dataset->GetNumberOfPartitionedDataSets());
  for (unsigned int pidx = 0; pidx < dataset->GetNumberOfPartitionedDataSets(); ++pidx)
  {
    blockIds[pidx] = pidx + 1;
    blockNames[pidx] = "block_" + std::to_string(blockIds[pidx]);
    if (auto info = dataset->GetMetaData(pidx))
    {
      if (info->Has(vtkCompositeDataSet::NAME()))
      {
        blockNames[pidx] = info->Get(vtkCompositeDataSet::NAME());
      }
      // this is true only if the dataset is coming from IOSS reader.
      if (info->Has(vtkIOSSReader::ENTITY_ID()))
      {
        blockIds[pidx] = info->Get(vtkIOSSReader::ENTITY_ID());
      }
    }
  }
  // this will be used as a start id for split blocks to ensure uniqueness.
  int startSplitEBlockId = *std::max_element(blockIds.begin(), blockIds.end()) + 1;
  // ensure that all processes have the same startSplitEBlockId.
  if (controller && controller->GetNumberOfProcesses() > 1)
  {
    int globalStartSplitBlockId;
    controller->AllReduce(
      &startSplitEBlockId, &globalStartSplitBlockId, 1, vtkCommunicator::MAX_OP);
    startSplitEBlockId = globalStartSplitBlockId;
  }

  // first things first, determine all information necessary about node block.
  // there's just 1 node block for exodus, build that.
  auto nodeBlock = std::make_shared<vtkNodeBlock>(dataset, "nodeblock_1", controller, writer);
  entityGroups.emplace(nodeBlock->GetIOSSEntityType(), nodeBlock);

  // process group entities.
  int blockCounter = 0;
  for (unsigned int pidx = 0; pidx < dataset->GetNumberOfPartitionedDataSets(); ++pidx)
  {
    const std::string& blockName = blockNames[pidx];
    const int& blockId = blockIds[pidx];
    const auto pds = dataset->GetPartitionedDataSet(pidx);

    // now create each type of GroupingEntity.

    // edge block
    const bool edgeBlockFound =
      entityIndices[EntityType::EDGEBLOCK].find(pidx) != entityIndices[EntityType::EDGEBLOCK].end();
    if (edgeBlockFound)
    {
      try
      {
        if (blockCounter++ != 0)
        {
          // add the number of cell types to the block id to ensure uniqueness.
          startSplitEBlockId += VTK_NUMBER_OF_CELL_TYPES;
        }
        auto edgeBlock = std::make_shared<vtkEdgeBlock>(
          pds, blockName, blockId, startSplitEBlockId, controller, writer);
        entityGroups.emplace(edgeBlock->GetIOSSEntityType(), edgeBlock);
        continue;
      }
      catch (std::runtime_error&)
      {
        // since global IDs are either present, created or modified, we should not get here.
        break;
      }
    }

    // face block
    const bool faceBlockFound =
      entityIndices[EntityType::FACEBLOCK].find(pidx) != entityIndices[EntityType::FACEBLOCK].end();
    if (faceBlockFound)
    {
      try
      {
        if (blockCounter++ != 0)
        {
          // add the number of cell types to the block id to ensure uniqueness.
          startSplitEBlockId += VTK_NUMBER_OF_CELL_TYPES;
        }
        auto faceBlock = std::make_shared<vtkFaceBlock>(
          pds, blockName, blockId, startSplitEBlockId, controller, writer);
        entityGroups.emplace(faceBlock->GetIOSSEntityType(), faceBlock);
        continue;
      }
      catch (std::runtime_error&)
      {
        // since global IDs are either present, created or modified, we should not get here.
        break;
      }
    }

    // element block
    const bool elementBlockFound = entityIndices[EntityType::ELEMENTBLOCK].find(pidx) !=
      entityIndices[EntityType::ELEMENTBLOCK].end();
    if (elementBlockFound)
    {
      try
      {
        if (blockCounter++ != 0)
        {
          // add the number of cell types to the block id to ensure uniqueness.
          startSplitEBlockId += VTK_NUMBER_OF_CELL_TYPES;
        }
        auto elementBlock = std::make_shared<vtkElementBlock>(
          pds, blockName, blockId, startSplitEBlockId, controller, writer);
        entityGroups.emplace(elementBlock->GetIOSSEntityType(), elementBlock);
        continue;
      }
      catch (std::runtime_error&)
      {
        // since global IDs are either present, created or modified, we should not get here.
        break;
      }
    }

    // node set
    const bool nodeSetFound =
      entityIndices[EntityType::NODESET].find(pidx) != entityIndices[EntityType::NODESET].end();
    if (nodeSetFound)
    {
      try
      {
        auto nodeSet = std::make_shared<vtkNodeSet>(pds, blockName, blockId, controller, writer);
        entityGroups.emplace(nodeSet->GetIOSSEntityType(), nodeSet);
        continue;
      }
      catch (std::runtime_error&)
      {
        // since global IDs are either present, created or modified, we should not get here.
        break;
      }
    }

    // edge set
    const bool edgeSetFound =
      entityIndices[EntityType::EDGESET].find(pidx) != entityIndices[EntityType::EDGESET].end();
    if (edgeSetFound && !internals.ElementSideInfo.GetHadIssues())
    {
      try
      {
        auto edgeSet = std::make_shared<vtkEdgeSet>(pds, blockName, blockId, controller, writer);
        entityGroups.emplace(edgeSet->GetIOSSEntityType(), edgeSet);
        continue;
      }
      catch (std::runtime_error&)
      {
        // since the existence of issues with element_side is checked, we should not get here.
        break;
      }
    }

    // face set
    const bool faceSetFound =
      entityIndices[EntityType::FACESET].find(pidx) != entityIndices[EntityType::FACESET].end();
    if (faceSetFound && !internals.ElementSideInfo.GetHadIssues())
    {
      try
      {
        auto faceSet = std::make_shared<vtkFaceSet>(pds, blockName, blockId, controller, writer);
        entityGroups.emplace(faceSet->GetIOSSEntityType(), faceSet);
        continue;
      }
      catch (std::runtime_error&)
      {
        // since the existence of issues with element_side is checked, we should not get here.
        break;
      }
    }

    // element set
    const bool elementSetFound = entityIndices[EntityType::ELEMENTSET].find(pidx) !=
      entityIndices[EntityType::ELEMENTSET].end();
    if (elementSetFound && !internals.ElementSideInfo.GetHadIssues())
    {
      try
      {
        auto elementSet =
          std::make_shared<vtkElementSet>(pds, blockName, blockId, controller, writer);
        entityGroups.emplace(elementSet->GetIOSSEntityType(), elementSet);
        continue;
      }
      catch (std::runtime_error&)
      {
        // since the existence of issues with element_side is checked, we should not get here.
        break;
      }
    }

    // side set
    const bool sideSetFound =
      entityIndices[EntityType::SIDESET].find(pidx) != entityIndices[EntityType::SIDESET].end();
    if (sideSetFound && !internals.ElementSideInfo.GetHadIssues())
    {
      try
      {
        auto sideSet = std::make_shared<vtkSideSet>(pds, blockName, blockId, controller, writer);
        entityGroups.emplace(sideSet->GetIOSSEntityType(), sideSet);
        continue;
      }
      catch (std::runtime_error&)
      {
        // since the existence of issues with element_side is checked, we should not get here.
        break;
      }
    }
  }
}

//----------------------------------------------------------------------------
vtkIOSSModel::~vtkIOSSModel() = default;

//----------------------------------------------------------------------------
void vtkIOSSModel::DefineModel(Ioss::Region& region) const
{
  auto& internals = (*this->Internals);
  region.begin_mode(Ioss::STATE_DEFINE_MODEL);
  for (auto& entity : internals.EntityGroups)
  {
    entity.second->DefineModel(region);
  }
  region.end_mode(Ioss::STATE_DEFINE_MODEL);
}

//----------------------------------------------------------------------------
void vtkIOSSModel::Model(Ioss::Region& region) const
{
  auto& internals = (*this->Internals);
  region.begin_mode(Ioss::STATE_MODEL);
  for (auto& entity : internals.EntityGroups)
  {
    entity.second->Model(region);
  }
  region.end_mode(Ioss::STATE_MODEL);
}

//----------------------------------------------------------------------------
void vtkIOSSModel::DefineTransient(Ioss::Region& region) const
{
  auto& internals = (*this->Internals);
  region.begin_mode(Ioss::STATE_DEFINE_TRANSIENT);
  for (auto& entity : internals.EntityGroups)
  {
    entity.second->DefineTransient(region);
  }
  region.end_mode(Ioss::STATE_DEFINE_TRANSIENT);
}

//----------------------------------------------------------------------------
void vtkIOSSModel::Transient(Ioss::Region& region, double time) const
{
  auto& internals = (*this->Internals);
  region.begin_mode(Ioss::STATE_TRANSIENT);
  const int step = region.add_state(time);
  region.begin_state(step);
  for (auto& entity : internals.EntityGroups)
  {
    entity.second->Transient(region);
  }
  region.end_state(step);
  region.end_mode(Ioss::STATE_TRANSIENT);
}

//----------------------------------------------------------------------------
std::string vtkIOSSModel::MD5() const
{
  unsigned char digest[16];
  char md5Hash[33];

  vtksysMD5* md5 = vtksysMD5_New();
  vtksysMD5_Initialize(md5);

  const auto& internals = (*this->Internals);
  size_t numberOfItems = internals.EntityGroups.size();
  vtksysMD5_Append(
    md5, reinterpret_cast<const unsigned char*>(&numberOfItems), static_cast<int>(sizeof(size_t)));

  for (const auto& entity : internals.EntityGroups)
  {
    entity.second->AppendMD5(md5);
  }

  vtksysMD5_Finalize(md5, digest);
  vtksysMD5_DigestToHex(digest, md5Hash);
  vtksysMD5_Delete(md5);
  md5Hash[32] = '\0';
  return std::string(md5Hash);
}

//----------------------------------------------------------------------------
bool vtkIOSSModel::GlobalIdsCreated() const
{
  return this->Internals->PointInfo.GetCreated() || this->Internals->CellInfo.GetCreated();
}

//----------------------------------------------------------------------------
bool vtkIOSSModel::GlobalIdsModified() const
{
  return this->Internals->PointInfo.GetModified() || this->Internals->CellInfo.GetModified();
}

//----------------------------------------------------------------------------
bool vtkIOSSModel::ElementSideCouldNotBeCreated() const
{
  return this->Internals->ElementSideInfo.GetCouldNotBeCreated();
}

//----------------------------------------------------------------------------
bool vtkIOSSModel::ElementSideCouldNotBeModified() const
{
  return this->Internals->ElementSideInfo.GetCouldNotBeModified();
}

//----------------------------------------------------------------------------
bool vtkIOSSModel::ElementSideModified() const
{
  return this->Internals->ElementSideInfo.GetModified();
}
VTK_ABI_NAMESPACE_END
