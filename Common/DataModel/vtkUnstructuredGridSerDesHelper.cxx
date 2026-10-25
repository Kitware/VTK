// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellArray.h"
#include "vtkDeserializer.h"
#include "vtkPolyhedronCellArray.h"
#include "vtkSerializer.h"
#include "vtkSmartPointer.h"
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
  int RegisterHandlers_vtkUnstructuredGridSerDesHelper(void* ser, void* deser, void* invoker);
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
  state["CellTypes"] = serializer->SerializeJSON(object->GetCellTypes());
  // A polyhedron's faces are not described by its point list. They live in
  // the polyhedron cell array, so it must travel alongside the connectivity or
  // the cell's topology is lost.
  if (auto* polyhedra = object->GetPolyhedronCellArray())
  {
    state["PolyhedronCellArray"] = serializer->SerializeJSON(polyhedra);
  }
  state["MeshMTime"] = object->GetMeshMTime();
  return state;
}

static bool Deserialize_vtkUnstructuredGrid(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  bool success = true;
  auto* object = vtkUnstructuredGrid::SafeDownCast(objectBase);
  if (!object)
  {
    vtkErrorWithObjectMacro(deserializer, << __func__ << ": object not a vtkUnstructuredGrid");
    return false;
  }
  if (auto f = deserializer->GetHandler(typeid(vtkUnstructuredGrid::Superclass)))
  {
    success &= f(state, object, deserializer);
  }
  vtkSmartPointer<vtkDataArray> cellTypes;
  vtkSmartPointer<vtkCellArray> connectivity;
  {
    auto iter = state.find("CellTypes");
    if ((iter != state.end()) && !iter->is_null())
    {
      const auto* context = deserializer->GetContext();
      const auto identifier = iter->at("Id").get<vtkTypeUInt32>();
      auto subObject = context->GetObjectAtId(identifier);
      success &= deserializer->DeserializeJSON(identifier, subObject);
      cellTypes = vtkDataArray::SafeDownCast(subObject);
    }
  }
  {
    auto iter = state.find("Cells");
    if ((iter != state.end()) && !iter->is_null())
    {
      const auto* context = deserializer->GetContext();
      const auto identifier = iter->at("Id").get<vtkTypeUInt32>();
      auto subObject = context->GetObjectAtId(identifier);
      success &= deserializer->DeserializeJSON(identifier, subObject);
      connectivity = vtkCellArray::SafeDownCast(subObject);
    }
  }
  vtkSmartPointer<vtkPolyhedronCellArray> polyhedra;
  {
    auto iter = state.find("PolyhedronCellArray");
    if ((iter != state.end()) && !iter->is_null())
    {
      const auto* context = deserializer->GetContext();
      const auto identifier = iter->at("Id").get<vtkTypeUInt32>();
      auto subObject = context->GetObjectAtId(identifier);
      success &= deserializer->DeserializeJSON(identifier, subObject);
      polyhedra = vtkPolyhedronCellArray::SafeDownCast(subObject);
    }
  }
  if (cellTypes && connectivity)
  {
    if (polyhedra)
    {
      // The connectivity holds point lists. SetCells() would reinterpret it as
      // a legacy face stream as soon as a VTK_POLYHEDRON is present, so the
      // faces have to be handed over explicitly.
      object->SetPolyhedralCells(cellTypes, connectivity, polyhedra);
    }
    else
    {
      object->SetCells(cellTypes, connectivity);
    }
  }
  return success;
}

int RegisterHandlers_vtkUnstructuredGridSerDesHelper(
  void* ser, void* deser, void* vtkNotUsed(invoker))
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
