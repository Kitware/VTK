// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDeserializer.h"
#include "vtkSerializer.h"
#include "vtkStringArray.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkStringArray
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkStringArraySerDesHelper(void* ser, void* deser, void* invoker);
}

static nlohmann::json Serialize_vtkStringArray(vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto object = vtkStringArray::SafeDownCast(objectBase);
  if (auto f = serializer->GetHandler(typeid(vtkStringArray::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkAbstractArray");
  auto& dst = state["Values"] = json::array();
  for (vtkIdType i = 0; i < object->GetNumberOfValues(); ++i)
  {
    dst.emplace_back(static_cast<std::string>(object->GetValue(i)));
  }
  return state;
}

static void Deserialize_vtkStringArray(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  using json = nlohmann::json;
  auto object = vtkStringArray::SafeDownCast(objectBase);
  if (auto f = deserializer->GetHandler(typeid(vtkStringArray::Superclass)))
  {
    f(state, object, deserializer);
  }

  {
    const auto iter = state.find("Values");
    if (iter != state.end() && iter->is_array())
    {
      const auto& values = iter->get<json::array_t>();
      vtkIdType id = 0;
      for (const auto& value : values)
      {
        object->SetValue(id++, value);
      }
    }
  }
}

int RegisterHandlers_vtkStringArraySerDesHelper(void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkStringArray), Serialize_vtkStringArray);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkStringArray), Deserialize_vtkStringArray);
      deserializer->RegisterConstructor("vtkStringArray", []() { return vtkStringArray::New(); });
      success = 1;
    }
  }
  return success;
}
