// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDeserializer.h"
#include "vtkFieldData.h"
#include "vtkObjectBase.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include <cstdint>
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkFieldData
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkFieldDataSerDesHelper(void* ser, void* deser, void* invoker);
}

static nlohmann::json Serialize_vtkFieldData(vtkObjectBase* object, vtkSerializer* serializer)
{
  using nlohmann::json;
  if (auto* fd = vtkFieldData::SafeDownCast(object))
  {
    json state;
    if (auto superSerializer = serializer->GetHandler(typeid(vtkFieldData::Superclass)))
    {
      state = superSerializer(object, serializer);
    }
    state["NumberOfArrays"] = fd->GetNumberOfArrays();
    auto& dst = state["Arrays"] = json::array();
    for (int i = 0; i < fd->GetNumberOfArrays(); ++i)
    {
      dst.push_back(serializer->SerializeJSON(fd->GetAbstractArray(i)));
    }
    state["GhostsToSkip"] = fd->GetGhostsToSkip();
    state["NumberOfTuples"] = fd->GetNumberOfTuples();
    return state;
  }
  else
  {
    return {};
  }
}

static void Deserialize_vtkFieldData(
  const nlohmann::json& state, vtkObjectBase* object, vtkDeserializer* deserializer)
{
  using nlohmann::json;
  if (auto* fd = vtkFieldData::SafeDownCast(object))
  {
    if (auto superDeserializer = deserializer->GetHandler(typeid(vtkFieldData::Superclass)))
    {
      superDeserializer(state, object, deserializer);
    }
    auto* context = deserializer->GetContext();
    const auto& stateOfArrays = state["Arrays"];
    // vector used to keep existing arrays alive so that fd->RemoveArray doesn't destroy the
    // vtkAbstractArray object.
    std::vector<vtkSmartPointer<vtkAbstractArray>> arrays;
    for (auto& stateOfarray : stateOfArrays)
    {
      const auto identifier = stateOfarray["Id"].get<vtkTypeUInt32>();
      auto subObject = context->GetObjectAtId(identifier);
      deserializer->DeserializeJSON(identifier, subObject);
      if (auto* array = vtkAbstractArray::SafeDownCast(subObject))
      {
        arrays.emplace_back(array);
      }
    }
    if (static_cast<std::size_t>(fd->GetNumberOfArrays()) != arrays.size())
    {
      // Now remove arrays from the collection.
      // If arrays already existied before entering this function, it does not invoke
      // destructor on the vtkAbstractArray because a reference is held by the vector of arrays.
      while (fd->GetNumberOfArrays() > 0)
      {
        auto* array = fd->GetAbstractArray(0);
        context->UnRegisterObject(context->GetId(array));
        fd->RemoveArray(0);
      }
      for (const auto& array : arrays)
      {
        fd->AddArray(array);
      }
    }
    VTK_DESERIALIZE_VALUE_FROM_STATE(NumberOfTuples, int, state, fd);
    VTK_DESERIALIZE_VALUE_FROM_STATE(GhostsToSkip, int, state, fd);
  }
}

int RegisterHandlers_vtkFieldDataSerDesHelper(void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkFieldData), Serialize_vtkFieldData);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkFieldData), Deserialize_vtkFieldData);
      deserializer->RegisterConstructor("vtkFieldData", vtkFieldData::New);
      success = 1;
    }
  }
  return success;
}
