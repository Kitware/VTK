// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// The auto-generated vtkBatchedLabeledDataMapperSerDes.cxx only serializes
// GetLabelTextProperty() (index 0). This helper overrides that handler so
// all MaxTextProperties text properties are round-tripped. Without this fix,
// deserialization leaves TextProperties[1..N] null, causing a null-dereference
// crash in MakeShaderArrays when any label uses a propId other than 0.

#include "vtkBatchedLabeledDataMapper.h"
#include "vtkDeserializer.h"
#include "vtkSerializer.h"
#include "vtkTextProperty.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkBatchedLabeledDataMapper.
   * Overrides the auto-generated handler to serialize all label text properties
   * by index (0..MaxTextProperties-1), not only the index-0 alias.
   */
  int RegisterHandlers_vtkBatchedLabeledDataMapperSerDesHelper(
    void* ser, void* deser, void* invoker);
}

static nlohmann::json Serialize_vtkBatchedLabeledDataMapper(
  vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto* object = vtkBatchedLabeledDataMapper::SafeDownCast(objectBase);
  if (auto f = serializer->GetHandler(typeid(vtkBatchedLabeledDataMapper::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkLabeledDataMapper");

  // Serialize all non-null text properties by index.
  // The auto-generated code only saves index 0; we save all MaxTextProperties.
  auto& propsArray = state["LabelTextProperties"] = json::array();
  for (int i = 0; i < vtkBatchedLabeledDataMapper::MaxTextProperties; ++i)
  {
    vtkTextProperty* prop = object->GetLabelTextProperty(i);
    if (prop)
    {
      json entry;
      entry["Index"] = i;
      entry["Property"] = serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(prop));
      propsArray.push_back(std::move(entry));
    }
  }

  if (auto* ptr = object->GetFrameColorsName())
  {
    state["FrameColorsName"] = ptr;
  }
  state["TextAnchor"] = object->GetTextAnchor();
  if (auto* ptr = object->GetDisplayOffset())
  {
    auto& dst = state["DisplayOffset"] = json::array();
    for (int i = 0; i < 2; ++i)
    {
      dst.push_back(ptr[i]);
    }
  }

  return state;
}

static bool Deserialize_vtkBatchedLabeledDataMapper(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  bool success = true;
  auto* object = vtkBatchedLabeledDataMapper::SafeDownCast(objectBase);
  if (!object)
  {
    vtkErrorWithObjectMacro(
      deserializer, << __func__ << ": object not a vtkBatchedLabeledDataMapper");
    return false;
  }
  if (auto f = deserializer->GetHandler(typeid(vtkBatchedLabeledDataMapper::Superclass)))
  {
    try
    {
      success &= f(state, object, deserializer);
    }
    catch (std::exception& e)
    {
      vtkErrorWithObjectMacro(deserializer, << "In " << __func__ << ", failed to deserialize state="
                                            << state.dump() << ". message=" << e.what());
      return false;
    }
  }
  if (!success)
  {
    vtkErrorWithObjectMacro(deserializer, << "Superclass deserialization failed");
    return false;
  }

  // Restore all indexed text properties.
  {
    const auto iter = state.find("LabelTextProperties");
    if (iter != state.end() && iter->is_array())
    {
      const auto* context = deserializer->GetContext();
      for (const auto& entry : *iter)
      {
        const int idx = entry.at("Index").get<int>();
        const auto& propState = entry.at("Property");
        if (propState.is_null())
        {
          object->SetLabelTextProperty(static_cast<vtkTextProperty*>(nullptr), idx);
          continue;
        }
        const auto identifier = propState.at("Id").get<vtkTypeUInt32>();
        auto subObject = context->GetObjectAtId(identifier);
        success &= deserializer->DeserializeJSON(identifier, subObject);
        if (subObject)
        {
          object->SetLabelTextProperty(reinterpret_cast<vtkTextProperty*>(subObject.Get()), idx);
        }
      }
    }
  }

  {
    const auto iter = state.find("FrameColorsName");
    if (iter != state.end() && !iter->is_null())
    {
      object->SetFrameColorsName(iter->get<std::string>().c_str());
    }
  }
  {
    const auto iter = state.find("TextAnchor");
    if (iter != state.end() && !iter->is_null())
    {
      object->SetTextAnchor(iter->get<int>());
    }
  }
  {
    const auto iter = state.find("DisplayOffset");
    if (iter != state.end() && !iter->is_null())
    {
      const auto values = iter->get<std::vector<int>>();
      object->SetDisplayOffset(values[0], values[1]);
    }
  }

  return success;
}

int RegisterHandlers_vtkBatchedLabeledDataMapperSerDesHelper(
  void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(
        typeid(vtkBatchedLabeledDataMapper), Serialize_vtkBatchedLabeledDataMapper);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(
        typeid(vtkBatchedLabeledDataMapper), Deserialize_vtkBatchedLabeledDataMapper);
      deserializer->RegisterConstructor(
        "vtkBatchedLabeledDataMapper", []() { return vtkBatchedLabeledDataMapper::New(); });
      success = 1;
    }
  }
  return success;
}
