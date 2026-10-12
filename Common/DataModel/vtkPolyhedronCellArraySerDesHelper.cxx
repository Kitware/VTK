// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPolyhedronCellArray.h"

#include "vtkCellArray.h"
#include "vtkDeserializer.h"
#include "vtkSerializer.h"
#include "vtkSmartPointer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkPolyhedronCellArray
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkPolyhedronCellArraySerDesHelper(void* ser, void* deser, void* invoker);
}

static nlohmann::json Serialize_vtkPolyhedronCellArray(
  vtkObjectBase* object, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  if (auto* polyhedra = vtkPolyhedronCellArray::SafeDownCast(object))
  {
    json state;
    if (auto superSerializer = serializer->GetHandler(typeid(vtkPolyhedronCellArray::Superclass)))
    {
      state = superSerializer(object, serializer);
    }
    state["Faces"] = serializer->SerializeJSON(polyhedra->GetFaces());
    state["FaceLocations"] = serializer->SerializeJSON(polyhedra->GetFaceLocations());
    return state;
  }
  else
  {
    return {};
  }
}

static bool Deserialize_vtkPolyhedronCellArray(
  const nlohmann::json& state, vtkObjectBase* object, vtkDeserializer* deserializer)
{
  bool success = true;
  auto* polyhedra = vtkPolyhedronCellArray::SafeDownCast(object);
  if (!polyhedra)
  {
    vtkErrorWithObjectMacro(deserializer, << __func__ << ": object not a vtkPolyhedronCellArray");
    return false;
  }
  if (auto superDeserializer = deserializer->GetHandler(typeid(vtkPolyhedronCellArray::Superclass)))
  {
    success &= superDeserializer(state, object, deserializer);
  }
  if (!success)
  {
    return false;
  }
  auto* context = deserializer->GetContext();
  const auto readCellArray = [&](const char* key)
  {
    const auto identifier = state[key]["Id"].get<vtkTypeUInt32>();
    auto subObject = context->GetObjectAtId(identifier);
    success &= deserializer->DeserializeJSON(identifier, subObject);
    return vtkSmartPointer<vtkCellArray>(vtkCellArray::SafeDownCast(subObject));
  };
  auto faces = readCellArray("Faces");
  auto faceLocations = readCellArray("FaceLocations");
  if (faces == nullptr || faceLocations == nullptr)
  {
    vtkErrorWithObjectMacro(context, << deserializer->GetObjectDescription()
                                     << " gave no faces or face locations for "
                                     << polyhedra->GetObjectDescription());
    return false;
  }
  polyhedra->SetData(faces, faceLocations);
  return success;
}

int RegisterHandlers_vtkPolyhedronCellArraySerDesHelper(
  void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkPolyhedronCellArray), Serialize_vtkPolyhedronCellArray);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(
        typeid(vtkPolyhedronCellArray), Deserialize_vtkPolyhedronCellArray);
      deserializer->RegisterConstructor("vtkPolyhedronCellArray", vtkPolyhedronCellArray::New);
      success = 1;
    }
  }
  return success;
}
