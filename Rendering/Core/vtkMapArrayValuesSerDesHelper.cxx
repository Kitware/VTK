// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDeserializer.h"
#include "vtkMapArrayValues.h"
#include "vtkSerializer.h"
#include "vtkVariant.h"
#include "vtkVariantSerDesHelper.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkMapArrayValues
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkMapArrayValuesSerDesHelper(void* ser, void* deser, void* invoker);
}

static nlohmann::json Serialize_vtkMapArrayValues(vtkObjectBase* object, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  if (auto* mapArrayValues = vtkMapArrayValues::SafeDownCast(object))
  {
    json state;
    if (auto superSerializer = serializer->GetHandler(typeid(vtkMapArrayValues::Superclass)))
    {
      state = superSerializer(object, serializer);
    }
    state["SuperClassNames"].push_back("vtkPassInputTypeAlgorithm");
    state["FieldType"] = mapArrayValues->GetFieldType();
    state["PassArray"] = mapArrayValues->GetPassArray();
    state["FillValue"] = mapArrayValues->GetFillValue();
    if (const char* ptr = mapArrayValues->GetInputArrayName())
    {
      state["InputArrayName"] = ptr;
    }
    else
    {
      // tempting to fold if-else into direct assignment, but don't!
      // nlohmann routes a pointer of type const char* through string_t constructor
      // which is UB.
      state["InputArrayName"] = nullptr;
    }
    if (const char* ptr = mapArrayValues->GetOutputArrayName())
    {
      state["OutputArrayName"] = ptr;
    }
    else
    {
      state["OutputArrayName"] = nullptr;
    }
    state["OutputArrayType"] = mapArrayValues->GetOutputArrayType();
    const auto& mapContents = mapArrayValues->GetMap();
    auto& dst = state["Map"] = json::array();
    for (const auto& entry : mapContents)
    {
      dst.emplace_back(json({ { "From", Serialize_vtkVariant(&(entry.first), serializer) },
        { "To", Serialize_vtkVariant(&(entry.second), serializer) } }));
    }
    return state;
  }
  else
  {
    vtkWarningWithObjectMacro(serializer, << "Object is not a vtkMapArrayValues!");
    return {};
  }
}

static bool Deserialize_vtkMapArrayValues(
  const nlohmann::json& state, vtkObjectBase* object, vtkDeserializer* deserializer)
{
  bool success = true;
  auto* mapArrayValues = vtkMapArrayValues::SafeDownCast(object);
  if (!mapArrayValues)
  {
    vtkErrorWithObjectMacro(deserializer, << __func__ << ": object not a vtkMapArrayValues");
    return false;
  }
  if (auto f = deserializer->GetHandler(typeid(vtkMapArrayValues::Superclass)))
  {
    success &= f(state, object, deserializer);
  }
  if (!success)
  {
    vtkErrorWithObjectMacro(deserializer, << "Superclass deserialization failed");
    return false;
  }
  VTK_DESERIALIZE_VALUE_FROM_STATE(FieldType, int, state, mapArrayValues);
  VTK_DESERIALIZE_VALUE_FROM_STATE(PassArray, vtkTypeBool, state, mapArrayValues);
  VTK_DESERIALIZE_VALUE_FROM_STATE(FillValue, double, state, mapArrayValues);
  if (const auto iter = state.find("InputArrayName"); iter != state.end() && !iter->is_null())
  {
    const auto value = iter->get<std::string>();
    mapArrayValues->SetInputArrayName(value.c_str());
  }
  else
  {
    mapArrayValues->SetInputArrayName(nullptr);
  }
  if (const auto iter = state.find("OutputArrayName"); iter != state.end() && !iter->is_null())
  {
    const auto value = iter->get<std::string>();
    mapArrayValues->SetOutputArrayName(value.c_str());
  }
  else
  {
    mapArrayValues->SetOutputArrayName(nullptr);
  }
  VTK_DESERIALIZE_VALUE_FROM_STATE(OutputArrayType, int, state, mapArrayValues);
  if (auto iter = state.find("Map"); iter != state.end())
  {
    for (const auto& entry : iter.value())
    {
      vtkVariant from, to;
      bool haveFrom = false, haveTo = false;
      if (auto fromIter = entry.find("From"); fromIter != entry.end())
      {
        haveFrom = Deserialize_vtkVariant(fromIter.value(), &from, deserializer);
      }
      if (auto toIter = entry.find("To"); toIter != entry.end())
      {
        haveTo = Deserialize_vtkVariant(toIter.value(), &to, deserializer);
      }
      if (!haveFrom)
      {
        vtkWarningWithObjectMacro(deserializer, << "Could not find a 'From' entry in the map");
        success &= false;
      }
      if (!haveTo)
      {
        vtkWarningWithObjectMacro(deserializer, << "Could not find a 'To' entry in the map");
        success &= false;
      }
      if (haveFrom && haveTo)
      {
        mapArrayValues->AddToMap(from, to);
      }
    }
  }
  return success;
}

int RegisterHandlers_vtkMapArrayValuesSerDesHelper(
  void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkMapArrayValues), Serialize_vtkMapArrayValues);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkMapArrayValues), Deserialize_vtkMapArrayValues);
      deserializer->RegisterConstructor("vtkMapArrayValues", vtkMapArrayValues::New);
      success = 1;
    }
  }
  return success;
}
