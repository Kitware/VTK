// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDeserializer.h"
#include "vtkPiecewiseFunction.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkPiecewiseFunction
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkPiecewiseFunctionSerDesHelper(void* ser, void* deser);
}
static nlohmann::json Serialize_vtkPiecewiseFunction(
  vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto* object = vtkPiecewiseFunction::SafeDownCast(objectBase);
  if (auto f = serializer->GetHandler(typeid(vtkPiecewiseFunction::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkDataObject");
  state["Clamping"] = object->GetClamping();
  state["UseLogScale"] = object->GetUseLogScale();
  state["AllowDuplicateScalars"] = object->GetAllowDuplicateScalars();
  state["CustomSearchMethod"] = object->GetCustomSearchMethod();
  {
    auto* elementsPtr = object->GetDataPointer();
    auto size = object->GetSize() << 1;
    state["Data"] = std::vector<double>(elementsPtr, elementsPtr + size);
  }
  return state;
}

static void Deserialize_vtkPiecewiseFunction(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  auto* object = vtkPiecewiseFunction::SafeDownCast(objectBase);
  if (auto f = deserializer->GetHandler(typeid(vtkPiecewiseFunction::Superclass)))
  {
    f(state, object, deserializer);
  }
  VTK_DESERIALIZE_VALUE_FROM_STATE(Clamping, int, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(UseLogScale, bool, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(AllowDuplicateScalars, int, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(CustomSearchMethod, int, state, object);
  {
    const auto iter = state.find("Data");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto elements = iter->get<std::vector<double>>();
      const int nb = static_cast<int>(elements.size()) >> 1;
      object->FillFromDataPointer(nb, elements.data());
    }
  }
}

int RegisterHandlers_vtkPiecewiseFunctionSerDesHelper(void* ser, void* deser)
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkPiecewiseFunction), Serialize_vtkPiecewiseFunction);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkPiecewiseFunction), Deserialize_vtkPiecewiseFunction);
      deserializer->RegisterConstructor(
        "vtkPiecewiseFunction", []() { return vtkPiecewiseFunction::New(); });
      success = 1;
    }
  }
  return success;
}
