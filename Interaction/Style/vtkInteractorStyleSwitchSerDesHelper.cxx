// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDeserializer.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkInteractorStyleSwitch
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkInteractorStyleSwitchSerDesHelper(void* ser, void* deser);
}

namespace
{
const std::vector<std::string> possibleStyles = { "vtkInteractorStyleJoystickActor",
  "vtkInteractorStyleJoystickCamera", "vtkInteractorStyleTrackballActor",
  "vtkInteractorStyleTrackballCamera", "vtkInteractorStyleMultiTouchCamera" };
}

static nlohmann::json Serialize_vtkInteractorStyleSwitch(
  vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto* object = vtkInteractorStyleSwitch::SafeDownCast(objectBase);
  if (auto f = serializer->GetHandler(typeid(vtkInteractorStyleSwitch::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkInteractorStyleSwitchBase");
  if (auto currentStyle = object->GetCurrentStyle())
  {
    vtkTypeUInt8 styleIndex = 0;
    for (const auto& styleName : possibleStyles)
    {
      if (currentStyle->IsA(styleName.c_str()))
      {
        state["CurrentStyleIndex"] = styleIndex;
        break;
      }
      styleIndex++;
    }
  }
  return state;
}

static void Deserialize_vtkInteractorStyleSwitch(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  auto* object = vtkInteractorStyleSwitch::SafeDownCast(objectBase);
  if (auto f = deserializer->GetHandler(typeid(vtkInteractorStyleSwitch::Superclass)))
  {
    f(state, object, deserializer);
  }
  const auto iter = state.find("CurrentStyleIndex");
  if ((iter != state.end()) && !iter->is_null())
  {
    const auto styleIndex = iter->get<vtkTypeUInt8>();
    switch (styleIndex)
    {
      case 0:
        object->SetCurrentStyleToJoystickActor();
        break;
      case 1:
        object->SetCurrentStyleToJoystickCamera();
        break;
      case 2:
        object->SetCurrentStyleToTrackballActor();
        break;
      case 3:
        object->SetCurrentStyleToTrackballCamera();
        break;
      case 4:
        object->SetCurrentStyleToMultiTouchCamera();
        break;
      default:
        vtkErrorWithObjectMacro(deserializer,
          "No style exists at styleIndex=" << styleIndex
                                           << " for vtkInteractorStyleSwitch::SetCurrentStyle. "
                                              "Value is expected to be in range [0, 4]");
        break;
    }
  }
}

int RegisterHandlers_vtkInteractorStyleSwitchSerDesHelper(void* ser, void* deser)
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(
        typeid(vtkInteractorStyleSwitch), Serialize_vtkInteractorStyleSwitch);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(
        typeid(vtkInteractorStyleSwitch), Deserialize_vtkInteractorStyleSwitch);
      deserializer->RegisterConstructor(
        "vtkInteractorStyleSwitch", []() { return vtkInteractorStyleSwitch::New(); });
      success = 1;
    }
  }
  return success;
}
