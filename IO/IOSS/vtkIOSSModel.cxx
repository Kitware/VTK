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
#include <set>
#include <numeric>

namespace
{
std::set<unsigned int> GetDatasetIndices(vtkDataAssembly* assembly, const char* name)
{
  if (assembly && assembly->GetRootNodeName() && strcmp(assembly->GetRootNodeName(), "IOSS") == 0)
  {
    const auto idx = assembly->FindFirstNodeWithName(name);
    if (idx != -1)
    {
      const auto vector = assembly->GetDataSetIndices(assembly->FindFirstNodeWithName(name));
      return std::set<unsigned int>{ vector.begin(), vector.end() };
    }
  }
  return {};
}

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
  std::map<unsigned char, std::atomic<int64_t>> elementCounts;
  for (auto& type : cellTypes)
  {
    elementCounts[type] = 0;
  }

  for (auto& ug : datasets)
  {
    vtkSMPTools::For(0, ug->GetNumberOfCells(), [&](vtkIdType start, vtkIdType end) {
      for (vtkIdType cc = start; cc < end; ++cc)
      {
        ++elementCounts[static_cast<unsigned char>(ug->GetCellType(cc))];
      }
    });
  }

  return { elementCounts.begin(), elementCounts.end() };
}

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

std::vector<std::tuple<std::string, Ioss::Field::BasicType, int>> GetFields(
  int association, vtkCompositeDataSet* pdc, vtkMultiProcessController* vtkNotUsed(controller))
{
  std::vector<std::tuple<std::string, Ioss::Field::BasicType, int>> fields;
  vtkDataSetAttributesFieldList fieldList;
  for (auto& ds : vtkCompositeDataSet::GetDataSets<vtkUnstructuredGrid>(pdc))
  {
    fieldList.IntersectFieldList(ds->GetAttributes(association));
  }

  vtkNew<vtkPointData> tmp;
  tmp->CopyAllocate(fieldList, 0);
  for (int idx = 0, max = tmp->GetNumberOfArrays(); idx < max; ++idx)
  {
    if (auto* array = tmp->GetArray(idx))
    {
      if (array == tmp->GetGlobalIds())
      {
        // we don't want to add global ids again.
        continue;
      }
      if (strcmp(array->GetName(), "object_id") == 0)
      {
        // skip "object_id". that's an array added by Ioss reader.
        continue;
      }
      const auto type = ::GetFieldType(array);
      fields.emplace_back(array->GetName(), type, array->GetNumberOfComponents());
    }
  }

  // TODO: reduce in parallel; for non IOSS input, we'll need to explicitly
  // ensure all blocks have same fields across all ranks.
  return fields;
}

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

struct vtkGroupingEntity
{
  vtkIOSSWriter* Writer = nullptr;
  vtkGroupingEntity(vtkIOSSWriter* writer)
    : Writer(writer)
  {
  }
  virtual ~vtkGroupingEntity() = default;
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
      Dispatcher::Execute(ds->GetAttributes(association)->GetArray(name.c_str()), worker);
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

  void AppendMD5(vtksysMD5* md5) const override
  {
    vtksysMD5_Append(md5, reinterpret_cast<const unsigned char*>(this->Ids.data()),
      static_cast<int>(sizeof(int32_t) * this->Ids.size()));
  }

  void Define(Ioss::Region& region) const override
  {
    auto* block = new Ioss::NodeBlock(region.get_database(), this->Name, this->Ids.size(), 3);
    // block->property_add(Ioss::Property("id", 1)); // block id.
    region.add(block);
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
      Dispatcher::Execute(ds->GetPoints()->GetData(), worker);
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
        Dispatcher::Execute(ds->GetPointData()->GetArray(displName.c_str()), dworker);
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
  vtkPartitionedDataSet* PartitionedDataSet;
  std::string RootName;
  std::map<unsigned char, int64_t> ElementCounts;
  std::vector<std::tuple<std::string, Ioss::Field::BasicType, int>> Fields;

  vtkElementBlock(vtkPartitionedDataSet* pd, const std::string& name,
    vtkMultiProcessController* controller, vtkIOSSWriter* writer)
    : vtkGroupingEntity(writer)
    , PartitionedDataSet(pd)
    , RootName{ name }
  {
    auto datasets = vtkCompositeDataSet::GetDataSets<vtkUnstructuredGrid>(pd);
    for (auto& ug : datasets)
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

  void Define(Ioss::Region& region) const override
  {
    for (auto& pair : this->ElementCounts)
    {
      const int64_t elementCount = pair.second;
      const unsigned char vtk_cell_type = pair.first;

      const auto* element = vtkIOSSUtilities::GetElementTopology(vtk_cell_type);
      const auto& elementType = element->name();

      const std::string bname =
        (this->ElementCounts.size() == 1) ? this->RootName : this->RootName + "_" + elementType;
      auto* block = new Ioss::ElementBlock(region.get_database(), bname, elementType, elementCount);
      // FIXME: for now, we let IOSS figure out the id; we need to add logic to
      //        preserve input id, if we can.
      // block->property_add(Ioss::Property("id", id++));
      region.add(block);
    }
  }

  void DefineTransient(Ioss::Region& region) const override
  {
    for (auto& pair : this->ElementCounts)
    {
      unsigned char vtk_cell_type = pair.first;
      const int64_t elementCount = pair.second;

      const auto* element = vtkIOSSUtilities::GetElementTopology(vtk_cell_type);
      const auto& elementType = element->name();
      const std::string bname =
        (this->ElementCounts.size() == 1) ? this->RootName : this->RootName + "_" + elementType;

      auto* block = region.get_element_block(bname);
      this->DefineFields(block, this->Fields, Ioss::Field::TRANSIENT, elementCount);
    }
  }

  void Model(Ioss::Region& region) const override
  {
    auto datasets = vtkCompositeDataSet::GetDataSets<vtkUnstructuredGrid>(this->PartitionedDataSet);
    for (auto& pair : this->ElementCounts)
    {
      unsigned char vtk_cell_type = pair.first;
      const int64_t elementCount = pair.second;

      const auto* element = vtkIOSSUtilities::GetElementTopology(vtk_cell_type);
      const auto& elementType = element->name();
      const int nodeCount = element->number_nodes();
      const std::string bname =
        (this->ElementCounts.size() == 1) ? this->RootName : this->RootName + "_" + elementType;

      auto* block = region.get_element_block(bname);

      // populate ids.
      std::vector<int32_t> elementIds; // these are global ids.
      elementIds.reserve(elementCount);

      std::vector<int32_t> connectivity;
      connectivity.reserve(elementCount * nodeCount);

      const int32_t gidOffset = this->Writer->GetOffsetGlobalIds() ? 1 : 0;
      for (auto& ug : datasets)
      {
        auto* gids = vtkIdTypeArray::SafeDownCast(ug->GetCellData()->GetGlobalIds());
        auto* pointGIDs = vtkIdTypeArray::SafeDownCast(ug->GetPointData()->GetGlobalIds());

        for (vtkIdType cc = 0, max = ug->GetNumberOfCells(); cc < max; ++cc)
        {
          if (ug->GetCellType(cc) == vtk_cell_type)
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
      block->put_field_data("ids", elementIds);
      block->put_field_data("connectivity", connectivity);
    }
  }

  void Transient(Ioss::Region& region) const override
  {
    auto datasets = vtkCompositeDataSet::GetDataSets<vtkUnstructuredGrid>(this->PartitionedDataSet);
    for (auto& pair : this->ElementCounts)
    {
      unsigned char vtk_cell_type = pair.first;

      const auto* element = vtkIOSSUtilities::GetElementTopology(vtk_cell_type);
      const auto& elementType = element->name();
      const std::string bname =
        (this->ElementCounts.size() == 1) ? this->RootName : this->RootName + "_" + elementType;

      auto* block = region.get_element_block(bname);

      // populate ids.
      std::vector<std::vector<vtkIdType>> lIds; // these are local ids.
      for (auto& ug : datasets)
      {
        lIds.emplace_back();
        lIds.back().reserve(ug->GetNumberOfCells());
        for (vtkIdType cc = 0, max = ug->GetNumberOfCells(); cc < max; ++cc)
        {
          if (ug->GetCellType(cc) == vtk_cell_type)
          {
            lIds.back().push_back(cc);
          }
        }
      }

      // add fields.
      this->PutFields(block, this->Fields, lIds, datasets, vtkDataObject::CELL);
    }
  }
};

struct vtkNodeSet : public vtkGroupingEntity
{
  vtkPartitionedDataSet* PartitionedDataSet;
  std::string Name;
  int64_t Count{ 0 };
  vtkNodeSet(vtkPartitionedDataSet* pd, const std::string& name,
    vtkMultiProcessController* vtkNotUsed(controller), vtkIOSSWriter* writer)
    : vtkGroupingEntity(writer)
    , PartitionedDataSet(pd)
    , Name(name)
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

  void AppendMD5(vtksysMD5* md5) const override
  {
    vtksysMD5_Append(md5, reinterpret_cast<const unsigned char*>(this->Name.c_str()), -1);
    vtksysMD5_Append(md5, reinterpret_cast<const unsigned char*>(&this->Count),
      static_cast<int>(sizeof(this->Count)));
  }

  void Define(Ioss::Region& region) const override
  {
    auto* nodeset = new Ioss::NodeSet(region.get_database(), this->Name, this->Count);
    // nodeset->property_add(Ioss::Property("id", id++));
    region.add(nodeset);
  }

  void DefineTransient(Ioss::Region& vtkNotUsed(region)) const override {}

  void Model(Ioss::Region& region) const override
  {
    auto* nodeset = region.get_nodeset(this->Name);
    std::vector<int32_t> ids;
    ids.reserve(this->Count);
    for (auto& ug : vtkCompositeDataSet::GetDataSets<vtkUnstructuredGrid>(this->PartitionedDataSet))
    {
      auto* gids = vtkIdTypeArray::SafeDownCast(ug->GetPointData()->GetGlobalIds());
      std::copy(
        gids->GetPointer(0), gids->GetPointer(gids->GetNumberOfTuples()), std::back_inserter(ids));
    }
    nodeset->put_field_data("ids", ids);
  }

  void Transient(Ioss::Region& vtkNotUsed(region)) const override {}
};

struct vtkSideSet : public vtkGroupingEntity
{
  vtkPartitionedDataSet* PartitionedDataSet;
  std::string Name;
  int64_t Count;

  vtkSideSet(vtkPartitionedDataSet* pd, const std::string& name,
    vtkMultiProcessController* vtkNotUsed(controller), vtkIOSSWriter* writer)
    : vtkGroupingEntity(writer)
    , PartitionedDataSet(pd)
    , Name(name)
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
    auto* sideblock = new Ioss::SideBlock(
      region.get_database(), "sideblock_0", elementType, elementType, this->Count);

    auto* sideset = new Ioss::SideSet(region.get_database(), this->Name);
    sideset->add(sideblock);
    region.add(sideset);
  }

  void DefineTransient(Ioss::Region& vtkNotUsed(region)) const override {}

  void Model(Ioss::Region& region) const override
  {
    auto* sideset = region.get_sideset(this->Name);
    auto* sideblock = sideset->get_side_block("sideblock_0");

    std::vector<int32_t> elementSide;
    elementSide.reserve(this->Count * 2);

    auto datasets = vtkCompositeDataSet::GetDataSets<vtkUnstructuredGrid>(this->PartitionedDataSet);
    for (auto& ug : datasets)
    {
      for (auto&& tuple : vtk::DataArrayTupleRange(
             vtkIntArray::SafeDownCast(ug->GetCellData()->GetArray("element_side"))))
      {
        for (const auto& comp : tuple)
        {
          elementSide.push_back(comp);
        }
      }
    }

    assert(elementSide.size() == static_cast<size_t>(this->Count * 2));
    sideblock->put_field_data("element_side", elementSide);
  }

  void Transient(Ioss::Region& vtkNotUsed(region)) const override {}
};

//=============================================================================
class vtkIOSSModel::vtkInternals
{
public:
  vtkSmartPointer<vtkMultiProcessController> Controller;
  std::multimap<Ioss::EntityType, std::shared_ptr<vtkGroupingEntity>> EntityGroups;
};

//----------------------------------------------------------------------------
vtkIOSSModel::vtkIOSSModel(vtkPartitionedDataSetCollection* dataset, vtkIOSSWriter* writer)
  : Internals(new vtkIOSSModel::vtkInternals())
{
  auto* controller = writer->GetController();
  auto& internals = (*this->Internals);
  internals.Controller = controller
    ? vtk::MakeSmartPointer(controller)
    : vtk::TakeSmartPointer(vtkMultiProcessController::SafeDownCast(vtkDummyController::New()));

  auto* assembly = dataset->GetDataAssembly();
  const bool isIOSS = writer->GetPreserveInputEntityGroups() &&
    (assembly && assembly->GetRootNodeName() && strcmp(assembly->GetRootNodeName(), "IOSS") == 0);
  const auto elementBlockIndices = ::GetDatasetIndices(assembly, "element_blocks");
  const auto nodeSetIndices = ::GetDatasetIndices(assembly, "node_sets");
  const auto sideSetIndices = ::GetDatasetIndices(assembly, "side_sets");

  // first things first, determine all information necessary about nodes.
  // there's just 1 node block for exodus, build that.
  internals.EntityGroups.emplace(Ioss::EntityType::NODEBLOCK,
    std::make_shared<vtkNodeBlock>(dataset, "nodeblock_1", internals.Controller, writer));

  // process element blocks.
  // now, if input is not coming for IOSS reader, then all blocks are simply
  // treated as element blocks, there's no way for us to deduce otherwise.
  // let's start by simply doing that.
  for (unsigned int pidx = 0; pidx < dataset->GetNumberOfPartitionedDataSets(); ++pidx)
  {
    std::string bname = "block_" + std::to_string(pidx + 1);
    if (auto info = dataset->GetMetaData(pidx))
    {
      if (info->Has(vtkCompositeDataSet::NAME()))
      {
        bname = info->Get(vtkCompositeDataSet::NAME());
      }
    }

    // now create each type of GroupingEntity.
    if (!isIOSS || elementBlockIndices.find(pidx) != elementBlockIndices.end())
    {
      try
      {
        internals.EntityGroups.emplace(Ioss::EntityType::ELEMENTBLOCK,
          std::make_shared<vtkElementBlock>(
            dataset->GetPartitionedDataSet(pidx), bname, internals.Controller, writer));
        continue;
      }
      catch (std::runtime_error&)
      {
        // if isIOSS, raise error...perhaps?
        break;
      }
    }

    if (sideSetIndices.find(pidx) != sideSetIndices.end())
    {
      try
      {
        internals.EntityGroups.emplace(Ioss::EntityType::SIDESET,
          std::make_shared<vtkSideSet>(
            dataset->GetPartitionedDataSet(pidx), bname, internals.Controller, writer));
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
        internals.EntityGroups.emplace(Ioss::EntityType::NODESET,
          std::make_shared<vtkNodeSet>(
            dataset->GetPartitionedDataSet(pidx), bname, internals.Controller, writer));
        continue;
      }
      catch (std::runtime_error&)
      {
        break;
      }
    }

    vtkLogF(WARNING, "Skipping block '%s'. Unsure how classify it.", bname.c_str());
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
