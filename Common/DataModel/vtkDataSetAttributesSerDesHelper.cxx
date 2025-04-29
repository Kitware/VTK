// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataSetAttributes.h"
#include "vtkDataSetAttributesFieldList.h"
#include "vtkDeserializer.h"
#include "vtkObjectBase.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkDataSetAttributes
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkDataSetAttributesSerDesHelper(void* ser, void* deser, void* invoker);
}

static nlohmann::json Serialize_vtkDataSetAttributes(
  vtkObjectBase* object, vtkSerializer* serializer)
{
  using nlohmann::json;
  if (auto* dsa = vtkDataSetAttributes::SafeDownCast(object))
  {
    json state;
    if (auto superSerializer = serializer->GetHandler(typeid(vtkDataSetAttributes::Superclass)))
    {
      state = superSerializer(object, serializer);
    }
    std::vector<int> attrIndices(vtkDataSetAttributes::NUM_ATTRIBUTES, -1);
    dsa->GetAttributeIndices(attrIndices.data());
    state["AttributeIndices"] = attrIndices;
    return state;
  }
  else
  {
    return {};
  }
}

static void Deserialize_vtkDataSetAttributes(
  const nlohmann::json& state, vtkObjectBase* object, vtkDeserializer* deserializer)
{
  using nlohmann::json;
  if (auto* dsa = vtkDataSetAttributes::SafeDownCast(object))
  {
    if (auto superDeserializer = deserializer->GetHandler(typeid(vtkDataSetAttributes::Superclass)))
    {
      superDeserializer(state, object, deserializer);
    }
    const auto& attributeIndices = state["AttributeIndices"];
    if (attributeIndices.size() != vtkDataSetAttributes::NUM_ATTRIBUTES)
    {
      vtkWarningWithObjectMacro(deserializer,
        << "Failed to deserialize active attribute types in the dataset attributes object. "
           "The number of attribute indices in state is not "
           "equal to vtkDataSetAttributes::NUM_ATTRIBUTES("
        << vtkDataSetAttributes::NUM_ATTRIBUTES << ")!");
      return;
    }

    std::vector<int> existingAttributeIndices(vtkDataSetAttributes::NUM_ATTRIBUTES, -1);
    dsa->GetAttributeIndices(existingAttributeIndices.data());
    for (int attributeType = 0; attributeType < vtkDataSetAttributes::NUM_ATTRIBUTES;
         ++attributeType)
    {
      if (existingAttributeIndices[attributeType] != attributeIndices[attributeType])
      {
        dsa->SetActiveAttribute(attributeIndices[attributeType], attributeType);
      }
    }
  }
}

int RegisterHandlers_vtkDataSetAttributesSerDesHelper(
  void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkDataSetAttributes), Serialize_vtkDataSetAttributes);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkDataSetAttributes), Deserialize_vtkDataSetAttributes);
      deserializer->RegisterConstructor("vtkDataSetAttributes", vtkDataSetAttributes::New);
      success = 1;
    }
  }
  return success;
}
