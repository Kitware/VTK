// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAbstractMapper.h"
#include "vtkDataObject.h"
#include "vtkDeserializer.h"
#include "vtkPlaneCollection.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  int RegisterHandlers_vtkAbstractMapperSerDesHelper(void* ser, void* deser);
}

static nlohmann::json Serialize_vtkAbstractMapper(
  vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto* object = vtkAbstractMapper::SafeDownCast(objectBase);
  // skip vtkAlgorithm
  if (auto f = serializer->GetHandler(typeid(vtkAbstractMapper::Superclass::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkAlgorithm");
  if (object->GetClippingPlanes())
  {
    state["ClippingPlanes"] =
      serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(object->GetClippingPlanes()));
  }
  // vtkDataSetMapper is a special case handled by vtkDataSetMapperSerialization.cxx
  if (object->IsA("vtkDataSetMapper"))
  {
    return state;
  }
  int outPort = 0;
  if (auto* inputAlg = object->GetInputAlgorithm(0, 0, outPort))
  {
    inputAlg->Update(outPort);
    if (auto* inputData = inputAlg->GetOutputDataObject(outPort))
    {
      state["ExtractedInputData"] = serializer->SerializeJSON(inputData);
    }
  }
  return state;
}

static void Deserialize_vtkAbstractMapper(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  auto* object = vtkAbstractMapper::SafeDownCast(objectBase);
  if (auto f = deserializer->GetHandler(typeid(vtkAbstractMapper::Superclass::Superclass)))
  {
    f(state, object, deserializer);
  }
  VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE(
    ClippingPlanes, vtkPlaneCollection, state, object, deserializer);
  // vtkDataSetMapper is a special case handled by vtkDataSetMapperSerialization.cxx
  if (object->IsA("vtkDataSetMapper"))
  {
    return;
  }
  else
  {
    VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE_DIFFERENT_NAMES(
      ExtractedInputData, InputDataObject, vtkDataObject, state, object, deserializer);
  }
}

int RegisterHandlers_vtkAbstractMapperSerDesHelper(void* ser, void* deser)
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkAbstractMapper), Serialize_vtkAbstractMapper);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkAbstractMapper), Deserialize_vtkAbstractMapper);
      deserializer->RegisterConstructor(
        "vtkAbstractMapper", []() { return vtkAbstractMapper::New(); });
      success = 1;
    }
  }
  return success;
}
