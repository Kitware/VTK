// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkVariantSerDesHelper.h"
#include "vtkABINamespace.h"
#include "vtkDeserializer.h"
#include "vtkSerializer.h"
#include "vtkVariant.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

VTK_ABI_NAMESPACE_BEGIN

nlohmann::json Serialize_vtkVariant(const vtkVariant* variant, vtkSerializer* serializer)
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
    state["Value"] = static_cast<std::string>(variant->ToString());
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

bool Deserialize_vtkVariant(
  const nlohmann::json& state, vtkVariant* variant, vtkDeserializer* deserializer)
{
  bool success = true;
  auto typeIter = state.find("Type");
  if (typeIter == state.end())
  {
    vtkErrorWithObjectMacro(deserializer, << __func__ << ": Missing 'Type' in JSON state.");
    return false;
  }
  const int type = typeIter->get<int>();
  auto valueIter = state.find("Value");
  if (valueIter == state.end())
  {
    vtkErrorWithObjectMacro(deserializer, << __func__ << ": Missing 'Value' in JSON state.");
    return false;
  }
  if (type == VTK_STRING)
  {
    *variant = vtkVariant(valueIter->get<std::string>());
  }
  else if (type == VTK_FLOAT)
  {
    *variant = vtkVariant(valueIter->get<float>());
  }
  else if (type == VTK_DOUBLE)
  {
    *variant = vtkVariant(valueIter->get<double>());
  }
  else if (type == VTK_OBJECT)
  {
    vtkSmartPointer<vtkObjectBase> obj;
    deserializer->DeserializeJSON(state["Value"]["Id"], obj);
    *variant = vtkVariant(obj);
  }
  else
  {
    *variant = vtkVariant(vtkVariant(valueIter->get<vtkTypeUInt64>()), type);
  }
  return success;
}

VTK_ABI_NAMESPACE_END
