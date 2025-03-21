// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDeserializer.h"
#include "vtkSerializer.h"
#include "vtkType.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkVariantArray
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkVariantArraySerDesHelper(void* ser, void* deser, void* invoker);
}

static nlohmann::json Serialize_vtkVariant(vtkVariant* variant, vtkSerializer* serializer)
{
  if (!variant->IsValid())
  {
    return {};
  }
  nlohmann::json state;
  int type = variant->GetType();
  state["Type"] = type;

  if (variant->IsString())
  {
    state["Value"] = variant->ToString();
  }
  else if (variant->IsFloat())
  {
    state["Value"] = variant->ToFloat();
  }
  else if (variant->IsDouble())
  {
    state["Value"] = variant->ToDouble();
  }
  else if (variant->IsNumeric())
  {
    state["Value"] = variant->ToTypeUInt64();
  }
  else if (variant->IsVTKObject())
  {
    state["Value"] = serializer->SerializeJSON(variant->ToVTKObject());
  }
  return state;
}

static void Deserialize_vtkVariant(
  const nlohmann::json& state, vtkVariant* variant, vtkDeserializer* deserializer)
{
  int type = state["Type"];
  auto iter = state.find("Value");
  if (type == VTK_STRING)
  {
    *variant = vtkVariant(iter->get<std::string>());
  }
  else if (type == VTK_FLOAT)
  {
    *variant = vtkVariant(iter->get<float>());
  }
  else if (type == VTK_DOUBLE)
  {
    *variant = vtkVariant(iter->get<double>());
  }
  else if (type == VTK_OBJECT)
  {
    vtkSmartPointer<vtkObjectBase> obj;
    deserializer->DeserializeJSON(state["Value"]["Id"], obj);
    *variant = vtkVariant(obj);
  }
  else
  {
    *variant = vtkVariant(vtkVariant(iter->get<vtkTypeUInt64>()), type);
  }
}

static nlohmann::json Serialize_vtkVariantArray(
  vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto object = vtkVariantArray::SafeDownCast(objectBase);
  if (auto f = serializer->GetHandler(typeid(vtkVariantArray::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkAbstractArray");

  auto& dst = state["Values"] = json::array();
  for (vtkIdType i = 0; i < object->GetNumberOfValues(); ++i)
  {
    dst.push_back(Serialize_vtkVariant(object->GetPointer(i), serializer));
  }
  return state;
}

static void Deserialize_vtkVariantArray(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  using json = nlohmann::json;
  auto object = vtkVariantArray::SafeDownCast(objectBase);
  if (auto f = deserializer->GetHandler(typeid(vtkVariantArray::Superclass)))
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
        vtkVariant variant;
        Deserialize_vtkVariant(value, &variant, deserializer);
        object->InsertValue(id++, variant);
      }
    }
  }
}

int RegisterHandlers_vtkVariantArraySerDesHelper(void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkVariantArray), Serialize_vtkVariantArray);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkVariantArray), Deserialize_vtkVariantArray);
      deserializer->RegisterConstructor("vtkVariantArray", []() { return vtkVariantArray::New(); });
      success = 1;
    }
  }
  return success;
}
