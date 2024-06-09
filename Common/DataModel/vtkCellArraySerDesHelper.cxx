// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkDeserializer.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkCellArray
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkCellArraySerDesHelper(void* ser, void* deser);
}

static nlohmann::json Serialize_vtkCellArray(vtkObjectBase* object, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  if (auto* cellArray = vtkCellArray::SafeDownCast(object))
  {
    json state;
    if (auto superSerializer = serializer->GetHandler(typeid(vtkCellArray::Superclass)))
    {
      state = superSerializer(object, serializer);
    }
    state["Offsets"] = serializer->SerializeJSON(cellArray->GetOffsetsArray());
    state["Connectivity"] = serializer->SerializeJSON(cellArray->GetConnectivityArray());
    state["NumberOfCells"] = cellArray->GetNumberOfCells();
    return state;
  }
  else
  {
    return {};
  }
}

static void Deserialize_vtkCellArray(
  const nlohmann::json& state, vtkObjectBase* object, vtkDeserializer* deserializer)
{
  if (auto* cellArray = vtkCellArray::SafeDownCast(object))
  {
    if (auto superDeserializer = deserializer->GetHandler(typeid(vtkCellArray::Superclass)))
    {
      superDeserializer(state, object, deserializer);
    }
    const auto numberOfCells = state["NumberOfCells"].get<vtkIdType>();
    if (numberOfCells == 0)
    {
      return;
    }
    vtkSmartPointer<vtkDataArray> offsets;
    vtkSmartPointer<vtkDataArray> connectivity;
    const auto* context = deserializer->GetContext();
    {
      const auto identifier = state["Offsets"]["Id"].get<vtkTypeUInt32>();
      auto subObject = context->GetObjectAtId(identifier);
      deserializer->DeserializeJSON(identifier, subObject);
      offsets = vtkDataArray::SafeDownCast(subObject);
    }
    {
      const auto identifier = state["Connectivity"]["Id"].get<vtkTypeUInt32>();
      auto subObject = context->GetObjectAtId(identifier);
      deserializer->DeserializeJSON(identifier, subObject);
      connectivity = vtkDataArray::SafeDownCast(subObject);
    }
    if (offsets == nullptr)
    {
      vtkErrorWithObjectMacro(context, << deserializer->GetObjectDescription()
                                       << " gave offsets=nullptr for "
                                       << cellArray->GetObjectDescription());
    }
    else if (connectivity == nullptr)
    {
      vtkErrorWithObjectMacro(context, << deserializer->GetObjectDescription()
                                       << " gave connectivity=nullptr for "
                                       << cellArray->GetObjectDescription());
    }
    else
    {
      cellArray->SetData(offsets, connectivity);
    }
  }
}

int RegisterHandlers_vtkCellArraySerDesHelper(void* ser, void* deser)
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkCellArray), Serialize_vtkCellArray);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkCellArray), Deserialize_vtkCellArray);
      deserializer->RegisterConstructor("vtkCellArray", vtkCellArray::New);
      success = 1;
    }
  }
  return success;
}
