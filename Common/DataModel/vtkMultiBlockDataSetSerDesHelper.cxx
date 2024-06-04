// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDeserializer.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkMultiBlockDataSet
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkMultiBlockDataSetSerDesHelper(void* ser, void* deser);
}

static nlohmann::json Serialize_vtkMultiBlockDataSet(
  vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto object = vtkMultiBlockDataSet::SafeDownCast(objectBase);
  if (auto f = serializer->GetHandler(typeid(vtkMultiBlockDataSet::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkDataObjectTree");
  auto& dst = state["Blocks"] = json::array();
  for (unsigned int i = 0; i < object->GetNumberOfBlocks(); ++i)
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
      { "DataObject", serializer->SerializeJSON(object->GetBlock(i)) } });
  }
  return state;
}

static void Deserialize_vtkMultiBlockDataSet(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  using json = nlohmann::json;
  auto object = vtkMultiBlockDataSet::SafeDownCast(objectBase);
  if (auto f = deserializer->GetHandler(typeid(vtkMultiBlockDataSet::Superclass)))
  {
    f(state, object, deserializer);
  }

  {
    const auto iter = state.find("Blocks");
    if (iter != state.end() && iter->is_array())
    {
      const auto& blocks = iter->get<json::array_t>();
      if (blocks.size() < object->GetNumberOfBlocks())
      {
        // shrink if the required number of blocks is smaller than current allocation.
        object->SetNumberOfBlocks(static_cast<unsigned int>(blocks.size()));
      }
      unsigned int i = 0;
      for (const auto& blockState : blocks)
      {
        const std::string name = blockState.at("Name").get<std::string>();
        const auto& block = blockState.at("DataObject");
        auto* context = deserializer->GetContext();
        const auto identifier = block.at("Id").get<vtkTypeUInt32>();
        auto subObject = context->GetObjectAtId(identifier);
        deserializer->DeserializeJSON(identifier, subObject);
        object->SetBlock(i, vtkDataObject::SafeDownCast(subObject));
        object->GetMetaData(i)->Set(vtkCompositeDataSet::NAME(), name);
        i++;
      }
    }
  }
}

int RegisterHandlers_vtkMultiBlockDataSetSerDesHelper(void* ser, void* deser)
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkMultiBlockDataSet), Serialize_vtkMultiBlockDataSet);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkMultiBlockDataSet), Deserialize_vtkMultiBlockDataSet);
      deserializer->RegisterConstructor(
        "vtkMultiBlockDataSet", []() { return vtkMultiBlockDataSet::New(); });
      success = 1;
    }
  }
  return success;
}
