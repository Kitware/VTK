// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAbstractArray.h"
#include "vtkDeserializer.h"
#include "vtkSerializer.h"
#include "vtkTable.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkTable
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkTableSerDesHelper(void* ser, void* deser);
}

static nlohmann::json Serialize_vtkTable(vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto object = vtkTable::SafeDownCast(objectBase);
  if (auto f = serializer->GetHandler(typeid(vtkTable::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkTable");
  auto& dst = state["Columns"] = json::array();
  for (vtkIdType i = 0; i < object->GetNumberOfColumns(); ++i)
  {
    dst.emplace_back(serializer->SerializeJSON(object->GetColumn(i)));
  }
  return state;
}

static void Deserialize_vtkTable(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  using json = nlohmann::json;
  auto object = vtkTable::SafeDownCast(objectBase);
  if (auto f = deserializer->GetHandler(typeid(vtkTable::Superclass)))
  {
    f(state, object, deserializer);
  }

  object->RemoveAllColumns();
  {
    const auto iter = state.find("Columns");
    if (iter != state.end() && iter->is_array())
    {
      const auto& columns = iter->get<json::array_t>();
      for (const auto& column : columns)
      {
        auto* context = deserializer->GetContext();
        const auto identifier = column.at("Id").get<vtkTypeUInt32>();
        auto subObject = context->GetObjectAtId(identifier);
        deserializer->DeserializeJSON(identifier, subObject);
        if (auto array = vtkAbstractArray::SafeDownCast(subObject))
        {
          object->AddColumn(array);
        }
      }
    }
  }
}

int RegisterHandlers_vtkTableSerDesHelper(void* ser, void* deser)
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkTable), Serialize_vtkTable);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkTable), Deserialize_vtkTable);
      deserializer->RegisterConstructor("vtkTable", []() { return vtkTable::New(); });
      success = 1;
    }
  }
  return success;
}
