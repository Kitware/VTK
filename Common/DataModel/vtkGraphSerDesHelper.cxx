// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkDeserializer.h"
#include "vtkGraph.h"
#include "vtkGraphInternals.h"
#include "vtkInformation.h"
#include "vtkPoints.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include <cstdint>
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkGraph
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkGraphSerDesHelper(void* ser, void* deser, void* invoker);
}

static nlohmann::json Serialize_vtkGraph(vtkObjectBase* object, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  if (auto* graph = vtkGraph::SafeDownCast(object))
  {
    json state;
    if (auto superSerializer = serializer->GetHandler(typeid(vtkGraph::Superclass)))
    {
      state = superSerializer(object, serializer);
    }
    state["SuperClassNames"].push_back("vtkObject");

    {
      auto value = graph->GetVertexData();
      if (value)
      {
        state["VertexData"] = serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(value));
      }
    }
    {
      auto value = graph->GetEdgeData();
      if (value)
      {
        state["EdgeData"] = serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(value));
      }
    }
    {
      auto value = graph->GetPoints();
      if (value)
      {
        state["Points"] = serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(value));
      }
    }

    vtkGraphInternals* internals = graph->GetGraphInternals(false);
    auto& graphInternalState = state["InternalAdjacency"] = json::array();
    for (const auto& adj : internals->Adjacency)
    {
      json adjListState;
      auto& inEdgesState = adjListState["InEdges"] = json::array();
      auto& outEdgesState = adjListState["OutEdges"] = json::array();
      for (const auto& inEdge : adj.InEdges)
      {
        inEdgesState.push_back({ inEdge.Id, inEdge.Source });
      }
      for (const auto& outEdge : adj.OutEdges)
      {
        outEdgesState.push_back({ outEdge.Id, outEdge.Target });
      }
      graphInternalState.push_back(adjListState);
    }
    state["NumberOfEdges"] = internals->NumberOfEdges;
    state["UsingPedigreeIds"] = internals->UsingPedigreeIds;

    return state;
  }
  else
  {
    return {};
  }
}

static bool Deserialize_vtkGraph(
  const nlohmann::json& state, vtkObjectBase* object, vtkDeserializer* deserializer)
{
  bool success = true;
  auto* graph = vtkGraph::SafeDownCast(object);
  if (!graph)
  {
    vtkErrorWithObjectMacro(deserializer, << __func__ << ": object not a vtkGraph");
    return false;
  }

  if (auto superDeserializer = deserializer->GetHandler(typeid(vtkGraph::Superclass)))
  {
    success &= superDeserializer(state, object, deserializer);
  }
  if (!success)
  {
    return false;
  }
  vtkGraphInternals* internals = graph->GetGraphInternals(true);
  std::vector<vtkVertexAdjacencyList> adjacency;
  for (const auto& adj : state["InternalAdjacency"])
  {
    vtkVertexAdjacencyList adjList;

    for (const auto& edge : adj["InEdges"])
    {
      adjList.InEdges.emplace_back(edge[1], edge[0]);
    }
    for (const auto& edge : adj["OutEdges"])
    {
      adjList.OutEdges.emplace_back(edge[1], edge[0]);
    }
    adjacency.push_back(adjList);
  }
  internals->Adjacency = adjacency;
  internals->NumberOfEdges = state["NumberOfEdges"];
  internals->UsingPedigreeIds = state["UsingPedigreeIds"];

  {
    auto iter = state.find("VertexData");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto* context = deserializer->GetContext();
      const auto identifier = iter->at("Id").get<vtkTypeUInt32>();
      vtkSmartPointer<vtkObjectBase> subObject =
        reinterpret_cast<vtkObjectBase*>(graph->GetVertexData());
      if (subObject == nullptr)
      {
        vtkErrorWithObjectMacro(context, << "An internal collection object is null!");
      }
      else
      {
        if (context->GetObjectAtId(identifier) != subObject)
        {
          auto registrationId = identifier;
          context->RegisterObject(subObject, registrationId);
        }
        success &= deserializer->DeserializeJSON(identifier, subObject);
      }
    }
  }
  {
    auto iter = state.find("EdgeData");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto* context = deserializer->GetContext();
      const auto identifier = iter->at("Id").get<vtkTypeUInt32>();
      vtkSmartPointer<vtkObjectBase> subObject =
        reinterpret_cast<vtkObjectBase*>(graph->GetEdgeData());
      if (subObject == nullptr)
      {
        vtkErrorWithObjectMacro(context, << "An internal collection object is null!");
      }
      else
      {
        if (context->GetObjectAtId(identifier) != subObject)
        {
          auto registrationId = identifier;
          context->RegisterObject(subObject, registrationId);
        }
        success &= deserializer->DeserializeJSON(identifier, subObject);
      }
    }
  }
  VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE(Points, vtkPoints, state, graph, deserializer);
  return success;
}

int RegisterHandlers_vtkGraphSerDesHelper(void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkGraph), Serialize_vtkGraph);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkGraph), Deserialize_vtkGraph);
      deserializer->RegisterConstructor("vtkGraph", vtkGraph::New);
      success = 1;
    }
  }
  return success;
}
