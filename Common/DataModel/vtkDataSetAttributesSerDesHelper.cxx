// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkDeserializer.h"
#include "vtkObjectBase.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  int RegisterHandlers_vtkDataSetAttributesSerDesHelper(void* ser, void* deser);
}

#define SERIALIZE_DATA_ATTRIBUTE(AttributeType)                                                    \
  if (auto* array = dsa->Get##AttributeType())                                                     \
  {                                                                                                \
    state[#AttributeType] = serializer->SerializeJSON(array);                                      \
  }

#define DESERIALIZE_DATA_ATTRIBUTE(AttributeType)                                                  \
  if (state.contains(#AttributeType))                                                              \
  {                                                                                                \
    const auto identifier = state[#AttributeType]["Id"].get<vtkTypeUInt32>();                      \
    if (auto* array = vtkDataArray::SafeDownCast(context->GetObjectAtId(identifier)))              \
    {                                                                                              \
      dsa->Set##AttributeType(array);                                                              \
    }                                                                                              \
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
    state["NumberOfArrays"] = dsa->GetNumberOfArrays();
    SERIALIZE_DATA_ATTRIBUTE(Scalars)
    SERIALIZE_DATA_ATTRIBUTE(Vectors)
    SERIALIZE_DATA_ATTRIBUTE(Normals)
    SERIALIZE_DATA_ATTRIBUTE(Tangents)
    SERIALIZE_DATA_ATTRIBUTE(TCoords)
    SERIALIZE_DATA_ATTRIBUTE(GlobalIds)
    SERIALIZE_DATA_ATTRIBUTE(PedigreeIds)
    SERIALIZE_DATA_ATTRIBUTE(RationalWeights)
    SERIALIZE_DATA_ATTRIBUTE(HigherOrderDegrees)
    SERIALIZE_DATA_ATTRIBUTE(ProcessIds)
    auto& dst = state["Arrays"] = json::array();
    for (int i = 0; i < dsa->GetNumberOfArrays(); ++i)
    {
      dst.push_back(serializer->SerializeJSON(dsa->GetArray(i)));
    }
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
    auto* context = deserializer->GetContext();
    for (int i = 0; i < dsa->GetNumberOfArrays(); ++i)
    {
      auto* array = dsa->GetArray(i);
      context->UnRegisterObject(context->GetId(array));
      dsa->RemoveArray(i);
    }
    const auto& stateOfArrays = state["Arrays"];
    for (auto& stateOfarray : stateOfArrays)
    {
      const auto identifier = stateOfarray["Id"].get<vtkTypeUInt32>();
      auto subObject = context->GetObjectAtId(identifier);
      deserializer->DeserializeJSON(identifier, subObject);
      if (auto* array = vtkDataArray::SafeDownCast(subObject))
      {
        dsa->AddArray(array);
      }
    }
    DESERIALIZE_DATA_ATTRIBUTE(Scalars)
    DESERIALIZE_DATA_ATTRIBUTE(Vectors)
    DESERIALIZE_DATA_ATTRIBUTE(Normals)
    DESERIALIZE_DATA_ATTRIBUTE(Tangents)
    DESERIALIZE_DATA_ATTRIBUTE(TCoords)
    DESERIALIZE_DATA_ATTRIBUTE(GlobalIds)
    DESERIALIZE_DATA_ATTRIBUTE(PedigreeIds)
    DESERIALIZE_DATA_ATTRIBUTE(RationalWeights)
    DESERIALIZE_DATA_ATTRIBUTE(HigherOrderDegrees)
    DESERIALIZE_DATA_ATTRIBUTE(ProcessIds)
  }
}

int RegisterHandlers_vtkDataSetAttributesSerDesHelper(void* ser, void* deser)
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
