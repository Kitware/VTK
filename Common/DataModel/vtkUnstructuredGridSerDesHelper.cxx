// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellArray.h"
#include "vtkDeserializer.h"
#include "vtkSerializer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkUnstructuredGrid
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkUnstructuredGridSerDesHelper(void* ser, void* deser);
}

static nlohmann::json Serialize_vtkUnstructuredGrid(
  vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto* object = vtkUnstructuredGrid::SafeDownCast(objectBase);
  if (auto f = serializer->GetHandler(typeid(vtkUnstructuredGrid::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkUnstructuredGridBase");
  state["DataObjectType"] = object->GetDataObjectType();
  state["Cells"] = serializer->SerializeJSON(object->GetCells());
  state["CellTypes"] = serializer->SerializeJSON(object->GetCellTypesArray());
  state["MeshMTime"] = object->GetMeshMTime();
  return state;
}

static void Deserialize_vtkUnstructuredGrid(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  auto* object = vtkUnstructuredGrid::SafeDownCast(objectBase);
  if (auto f = deserializer->GetHandler(typeid(vtkUnstructuredGrid::Superclass)))
  {
    f(state, object, deserializer);
  }

  vtkUnsignedCharArray* cellTypes = nullptr;
  vtkCellArray* connectivity = nullptr;
  {
    auto iter = state.find("CellTypes");
    if ((iter != state.end()) && !iter->is_null())
    {
      const auto* context = deserializer->GetContext();
      const auto identifier = iter->at("Id").get<vtkTypeUInt32>();
      auto subObject = context->GetObjectAtId(identifier);
      deserializer->DeserializeJSON(identifier, subObject);
      cellTypes = vtkUnsignedCharArray::SafeDownCast(subObject);
    }
  }
  {
    auto iter = state.find("Cells");
    if ((iter != state.end()) && !iter->is_null())
    {
      const auto* context = deserializer->GetContext();
      const auto identifier = iter->at("Id").get<vtkTypeUInt32>();
      auto subObject = context->GetObjectAtId(identifier);
      deserializer->DeserializeJSON(identifier, subObject);
      connectivity = vtkCellArray::SafeDownCast(subObject);
    }
  }
  if (cellTypes && connectivity)
  {
    object->SetCells(cellTypes, connectivity);
  }
}

int RegisterHandlers_vtkUnstructuredGridSerDesHelper(void* ser, void* deser)
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkUnstructuredGrid), Serialize_vtkUnstructuredGrid);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkUnstructuredGrid), Deserialize_vtkUnstructuredGrid);
      deserializer->RegisterConstructor(
        "vtkUnstructuredGrid", []() { return vtkUnstructuredGrid::New(); });
      success = 1;
    }
  }
  return success;
}
