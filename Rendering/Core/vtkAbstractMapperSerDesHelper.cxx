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
  /**
   * Register the (de)serialization handlers of vtkAbstractMapper
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkAbstractMapperSerDesHelper(void* ser, void* deser);
}

static nlohmann::json Serialize_vtkAbstractMapper(
  vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto* object = vtkAbstractMapper::SafeDownCast(objectBase);
  if (object->GetClippingPlanes())
  {
    state["ClippingPlanes"] =
      serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(object->GetClippingPlanes()));
  }
  // vtkDataSetMapper is a special case handled by vtkDataSetMapperSerDes.cxx
  if (object->IsA("vtkDataSetMapper"))
  {
    // skip vtkAlgorithm
    if (auto f = serializer->GetHandler(typeid(vtkAbstractMapper::Superclass::Superclass)))
    {
      state = f(object, serializer);
    }
  }
  else
  {
    // serialize vtkAlgorithm properties
    if (auto f = serializer->GetHandler(typeid(vtkAbstractMapper::Superclass)))
    {
      state = f(object, serializer);
    }
  }
  state["SuperClassNames"].push_back("vtkAlgorithm");
  return state;
}

static void Deserialize_vtkAbstractMapper(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  auto* object = vtkAbstractMapper::SafeDownCast(objectBase);
  VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE(
    ClippingPlanes, vtkPlaneCollection, state, object, deserializer);
  // vtkDataSetMapper is a special case handled by vtkDataSetMapperSerDes.cxx
  if (object->IsA("vtkDataSetMapper"))
  {
    // skip vtkAlgorithm
    if (auto f = deserializer->GetHandler(typeid(vtkAbstractMapper::Superclass::Superclass)))
    {
      f(state, object, deserializer);
    }
  }
  else
  {
    // deserialize vtkAlgorithm properties
    if (auto f = deserializer->GetHandler(typeid(vtkAbstractMapper::Superclass)))
    {
      f(state, object, deserializer);
    }
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
