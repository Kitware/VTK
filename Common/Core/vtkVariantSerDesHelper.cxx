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

void Deserialize_vtkVariant(
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

VTK_ABI_NAMESPACE_END
