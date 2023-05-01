/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIOSSModel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIOSSModel.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
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
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <vtksys/MD5.h>

// Ioss includes
#include <vtk_ioss.h>
// clang-format off
#include VTK_IOSS(Ioss_Assembly.h)
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

namespace
{
//=============================================================================
bool HandleGlobalIds(
  vtkPartitionedDataSetCollection* pdc, int association, vtkMultiProcessController* controller)
{
  const auto datasets = vtkCompositeDataSet::GetDataSets<vtkUnstructuredGrid>(pdc);
  // check if global ids are present, otherwise create them.
  int hasGlobalIds = true;
  for (auto& ds : datasets)
  {
    auto* pd = ds->GetAttributes(association);
    auto* gids = vtkIdTypeArray::SafeDownCast(pd->GetGlobalIds());
    if (!gids)
    {
      hasGlobalIds = false;
      break;
    }
  }
  if (controller->GetNumberOfProcesses() > 1)
  {
    controller->AllReduce(&hasGlobalIds, &hasGlobalIds, 1, vtkCommunicator::MIN_OP);
  }

  if (!hasGlobalIds)
  {
    const auto numElements = std::accumulate(datasets.begin(), datasets.end(), 0,
      [&](int sum, vtkUnstructuredGrid* ds) { return sum + ds->GetNumberOfElements(association); });

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
    for (auto& ds : datasets)
    {
      const auto numberOfElements = ds->GetNumberOfElements(association);
      vtkNew<vtkIdTypeArray> globalIds;
      globalIds->SetName("ids");
      globalIds->SetNumberOfComponents(1);
      globalIds->SetNumberOfTuples(numberOfElements);
      vtkSMPTools::For(0, numberOfElements, [&](vtkIdType begin, vtkIdType end) {
        auto globalIdsPtr = globalIds->GetPointer(0);
        for (vtkIdType i = begin; i < end; ++i)
        {
          globalIdsPtr[i] = startId + i;
        }
      });
      ds->GetAttributes(association)->SetGlobalIds(globalIds);
      startId += numberOfElements;
    }
  }
  // returns if globals were created or not.
  return !hasGlobalIds;
}

//=============================================================================
std::set<unsigned int> GetDatasetIndices(vtkDataAssembly* assembly, std::vector<std::string> paths)
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
  auto datasets = vtkCompositeDataSet::GetDataSets<vtkUnstructuredGrid>(pd);
  for (auto& ug : datasets)
  {
    auto* distinctCellTypes = ug->GetDistinctCellTypesArray();
    auto range = vtk::DataArrayValueRange(distinctCellTypes);
    std::copy(range.begin(), range.end(), std::inserter(cellTypes, cellTypes.end()));
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

  for (auto& ug : datasets)
  {
    const unsigned char* cellTypesPtr = ug->GetCellTypesArray()->GetPointer(0);
    vtkSMPTools::For(0, ug->GetNumberOfCells(), [&](vtkIdType start, vtkIdType end) {
      for (vtkIdType cc = start; cc < end; ++cc)
      {
        // memory_order_relaxed is safe here, since we're not using the atomics for synchronization.
        elementCounts[cellTypesPtr[cc]].fetch_add(1, std::memory_order_relaxed);
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
std::vector<std::tuple<std::string, Ioss::Field::BasicType, int>> GetFields(
  int association, vtkCompositeDataSet* cds, vtkMultiProcessController* controller)
{
  std::vector<std::tuple<std::string, Ioss::Field::BasicType, int>> fields;
  vtkDataSetAttributesFieldList fieldList;
  for (auto& ds : vtkCompositeDataSet::GetDataSets<vtkUnstructuredGrid>(cds))
  {
    fieldList.IntersectFieldList(ds->GetAttributes(association));
  }

  vtkNew<vtkDataSetAttributes> tmpDA;
  tmpDA->CopyAllocate(fieldList, 1);
  tmpDA->SetNumberOfTuples(1);
  if (tmpDA->GetGlobalIds())
  {
    // we don't want to add global ids again.
    tmpDA->RemoveArray(tmpDA->GetGlobalIds()->GetName());
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
  if (controller->GetNumberOfProcesses() == 1)
  {
    for (int idx = 0, max = tmpDA->GetNumberOfArrays(); idx < max; ++idx)
    {
      if (auto array = tmpDA->GetArray(idx))
      {
        const auto type = ::GetFieldType(array);
        fields.emplace_back(array->GetName(), type, array->GetNumberOfComponents());
      }
    }
  }
  else // controller->GetNumberOfProcesses() > 1
  {
    // gather the number of elements from all ranks.
    vtkNew<vtkIdTypeArray> sendNumberOfElements;
    sendNumberOfElements->InsertNextValue(cds->GetNumberOfElements(association));
    vtkNew<vtkIdTypeArray> recvNumberOfElements;
    controller->AllGather(sendNumberOfElements, recvNumberOfElements);
    // create an unstructured grid to pack the tmpDA as field data.
    auto send = vtkSmartPointer<vtkUnstructuredGrid>::New();
    send->GetFieldData()->ShallowCopy(tmpDA);
    // now gather all field data from all ranks.
    std::vector<vtkSmartPointer<vtkDataObject>> recv;
    controller->AllGather(send, recv);
    // now intersect all field data to get the common fields.
    vtkDataSetAttributesFieldList coresFieldList;
    for (size_t i = 0; i < recv.size(); ++i)
    {
      const auto ug = vtkUnstructuredGrid::SafeDownCast(recv[i]);
      const auto numberOfElements = recvNumberOfElements->GetValue(i);
      // skip empty datasets.
      if (ug && numberOfElements > 0)
      {
        const auto fd = ug->GetFieldData();
        // convert field data to dataset attributes
        vtkNew<vtkDataSetAttributes> localDa;
        for (int idx = 0, max = fd->GetNumberOfArrays(); idx < max; ++idx)
        {
          if (auto array = fd->GetArray(idx))
          {
            localDa->AddArray(array);
          }
        }
        // intersect field data with current field list.
        coresFieldList.IntersectFieldList(localDa);
      }
    }
    // now we have the common fields. we need to create a new field data
    vtkNew<vtkDataSetAttributes> coresTempDA;
    coresTempDA->CopyAllocate(coresFieldList, 0);
    for (int idx = 0, max = coresTempDA->GetNumberOfArrays(); idx < max; ++idx)
    {
      if (auto* array = coresTempDA->GetArray(idx))
      {
        const auto type = ::GetFieldType(array);
        fields.emplace_back(array->GetName(), type, array->GetNumberOfComponents());
      }
    }
  }
  return fields;
}

//=============================================================================
template <typename T>
struct PutFieldWorker
{
  std::vector<std::vector<T>> Data;
  size_t Offset{ 0 };
  const std::vector<vtkIdType>* SourceIds = nullptr;
  PutFieldWorker(int numComponents, size_t targetSize)
    : Data(numComponents)
  {
    for (int cc = 0; cc < numComponents; ++cc)
    {
      this->Data[cc].resize(targetSize);
    }
  }

  void SetSourceIds(const std::vector<vtkIdType>* ids) { this->SourceIds = ids; }

  template <typename ArrayType>
  void operator()(ArrayType* array)
  {
    using SourceT = vtk::GetAPIType<ArrayType>;
    vtkSMPTools::For(0, this->SourceIds->size(), [&](vtkIdType start, vtkIdType end) {
      SourceT* tuple = new SourceT[this->Data.size()];
      for (vtkIdType cc = start; cc < end; ++cc)
      {
        array->GetTypedTuple((*this->SourceIds)[cc], tuple);
        for (size_t comp = 0; comp < this->Data.size(); ++comp)
        {
          this->Data[comp][this->Offset + cc] = tuple[comp];
        }
      }
      delete[] tuple;
    });

    this->Offset += this->SourceIds->size();
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
  virtual Ioss::EntityType GetEntityType() const = 0;
  virtual void Define(Ioss::Region& region) const = 0;
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
      switch (std::get<1>(field))
      {
        case Ioss::Field::DOUBLE:
          this->PutField<double>(
            block, std::get<0>(field), std::get<2>(field), lIds, datasets, association);
          break;

        case Ioss::Field::INT32:
          this->PutField<int32_t>(
            block, std::get<0>(field), std::get<2>(field), lIds, datasets, association);
          break;

        case Ioss::Field::INT64:
          this->PutField<int64_t>(
            block, std::get<0>(field), std::get<2>(field), lIds, datasets, association);
          break;

        default:
          vtkLogF(TRACE, "Unsupported field type. Skipping %s", std::get<0>(field).c_str());
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
    PutFieldWorker<T> worker(numComponents, totalSize);
    for (size_t dsIndex = 0; dsIndex < datasets.size(); ++dsIndex)
    {
      auto& ds = datasets[dsIndex];
      auto& lids = lIds[dsIndex];
      worker.SetSourceIds(&lids);
      if (!Dispatcher::Execute(ds->GetAttributes(association)->GetArray(name.c_str()), worker))
      {
        vtkLogF(ERROR, "Failed to dispatch array %s", name.c_str());
      }
    }

    for (int comp = 0; comp < numComponents; ++comp)
    {
      const auto fieldName = numComponents == 1 ? name : name + std::to_string(comp + 1);
      block->put_field_data(fieldName, worker.Data[comp]);
    }
  }

  void DefineFields(Ioss::GroupingEntity* block,
    const std::vector<std::tuple<std::string, Ioss::Field::BasicType, int>>& fields,
    Ioss::Field::RoleType role, int64_t elementCount) const
  {
    for (const auto& field : fields)
    {
      if (std::get<2>(field) == 1)
      {
        block->field_add(
          Ioss::Field(std::get<0>(field), std::get<1>(field), "scalar", role, elementCount));
      }
      else
      {
        for (int comp = 0; comp < std::get<2>(field); ++comp)
        {
          block->field_add(Ioss::Field(std::get<0>(field) + std::to_string(comp + 1),
            std::get<1>(field), "scalar", role, elementCount));
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
  const std::vector<vtkUnstructuredGrid*> DataSets;
  const std::string Name;

  // build a map of ds idx, gid, and lid and use that later.
  std::vector<int32_t> Ids;
  std::vector<std::vector<vtkIdType>> IdsRaw;

  std::vector<std::tuple<std::string, Ioss::Field::BasicType, int>> Fields;

  vtkNodeBlock(vtkPartitionedDataSetCollection* pdc, const std::string& name,
    vtkMultiProcessController* controller, vtkIOSSWriter* writer)
    : vtkGroupingEntity(writer)
    , DataSets{ vtkCompositeDataSet::GetDataSets<vtkUnstructuredGrid>(pdc) }
    , Name(name)
  {
    this->IdsRaw.reserve(this->DataSets.size());

    std::set<int32_t> id_set;
    for (auto& ds : this->DataSets)
    {
      auto* pd = ds->GetPointData();
      auto* gids = vtkIdTypeArray::SafeDownCast(pd->GetGlobalIds());
      if (!gids)
      {
        throw std::runtime_error("point global ids missing.");
      }

      const auto numPoints = ds->GetNumberOfPoints();
      assert(gids->GetNumberOfTuples() == numPoints);

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
    this->Fields = ::GetFields(vtkDataObject::POINT, pdc, controller);
  }

  Ioss::EntityType GetEntityType() const override { return Ioss::EntityType::NODEBLOCK; }

  void AppendMD5(vtksysMD5* md5) const override
  {
    vtksysMD5_Append(md5, reinterpret_cast<const unsigned char*>(this->Ids.data()),
      static_cast<int>(sizeof(int32_t) * this->Ids.size()));
  }

  void Define(Ioss::Region& region) const override
  {
    auto* nodeBlock = new Ioss::NodeBlock(region.get_database(), this->Name, this->Ids.size(), 3);
    nodeBlock->property_add(Ioss::Property("id", 1)); // block id.
    region.add(nodeBlock);
  }

  void DefineTransient(Ioss::Region& region) const override
  {
    auto* block = region.get_node_block(this->Name);
    this->DefineFields(block, this->Fields, Ioss::Field::TRANSIENT, this->Ids.size());
  }

  void Model(Ioss::Region& region) const override
  {
    auto* block = region.get_node_block(this->Name);
    block->put_field_data("ids", this->Ids);

    // add mesh coordinates
    using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
    PutFieldWorker<double> worker(3, this->Ids.size());
    for (size_t dsIndex = 0; dsIndex < this->DataSets.size(); ++dsIndex)
    {
      auto& ds = this->DataSets[dsIndex];
      auto& lids = this->IdsRaw[dsIndex];
      worker.SetSourceIds(&lids);
      if (!Dispatcher::Execute(ds->GetPoints()->GetData(), worker))
      {
        vtkLog(ERROR, "Failed to dispatch points.");
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
      DisplacementWorker<double> dworker(worker.Data, displMagnitude);
      for (size_t dsIndex = 0; dsIndex < this->DataSets.size(); ++dsIndex)
      {
        auto& ds = this->DataSets[dsIndex];
        auto& lids = this->IdsRaw[dsIndex];
        dworker.SetSourceIds(&lids);
        if (!Dispatcher::Execute(ds->GetPointData()->GetArray(displName.c_str()), dworker))
        {
          vtkLog(ERROR, "Failed to dispatch displacements.");
        }
      }
    }

    block->put_field_data("mesh_model_coordinates_x", worker.Data[0]);
    block->put_field_data("mesh_model_coordinates_y", worker.Data[1]);
    block->put_field_data("mesh_model_coordinates_z", worker.Data[2]);
  }

  void Transient(Ioss::Region& region) const override
  {
    auto* block = region.get_node_block(this->Name);
    this->PutFields(block, this->Fields, this->IdsRaw, this->DataSets, vtkDataObject::POINT);
  }
};

/**
 * Builds a Ioss::ElementBlock from a vtkPartitionedDataSet. The differences
 * between the Ioss and VTK data model for the two are handled as follows:
 *
 * * We only support vtkPartitionedDataSet comprising of one or more vtkUnstructuredGrids.
 *   All other dataset types are simply ignored.
 *
 * * An ElementBlock cannot have multiple "pieces" in the same file. So if a
 *   vtkPartitionedDataSet has multiple datasets, we need to "combine" them into
 *   one.
 *
 * * An ElementBlock cannot have elements of different types. However,
 *   vtkUnstructuredGrid supports heterogeneous cells. So if all
 *   vtkUnstructuredGrids in the vtkPartitionedDataSet have more than 1 cell type,
 *   we create multiple element blocks. Each ElementBlock is uniquely named by
 *   using the given block name and the element type as a suffix.
 *
 *   In MPI world, the cell types are gathered across all ranks to ensure each
 *   ranks creates identical blocks / block names.
 *
 */
struct vtkElementBlock : public vtkGroupingEntity
{
  const std::vector<vtkUnstructuredGrid*> DataSets;
  std::string RootName;
  int BlockId;
  int StartSplitElementBlockId;
  std::map<unsigned char, int64_t> ElementCounts;
  std::vector<std::tuple<std::string, Ioss::Field::BasicType, int>> Fields;

  vtkElementBlock(vtkPartitionedDataSet* pd, const std::string& name, const int blockId,
    int startSplitElementBlockId, vtkMultiProcessController* controller, vtkIOSSWriter* writer)
    : vtkGroupingEntity(writer)
    , DataSets{ vtkCompositeDataSet::GetDataSets<vtkUnstructuredGrid>(pd) }
    , RootName{ name }
    , BlockId{ blockId }
    , StartSplitElementBlockId{ startSplitElementBlockId }
  {
    auto datasets = vtkCompositeDataSet::GetDataSets<vtkUnstructuredGrid>(pd);
    for (auto& ug : this->DataSets)
    {
      auto* cd = ug->GetCellData();
      auto* gids = vtkIdTypeArray::SafeDownCast(cd->GetGlobalIds());
      if (!gids)
      {
        throw std::runtime_error("cell global ids missing!");
      }
    }

    this->ElementCounts = ::GetElementCounts(pd, controller);
    this->Fields = ::GetFields(vtkDataObject::CELL, pd, controller);
  }

  Ioss::EntityType GetEntityType() const override { return Ioss::EntityType::ELEMENTBLOCK; }

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

  void Define(Ioss::Region& region) const override
  {
    for (const auto& element : this->ElementCounts)
    {
      const int64_t elementCount = element.second;
      const unsigned char vtk_cell_type = element.first;

      const auto* elementTopology = vtkIOSSUtilities::GetElementTopology(vtk_cell_type);
      const auto& elementType = elementTopology->name();
      const auto blockInfo = this->GetSubElementBlockInfo(vtk_cell_type, elementType);

      auto* elementBlock =
        new Ioss::ElementBlock(region.get_database(), blockInfo.second, elementType, elementCount);
      elementBlock->property_add(Ioss::Property("id", blockInfo.first));
      if (this->Writer->GetPreserveOriginalIds())
      {
        elementBlock->property_add(
          Ioss::Property("original_id", this->BlockId, Ioss::Property::ATTRIBUTE));
      }
      region.add(elementBlock);
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

      auto* elementBlock = region.get_element_block(blockName);
      this->DefineFields(elementBlock, this->Fields, Ioss::Field::TRANSIENT, elementCount);
    }
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

      auto* elementBlock = region.get_element_block(blockName);

      // populate ids.
      std::vector<int32_t> elementIds; // these are global ids.
      elementIds.reserve(elementCount);

      std::vector<int32_t> connectivity;
      connectivity.reserve(elementCount * nodeCount);

      const int32_t gidOffset = this->Writer->GetOffsetGlobalIds() ? 1 : 0;
      const bool removeGhosts = this->Writer->GetRemoveGhosts();
      for (auto& ug : this->DataSets)
      {
        vtkUnsignedCharArray* ghost = ug->GetCellGhostArray();
        auto* gids = vtkIdTypeArray::SafeDownCast(ug->GetCellData()->GetGlobalIds());
        auto* pointGIDs = vtkIdTypeArray::SafeDownCast(ug->GetPointData()->GetGlobalIds());

        for (vtkIdType cc = 0, max = ug->GetNumberOfCells(); cc < max; ++cc)
        {
          const bool process = !removeGhosts || !ghost || ghost->GetValue(cc) == 0;
          if (process && ug->GetCellType(cc) == vtk_cell_type)
          {
            elementIds.push_back(gidOffset + gids->GetValue(cc));

            vtkIdType numPts;
            vtkIdType const* cellPoints;
            ug->GetCellPoints(cc, numPts, cellPoints);
            assert(numPts == nodeCount);

            // map cell's point to global ids for those points.
            std::transform(cellPoints, cellPoints + numPts, std::back_inserter(connectivity),
              [&](vtkIdType ptid) { return gidOffset + pointGIDs->GetValue(ptid); });
          }
        }
      }
      assert(elementIds.size() == static_cast<size_t>(elementCount));
      assert(connectivity.size() == static_cast<size_t>(elementCount * nodeCount));
      elementBlock->put_field_data("ids", elementIds);
      elementBlock->put_field_data("connectivity", connectivity);
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

      auto* elementBlock = region.get_element_block(blockName);

      // populate ids.
      std::vector<std::vector<vtkIdType>> lIds; // these are local ids.
      const bool removeGhosts = this->Writer->GetRemoveGhosts();
      for (auto& ug : this->DataSets)
      {
        vtkUnsignedCharArray* ghost = ug->GetCellGhostArray();
        lIds.emplace_back();
        lIds.back().reserve(ug->GetNumberOfCells());
        for (vtkIdType cc = 0, max = ug->GetNumberOfCells(); cc < max; ++cc)
        {
          const bool process = !removeGhosts || !ghost || ghost->GetValue(cc) == 0;
          if (process && ug->GetCellType(cc) == vtk_cell_type)
          {
            lIds.back().push_back(cc);
          }
        }
      }

      // add fields.
      this->PutFields(elementBlock, this->Fields, lIds, this->DataSets, vtkDataObject::CELL);
    }
  }
};

struct vtkNodeSet : public vtkGroupingEntity
{
  vtkPartitionedDataSet* PartitionedDataSet;
  std::string Name;
  int BlockId;
  int64_t Count{ 0 };
  vtkNodeSet(vtkPartitionedDataSet* pd, const std::string& name, int blockId,
    vtkMultiProcessController* vtkNotUsed(controller), vtkIOSSWriter* writer)
    : vtkGroupingEntity(writer)
    , PartitionedDataSet(pd)
    , Name(name)
    , BlockId(blockId)
  {
    auto datasets = vtkCompositeDataSet::GetDataSets<vtkUnstructuredGrid>(pd);
    for (auto& ug : datasets)
    {
      auto* gids = vtkIdTypeArray::SafeDownCast(ug->GetPointData()->GetGlobalIds());
      if (!gids)
      {
        throw std::runtime_error("missing point global ids for nodesets.");
      }
      this->Count += ug->GetNumberOfPoints();
    }
    // TODO: identify nodeset-only fields.
  }

  Ioss::EntityType GetEntityType() const override { return Ioss::EntityType::NODESET; }

  void AppendMD5(vtksysMD5* md5) const override
  {
    vtksysMD5_Append(md5, reinterpret_cast<const unsigned char*>(this->Name.c_str()), -1);
    vtksysMD5_Append(md5, reinterpret_cast<const unsigned char*>(&this->Count),
      static_cast<int>(sizeof(this->Count)));
  }

  void Define(Ioss::Region& region) const override
  {
    auto* nodeset = new Ioss::NodeSet(region.get_database(), this->Name, this->Count);
    nodeset->property_add(Ioss::Property("id", this->BlockId));
    region.add(nodeset);
  }

  void DefineTransient(Ioss::Region& vtkNotUsed(region)) const override {}

  void Model(Ioss::Region& region) const override
  {
    auto* nodeSet = region.get_nodeset(this->Name);
    std::vector<int32_t> ids;
    ids.reserve(this->Count);
    for (auto& ug : vtkCompositeDataSet::GetDataSets<vtkUnstructuredGrid>(this->PartitionedDataSet))
    {
      auto* gids = vtkIdTypeArray::SafeDownCast(ug->GetPointData()->GetGlobalIds());
      std::copy(
        gids->GetPointer(0), gids->GetPointer(gids->GetNumberOfTuples()), std::back_inserter(ids));
    }
    nodeSet->put_field_data("ids", ids);
  }

  void Transient(Ioss::Region& vtkNotUsed(region)) const override {}
};

struct vtkSideSet : public vtkGroupingEntity
{
  vtkPartitionedDataSet* PartitionedDataSet;
  std::string Name;
  int BlockId;
  int64_t Count;

  vtkSideSet(vtkPartitionedDataSet* pd, const std::string& name, int blockId,
    vtkMultiProcessController* vtkNotUsed(controller), vtkIOSSWriter* writer)
    : vtkGroupingEntity(writer)
    , PartitionedDataSet(pd)
    , Name(name)
    , BlockId(blockId)
    , Count{ 0 }
  {
    auto datasets = vtkCompositeDataSet::GetDataSets<vtkUnstructuredGrid>(pd);
    for (auto& ug : datasets)
    {
      auto* cd = ug->GetCellData();
      if (vtkIdTypeArray::SafeDownCast(cd->GetGlobalIds()) != nullptr)
      {
        throw std::runtime_error("cell global ids present, must not be a sideset.");
      }

      if (vtkIntArray::SafeDownCast(cd->GetArray("element_side")) == nullptr)
      {
        throw std::runtime_error("missing 'element_side' cell array.");
      }

      this->Count += ug->GetNumberOfCells();
    }
  }

  Ioss::EntityType GetEntityType() const override { return Ioss::EntityType::SIDESET; }

  void AppendMD5(vtksysMD5* md5) const override
  {
    vtksysMD5_Append(md5, reinterpret_cast<const unsigned char*>(this->Name.c_str()), -1);
    vtksysMD5_Append(md5, reinterpret_cast<const unsigned char*>(&this->Count),
      static_cast<int>(sizeof(this->Count)));
  }

  void Define(Ioss::Region& region) const override
  {
    // for mixed topology blocks, IOSS uses "unknown"
    const auto* mixed_topo = Ioss::ElementTopology::factory("unknown");
    const auto& elementType = mixed_topo->name();
    auto* sideBlock = new Ioss::SideBlock(
      region.get_database(), "sideblock_0", elementType, elementType, this->Count);

    auto* sideSet = new Ioss::SideSet(region.get_database(), this->Name);
    sideSet->property_add(Ioss::Property("id", this->BlockId));
    sideSet->add(sideBlock);
    region.add(sideSet);
  }

  void DefineTransient(Ioss::Region& vtkNotUsed(region)) const override {}

  void Model(Ioss::Region& region) const override
  {
    auto* sideSet = region.get_sideset(this->Name);
    auto* sideBlock = sideSet->get_side_block("sideblock_0");

    std::vector<int32_t> elementSide;
    elementSide.reserve(this->Count * 2);

    const bool removeGhosts = this->Writer->GetRemoveGhosts();
    auto datasets = vtkCompositeDataSet::GetDataSets<vtkUnstructuredGrid>(this->PartitionedDataSet);
    for (auto& ug : datasets)
    {
      auto elemSideArray = vtkIntArray::SafeDownCast(ug->GetCellData()->GetArray("element_side"));
      if (!elemSideArray)
      {
        throw std::runtime_error("missing 'element_side' cell array.");
      }
      vtkUnsignedCharArray* ghost = ug->GetCellGhostArray();
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
    sideBlock->put_field_data("element_side", elementSide);
  }

  void Transient(Ioss::Region& vtkNotUsed(region)) const override {}
};

//=============================================================================
class vtkIOSSModel::vtkInternals
{
public:
  vtkSmartPointer<vtkMultiProcessController> Controller;
  vtkSmartPointer<vtkPartitionedDataSetCollection> DataSet;
  std::multimap<Ioss::EntityType, std::shared_ptr<vtkGroupingEntity>> EntityGroups;
  bool GlobalIdsCreated;
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

  dataset = vtkSmartPointer<vtkPartitionedDataSetCollection>::New();
  dataset->CopyStructure(pdc);
  dataset->ShallowCopy(pdc);
  internals.GlobalIdsCreated = false;
  // create global point ids if needed
  internals.GlobalIdsCreated |= ::HandleGlobalIds(dataset, vtkDataObject::POINT, controller);
  // create global cell ids if needed
  internals.GlobalIdsCreated |= ::HandleGlobalIds(dataset, vtkDataObject::CELL, controller);

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
  // this will be used as a start id for split element blocks to ensure uniqueness.
  int startSplitElementBlockId = *std::max_element(blockIds.begin(), blockIds.end()) + 1;
  // ensure that all processes have the same startSplitElementBlockId.
  if (controller && controller->GetNumberOfProcesses() > 1)
  {
    int globalStartSplitElementBlockId;
    controller->AllReduce(
      &startSplitElementBlockId, &globalStartSplitElementBlockId, 1, vtkCommunicator::MAX_OP);
    startSplitElementBlockId = globalStartSplitElementBlockId;
  }

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
  std::vector<std::string> elementBlockSelectors(writer->GetNumberOfElementBlockSelectors());
  for (size_t idx = 0; idx < elementBlockSelectors.size(); ++idx)
  {
    elementBlockSelectors[idx] = writer->GetElementBlockSelector(idx);
  }
  std::vector<std::string> nodeSetSelectors(writer->GetNumberOfNodeSetSelectors());
  for (size_t idx = 0; idx < nodeSetSelectors.size(); ++idx)
  {
    nodeSetSelectors[idx] = writer->GetNodeSetSelector(idx);
  }
  std::vector<std::string> sideSetSelectors(writer->GetNumberOfSideSetSelectors());
  for (size_t idx = 0; idx < sideSetSelectors.size(); ++idx)
  {
    sideSetSelectors[idx] = writer->GetSideSetSelector(idx);
  }
  auto elementBlockIndices = ::GetDatasetIndices(assembly, elementBlockSelectors);
  auto nodeSetIndices = ::GetDatasetIndices(assembly, nodeSetSelectors);
  auto sideSetIndices = ::GetDatasetIndices(assembly, sideSetSelectors);
  bool indicesEmpty =
    elementBlockIndices.empty() && nodeSetIndices.empty() && sideSetIndices.empty();
  if (indicesEmpty)
  {
    // if no indices are specified, then all blocks will be processed as element blocks
    // but, if the dataset was read from vtkIOSSReader, then we can deduce the type of the block
    const auto dataAssembly = dataset->GetDataAssembly();
    const bool isIOSS = (dataAssembly && dataAssembly->GetRootNodeName() &&
      strcmp(dataAssembly->GetRootNodeName(), "IOSS") == 0);
    if (isIOSS)
    {
      elementBlockIndices = ::GetDatasetIndices(dataAssembly, { "/IOSS/element_blocks" });
      nodeSetIndices = ::GetDatasetIndices(dataAssembly, { "/IOSS/node_sets" });
      sideSetIndices = ::GetDatasetIndices(dataAssembly, { "/IOSS/side_sets" });
      indicesEmpty =
        elementBlockIndices.empty() && nodeSetIndices.empty() && sideSetIndices.empty();
    }
  }

  // first things first, determine all information necessary about nodes.
  // there's just 1 node block for exodus, build that.
  auto nodeBlock = std::make_shared<vtkNodeBlock>(dataset, "nodeblock_1", controller, writer);
  entityGroups.emplace(nodeBlock->GetEntityType(), nodeBlock);

  // process element blocks.
  // now, if input is not coming for IOSS reader, then all blocks are simply
  // treated as element blocks, there's no way for us to deduce otherwise.
  // let's start by simply doing that.
  int elementBlockCounter = 0;
  for (unsigned int pidx = 0; pidx < dataset->GetNumberOfPartitionedDataSets(); ++pidx)
  {
    const std::string& blockName = blockNames[pidx];
    const int& blockId = blockIds[pidx];
    const auto pds = dataset->GetPartitionedDataSet(pidx);

    // now create each type of GroupingEntity.

    // if indices are empty, all blocks will be processed as element blocks.
    if (indicesEmpty || elementBlockIndices.find(pidx) != elementBlockIndices.end())
    {
      try
      {
        if (elementBlockCounter != 0)
        {
          // add the number of cell types to the block id to ensure uniqueness.
          startSplitElementBlockId += VTK_NUMBER_OF_CELL_TYPES;
        }
        auto elementBlock = std::make_shared<vtkElementBlock>(
          pds, blockName, blockId, startSplitElementBlockId, controller, writer);
        entityGroups.emplace(elementBlock->GetEntityType(), elementBlock);
        ++elementBlockCounter;
        continue;
      }
      catch (std::runtime_error&)
      {
        break;
      }
    }

    if (sideSetIndices.find(pidx) != sideSetIndices.end())
    {
      try
      {
        auto sideSet = std::make_shared<vtkSideSet>(pds, blockName, blockId, controller, writer);
        entityGroups.emplace(sideSet->GetEntityType(), sideSet);
        continue;
      }
      catch (std::runtime_error&)
      {
        break;
      }
    }

    // nodesets are the most forgiving kind, so let's do them last.
    if (nodeSetIndices.find(pidx) != nodeSetIndices.end())
    {
      try
      {
        auto nodeSet = std::make_shared<vtkNodeSet>(pds, blockName, blockId, controller, writer);
        entityGroups.emplace(nodeSet->GetEntityType(), nodeSet);
        continue;
      }
      catch (std::runtime_error&)
      {
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
    entity.second->Define(region);
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
  return this->Internals->GlobalIdsCreated;
}
VTK_ABI_NAMESPACE_END
