// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDeserializer.h"
#include "vtkInformation.h"
#include "vtkPartitionedDataSet.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkPartitionedDataSet
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkPartitionedDataSetSerDesHelper(void* ser, void* deser, void* invoker);
}

static nlohmann::json Serialize_vtkPartitionedDataSet(
  vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto object = vtkPartitionedDataSet::SafeDownCast(objectBase);
  if (auto f = serializer->GetHandler(typeid(vtkPartitionedDataSet::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkDataObjectTree");
  auto& dst = state["Partitions"] = json::array();
  for (unsigned int i = 0; i < object->GetNumberOfPartitions(); ++i)
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
      { "DataObject", serializer->SerializeJSON(object->GetPartitionAsDataObject(i)) } });
  }
  return state;
}

static void Deserialize_vtkPartitionedDataSet(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  using json = nlohmann::json;
  auto object = vtkPartitionedDataSet::SafeDownCast(objectBase);
  if (auto f = deserializer->GetHandler(typeid(vtkPartitionedDataSet::Superclass)))
  {
    f(state, object, deserializer);
  }

  {
    const auto iter = state.find("Partitions");
    if (iter != state.end() && iter->is_array())
    {
      const auto& partitions = iter->get<json::array_t>();
      if (partitions.size() < object->GetNumberOfPartitions())
      {
        // shrink if the required number of partitions is smaller than current allocation.
        object->SetNumberOfPartitions(static_cast<unsigned int>(partitions.size()));
      }
      unsigned int i = 0;
      for (const auto& partitionState : partitions)
      {
        const std::string name = partitionState.at("Name").get<std::string>();
        const auto& partition = partitionState.at("DataObject");
        if (!partition.empty())
        {
          auto* context = deserializer->GetContext();
          const auto identifier = partition.at("Id").get<vtkTypeUInt32>();
          auto subObject = context->GetObjectAtId(identifier);
          deserializer->DeserializeJSON(identifier, subObject);
          object->SetPartition(i, vtkDataObject::SafeDownCast(subObject));
          object->GetMetaData(i)->Set(vtkCompositeDataSet::NAME(), name);
        }
        else
        {
          object->SetPartition(i, nullptr);
        }
        i++;
      }
    }
  }
}

int RegisterHandlers_vtkPartitionedDataSetSerDesHelper(
  void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkPartitionedDataSet), Serialize_vtkPartitionedDataSet);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(
        typeid(vtkPartitionedDataSet), Deserialize_vtkPartitionedDataSet);
      deserializer->RegisterConstructor(
        "vtkPartitionedDataSet", []() { return vtkPartitionedDataSet::New(); });
      success = 1;
    }
  }
  return success;
}
