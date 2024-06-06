// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkDeserializer.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkCompositeDataDisplayAttributes
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkCompositeDataDisplayAttributesSerDesHelper(void* ser, void* deser);
}

static nlohmann::json Serialize_vtkCompositeDataDisplayAttributes(
  vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto object = vtkCompositeDataDisplayAttributes::SafeDownCast(objectBase);
  if (auto f = serializer->GetHandler(typeid(vtkCompositeDataDisplayAttributes::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkObject");

  const auto objectState = object->Serialize(serializer);
  state.insert(objectState.begin(), objectState.end());
  return state;
}

static void Deserialize_vtkCompositeDataDisplayAttributes(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  auto object = vtkCompositeDataDisplayAttributes::SafeDownCast(objectBase);
  object->Deserialize(state, deserializer);
}

int RegisterHandlers_vtkCompositeDataDisplayAttributesSerDesHelper(void* ser, void* deser)
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(
        typeid(vtkCompositeDataDisplayAttributes), Serialize_vtkCompositeDataDisplayAttributes);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(
        typeid(vtkCompositeDataDisplayAttributes), Deserialize_vtkCompositeDataDisplayAttributes);
      deserializer->RegisterConstructor("vtkCompositeDataDisplayAttributes",
        []() { return vtkCompositeDataDisplayAttributes::New(); });
      success = 1;
    }
  }
  return success;
}
