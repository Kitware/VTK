// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataAssembly.h"
#include "vtkDeserializer.h"
#include "vtkInformation.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkPartitionedDataSetCollection
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkPartitionedDataSetCollectionSerDesHelper(void* ser, void* deser);
}

static nlohmann::json Serialize_vtkPartitionedDataSetCollection(
  vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto object = vtkPartitionedDataSetCollection::SafeDownCast(objectBase);
  if (auto f = serializer->GetHandler(typeid(vtkPartitionedDataSetCollection::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkDataObjectTree");
  auto& dst = state["PartitionedDataSets"] = json::array();
  for (unsigned int i = 0; i < object->GetNumberOfPartitionedDataSets(); ++i)
  {
    const char* name = nullptr;
    if (object->HasMetaData(i))
    {
      auto* metadata = object->GetMetaData(i);
      if (metadata->Has(vtkCompositeDataSet::NAME()))
      {
        name = metadata->Get(vtkCompositeDataSet::NAME());
      }
    }
    dst.emplace_back(json{ { "Name", name ? name : "" },
      { "DataObject", serializer->SerializeJSON(object->GetPartitionedDataSet(i)) } });
  }
  if (auto* dataAssembly = object->GetDataAssembly())
  {
    state["DataAssembly"] = dataAssembly->SerializeToXML(vtkIndent());
  }
  else
  {
    state["DataAssembly"] = "";
  }
  return state;
}

static void Deserialize_vtkPartitionedDataSetCollection(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  using json = nlohmann::json;
  auto object = vtkPartitionedDataSetCollection::SafeDownCast(objectBase);
  if (auto f = deserializer->GetHandler(typeid(vtkPartitionedDataSetCollection::Superclass)))
  {
    f(state, object, deserializer);
  }

  {
    const auto iter = state.find("PartitionedDataSets");
    if (iter != state.end() && iter->is_array())
    {
      const auto& partitionedDataSets = iter->get<json::array_t>();
      if (partitionedDataSets.size() < object->GetNumberOfPartitionedDataSets())
      {
        // shrink if the required number of vtkPartitionedDataSet(s) is smaller than current
        // allocation.
        object->SetNumberOfPartitionedDataSets(
          static_cast<unsigned int>(partitionedDataSets.size()));
      }
      unsigned int i = 0;
      for (const auto& pdsState : partitionedDataSets)
      {
        const std::string name = pdsState.at("Name").get<std::string>();
        const auto& pds = pdsState.at("DataObject");
        auto* context = deserializer->GetContext();
        const auto identifier = pds.at("Id").get<vtkTypeUInt32>();
        auto subObject = context->GetObjectAtId(identifier);
        deserializer->DeserializeJSON(identifier, subObject);
        object->SetPartitionedDataSet(i, vtkPartitionedDataSet::SafeDownCast(subObject));
        object->GetMetaData(i)->Set(vtkCompositeDataSet::NAME(), name);
        i++;
      }
    }
  }
  {
    const auto iter = state.find("DataAssembly");
    if (iter != state.end() && iter->is_string())
    {
      const auto dataAssemblyState = iter->get<std::string>();
      vtkSmartPointer<vtkDataAssembly> dataAssembly = object->GetDataAssembly();
      if (dataAssembly == nullptr)
      {
        dataAssembly = vtk::TakeSmartPointer(vtkDataAssembly::New());
        object->SetDataAssembly(dataAssembly);
      }
      if (dataAssemblyState.empty())
      {
        dataAssembly->InitializeFromXML(nullptr);
      }
      else
      {
        dataAssembly->InitializeFromXML(dataAssemblyState.c_str());
      }
    }
  }
}

int RegisterHandlers_vtkPartitionedDataSetCollectionSerDesHelper(void* ser, void* deser)
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(
        typeid(vtkPartitionedDataSetCollection), Serialize_vtkPartitionedDataSetCollection);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(
        typeid(vtkPartitionedDataSetCollection), Deserialize_vtkPartitionedDataSetCollection);
      deserializer->RegisterConstructor(
        "vtkPartitionedDataSetCollection", []() { return vtkPartitionedDataSetCollection::New(); });
      success = 1;
    }
  }
  return success;
}
