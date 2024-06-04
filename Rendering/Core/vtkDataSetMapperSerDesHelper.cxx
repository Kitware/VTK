// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataSetMapper.h"
#include "vtkDeserializer.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkDataSetMapper
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkDataSetMapperSerDesHelper(void* ser, void* deser);
}

static nlohmann::json Serialize_vtkDataSetMapper(
  vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto* object = vtkDataSetMapper::SafeDownCast(objectBase);
  if (auto f = serializer->GetHandler(typeid(vtkDataSetMapper::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkMapper");
  if (auto polyDataMapper = object->GetPolyDataMapper())
  {
    auto* inputAlgorithm = polyDataMapper->GetInputAlgorithm();
    inputAlgorithm->Update(0);
    auto* polyData = inputAlgorithm->GetOutputDataObject(0);
    if (polyData)
    {
      state["ExtractedPolyData"] = serializer->SerializeJSON(polyData);
    }
  }
  return state;
}

static void Deserialize_vtkDataSetMapper(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  auto* object = vtkDataSetMapper::SafeDownCast(objectBase);
  if (auto f = deserializer->GetHandler(typeid(vtkDataSetMapper::Superclass)))
  {
    f(state, object, deserializer);
  }
  VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE_DIFFERENT_NAMES(
    ExtractedPolyData, InputData, vtkPolyData, state, object, deserializer);
}

int RegisterHandlers_vtkDataSetMapperSerDesHelper(void* ser, void* deser)
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkDataSetMapper), Serialize_vtkDataSetMapper);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkDataSetMapper), Deserialize_vtkDataSetMapper);
      deserializer->RegisterConstructor(
        "vtkDataSetMapper", []() { return vtkDataSetMapper::New(); });
      success = 1;
    }
  }
  return success;
}
