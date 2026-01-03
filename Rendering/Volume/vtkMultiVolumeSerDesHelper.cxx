// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAbstractVolumeMapper.h"
#include "vtkDeserializer.h"
#include "vtkMultiVolume.h"
#include "vtkObjectBase.h"
#include "vtkSerializer.h"
#include "vtkStringFormatter.h"
#include "vtkStringScanner.h"
#include "vtkVariant.h"
#include "vtkVolume.h"
//clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
//clang-format on
extern "C"
{
  /**
   * Register the (de)serialization handlers of classes from all serialized libraries.
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   * @param invoker a vtkInvoker instance
   * @param error when registration fails, the error message is pointed to by `error`. Use it for
   * logging purpose.
   * @warning The memory pointed to by `error` is not dynamically allocated. Do not free it.
   */
  int RegisterHandlers_vtkMultiVolumeSerDesHelper(void* ser, void* deser, void* invoker);
}
static nlohmann::json Serialize_vtkMultiVolume(vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto* object = vtkMultiVolume::SafeDownCast(objectBase);
  if (auto f = serializer->GetHandler(typeid(vtkMultiVolume::Superclass::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkVolume");

  {
    auto* value = object->GetMapper();
    if (value)
    {
      state["Mapper"] = serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(value));
    }
  }

  const auto& map = object->GetAllVolumes();
  auto& dst = state["AllVolumes"] = json::object();
  for (const auto& pair : map)
  {
    dst[vtk::to_string(pair.first)] =
      serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(pair.second));
  }
  (void)serializer;
  return state;
}

static bool Deserialize_vtkMultiVolume(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  bool success = true;
  auto* object = vtkMultiVolume::SafeDownCast(objectBase);
  if (!object)
  {
    vtkErrorWithObjectMacro(deserializer, << __func__ << ": object not a vtkMultiVolume");
    return false;
  }
  // Skip superclass vtkVolume to avoid warning from vtkMultiVolume::SetProperty().
  if (auto f = deserializer->GetHandler(typeid(vtkMultiVolume::Superclass::Superclass)))
  {
    success &= f(state, object, deserializer);
  }
  if (!success)
  {
    return false;
  }
  {
    auto iter = state.find("Mapper");
    if ((iter != state.end()) && !iter->is_null())
    {
      const auto* context = deserializer->GetContext();
      const auto identifier = iter->at("Id").get<vtkTypeUInt32>();
      auto subObject = context->GetObjectAtId(identifier);
      success &= deserializer->DeserializeJSON(identifier, subObject);
      if (subObject != nullptr)
      {
        object->SetMapper(vtkAbstractVolumeMapper::SafeDownCast(subObject));
      }
    }
  }

  {
    const auto iter = state.find("AllVolumes");
    if ((iter != state.end()) && !iter->is_null())
    {
      const auto* context = deserializer->GetContext();
      auto values = iter->get<std::map<std::string, nlohmann::json>>();
      std::unordered_map<int, vtkVolume*> map;
      for (const auto& item : values)
      {
        const auto identifier = item.second.at("Id").get<vtkTypeUInt32>();
        auto subObject = context->GetObjectAtId(identifier);
        success &= deserializer->DeserializeJSON(identifier, subObject);
        if (subObject != nullptr)
        {
          subObject->Register(object);
          int index = 0;
          VTK_FROM_CHARS_IF_ERROR_RETURN(item.first, index, false);
          map[index] = vtkVolume::SafeDownCast(subObject);
        }
      }
      object->SetAllVolumes(map);
      for (const auto& item : map)
      {
        item.second->UnRegister(object);
      }
    }
  }
  return success;
}

int RegisterHandlers_vtkMultiVolumeSerDesHelper(void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkMultiVolume), Serialize_vtkMultiVolume);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkMultiVolume), Deserialize_vtkMultiVolume);
      deserializer->RegisterConstructor("vtkMultiVolume", []() { return vtkMultiVolume::New(); });
      success = 1;
    }
  }
  return success;
}
