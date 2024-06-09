// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkColorTransferFunction.h"
#include "vtkDeserializer.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkColorTransferFunction
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkColorTransferFunctionSerDesHelper(void* ser, void* deser);
}

static nlohmann::json Serialize_vtkColorTransferFunction(
  vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto* object = vtkColorTransferFunction::SafeDownCast(objectBase);
  if (auto f = serializer->GetHandler(typeid(vtkColorTransferFunction::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkScalarsToColors");
  state["Clamping"] = object->GetClamping();
  state["ColorSpace"] = object->GetColorSpace();
  state["HSVWrap"] = object->GetHSVWrap();
  state["Scale"] = object->GetScale();
  if (auto ptr = object->GetNanColor())
  {
    auto& dst = state["NanColor"] = json::array();
    for (int i = 0; i < 3; ++i)
    {
      dst.push_back(ptr[i]);
    }
  }
  state["NanOpacity"] = object->GetNanOpacity();
  if (auto ptr = object->GetBelowRangeColor())
  {
    auto& dst = state["BelowRangeColor"] = json::array();
    for (int i = 0; i < 3; ++i)
    {
      dst.push_back(ptr[i]);
    }
  }
  state["UseBelowRangeColor"] = object->GetUseBelowRangeColor();
  if (auto ptr = object->GetAboveRangeColor())
  {
    auto& dst = state["AboveRangeColor"] = json::array();
    for (int i = 0; i < 3; ++i)
    {
      dst.push_back(ptr[i]);
    }
  }
  state["UseAboveRangeColor"] = object->GetUseAboveRangeColor();
  state["AllowDuplicateScalars"] = object->GetAllowDuplicateScalars();
  state["NumberOfAvailableColors"] = object->GetNumberOfAvailableColors();
  {
    auto elements = object->GetDataPointer();
    const auto size = object->GetSize() << 2;
    state["Data"] = std::vector<double>(elements, elements + size);
  }
  return state;
}

static void Deserialize_vtkColorTransferFunction(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  auto* object = vtkColorTransferFunction::SafeDownCast(objectBase);
  if (auto f = deserializer->GetHandler(typeid(vtkColorTransferFunction::Superclass)))
  {
    f(state, object, deserializer);
  }
  VTK_DESERIALIZE_VALUE_FROM_STATE(Clamping, int, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(ColorSpace, int, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(HSVWrap, int, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(Scale, int, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(NanOpacity, double, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(UseAboveRangeColor, int, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(UseBelowRangeColor, int, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(AllowDuplicateScalars, int, state, object);

  VTK_DESERIALIZE_VECTOR_FROM_STATE(NanColor, double, state, object);
  VTK_DESERIALIZE_VECTOR_FROM_STATE(AboveRangeColor, double, state, object);
  VTK_DESERIALIZE_VECTOR_FROM_STATE(BelowRangeColor, double, state, object);

  {
    const auto iter = state.find("Data");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto elements = iter->get<std::vector<double>>();
      const auto nb = static_cast<int>(elements.size()) >> 2;
      object->FillFromDataPointer(nb, elements.data());
    }
  }
}

int RegisterHandlers_vtkColorTransferFunctionSerDesHelper(void* ser, void* deser)
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(
        typeid(vtkColorTransferFunction), Serialize_vtkColorTransferFunction);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(
        typeid(vtkColorTransferFunction), Deserialize_vtkColorTransferFunction);
      deserializer->RegisterConstructor(
        "vtkColorTransferFunction", []() { return vtkColorTransferFunction::New(); });
      success = 1;
    }
  }
  return success;
}
