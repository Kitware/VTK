// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDeserializer.h"
#include "vtkSerializer.h"
#include "vtkShaderProperty.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include <string>
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkShaderProperty
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkShaderPropertySerDesHelper(void* ser, void* deser, void* invoker);
}

static nlohmann::json Serialize_vtkShaderProperty(
  vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto object = vtkShaderProperty::SafeDownCast(objectBase);
  if (auto f = serializer->GetHandler(typeid(vtkShaderProperty::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkObject");

  if (auto ptr = object->GetVertexShaderCode())
  {
    state["VertexShaderCode"] = ptr;
  }
  if (auto ptr = object->GetFragmentShaderCode())
  {
    state["FragmentShaderCode"] = ptr;
  }
  if (auto ptr = object->GetGeometryShaderCode())
  {
    state["GeometryShaderCode"] = ptr;
  }
  if (auto ptr = object->GetTessControlShaderCode())
  {
    state["TessControlShaderCode"] = ptr;
  }
  if (auto ptr = object->GetTessEvaluationShaderCode())
  {
    state["TessEvaluationShaderCode"] = ptr;
  }

  auto& dst = state["Replacements"] = json::array();
  for (int i = 0; i < object->GetNumberOfShaderReplacements(); i++)
  {
    json replacement;
    replacement["ShaderType"] = object->GetNthShaderReplacementTypeAsString(i);
    std::string name;
    std::string replacementValue;
    bool replaceFirst;
    bool replaceAll;
    object->GetNthShaderReplacement(i, name, replaceFirst, replacementValue, replaceAll);
    replacement["OriginalValue"] = name;
    replacement["ReplacementValue"] = replacementValue;
    replacement["ReplaceFirst"] = replaceFirst;
    replacement["ReplaceAll"] = replaceAll;

    dst.push_back(replacement);
  }

  return state;
}

static void Deserialize_vtkShaderProperty(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  auto object = vtkShaderProperty::SafeDownCast(objectBase);

  if (auto f = deserializer->GetHandler(typeid(vtkShaderProperty::Superclass)))
  {
    f(state, object, deserializer);
  }

  {
    const auto iter = state.find("VertexShaderCode");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto values = iter->get<std::string>();
      object->SetVertexShaderCode(values.c_str());
    }
  }
  {
    const auto iter = state.find("FragmentShaderCode");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto values = iter->get<std::string>();
      object->SetFragmentShaderCode(values.c_str());
    }
  }
  {
    const auto iter = state.find("GeometryShaderCode");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto values = iter->get<std::string>();
      object->SetGeometryShaderCode(values.c_str());
    }
  }
  {
    const auto iter = state.find("TessControlShaderCode");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto values = iter->get<std::string>();
      object->SetTessControlShaderCode(values.c_str());
    }
  }
  {
    const auto iter = state.find("TessEvaluationShaderCode");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto values = iter->get<std::string>();
      object->SetTessEvaluationShaderCode(values.c_str());
    }
  }

  if (state.contains("Replacements"))
  {
    for (size_t i = 0; i < state["Replacements"].size(); i++)
    {
      auto& replacement = state["Replacements"][i];
      const std::string& originalValue = replacement["OriginalValue"];
      const std::string& replacementValue = replacement["ReplacementValue"];
      bool replaceFirst = replacement["ReplaceFirst"];
      bool replaceAll = replacement["ReplaceAll"];
      const std::string& type = replacement["ShaderType"];

      if (type == "Vertex")
      {
        object->AddVertexShaderReplacement(
          originalValue, replaceFirst, replacementValue, replaceAll);
      }
      else if (type == "Fragment")
      {
        object->AddFragmentShaderReplacement(
          originalValue, replaceFirst, replacementValue, replaceAll);
      }
      else if (type == "Geometry")
      {
        object->AddGeometryShaderReplacement(
          originalValue, replaceFirst, replacementValue, replaceAll);
      }
      else if (type == "TessControl")
      {
        object->AddTessControlShaderReplacement(
          originalValue, replaceFirst, replacementValue, replaceAll);
      }
      else if (type == "TessEvaluation")
      {
        object->AddTessEvaluationShaderReplacement(
          originalValue, replaceFirst, replacementValue, replaceAll);
      }
    }
  }
}

int RegisterHandlers_vtkShaderPropertySerDesHelper(
  void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkShaderProperty), Serialize_vtkShaderProperty);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkShaderProperty), Deserialize_vtkShaderProperty);
      deserializer->RegisterConstructor(
        "vtkShaderProperty", []() { return vtkShaderProperty::New(); });
      success = 1;
    }
  }
  return success;
}
