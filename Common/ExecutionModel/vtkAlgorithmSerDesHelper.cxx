// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkDeserializer.h"
#include "vtkExecutive.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  int RegisterHandlers_vtkAlgorithmSerDesHelper(void* ser, void* deser);
}

static nlohmann::json Serialize_vtkAlgorithm(vtkObjectBase* object, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  if (auto* algorithm = vtkAlgorithm::SafeDownCast(object))
  {
    json state;
    if (auto superSerializer = serializer->GetHandler(typeid(vtkAlgorithm::Superclass)))
    {
      state = superSerializer(object, serializer);
    }
    // Push super info
    state["SuperClassNames"].push_back("vtkObject");
    if (algorithm->GetNumberOfOutputPorts() > 0)
    {
      if (auto outputDataObject = algorithm->GetOutputDataObject(0))
      {
        state["OutputDataObject"] =
          serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(outputDataObject));
      }
    }
    state["AbortExecute"] = algorithm->GetAbortExecute();
    auto& stateOfPorts = state["InputPorts"] = json::array();
    for (int port = 0; port < algorithm->GetNumberOfInputPorts(); ++port)
    {
      auto stateOfConnections = json::array();
      for (int index = 0; index < algorithm->GetNumberOfInputConnections(port); ++index)
      {
        auto* inputConnection = algorithm->GetInputConnection(port, index);
        stateOfConnections.push_back(serializer->SerializeJSON(inputConnection));
      }
      stateOfPorts.push_back(stateOfConnections);
    }
    return state;
  }
  else
  {
    return {};
  }
}

static void Deserialize_vtkAlgorithm(
  const nlohmann::json& state, vtkObjectBase* object, vtkDeserializer* deserializer)
{
  using json = nlohmann::json;
  auto* algorithm = vtkAlgorithm::SafeDownCast(object);
  if (!algorithm)
  {
    return;
  }
  const auto* context = deserializer->GetContext();
  VTK_DESERIALIZE_VALUE_FROM_STATE(AbortExecute, int, state, algorithm);
  {
    const auto iter = state.find("InputPorts");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto stateOfPorts = iter->get<json::array_t>();
      if (algorithm->GetNumberOfInputPorts() != static_cast<int>(stateOfPorts.size()))
      {
        vtkWarningWithObjectMacro(
          context, << deserializer->GetObjectDescription()
                   << " failed because number of input ports does not match for algorithm="
                   << algorithm->GetObjectDescription());
        return;
      }
      // Deserialize output ports and add them as input connections
      for (int port = 0; port < algorithm->GetNumberOfInputPorts(); ++port)
      {
        algorithm->RemoveAllInputConnections(port);
        auto stateOfConnections = stateOfPorts[port].get<json::array_t>();
        for (std::size_t index = 0; index < stateOfConnections.size(); ++index)
        {
          const auto identifier = stateOfConnections[index]["Id"].get<vtkTypeUInt32>();
          auto subObject = context->GetObjectAtId(identifier);
          deserializer->DeserializeJSON(identifier, subObject);
          if (auto* outputPort = vtkAlgorithmOutput::SafeDownCast(subObject))
          {
            algorithm->AddInputConnection(port, outputPort);
          }
        }
      }
    }
  }
}

int RegisterHandlers_vtkAlgorithmSerDesHelper(void* ser, void* deser)
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkAlgorithm), Serialize_vtkAlgorithm);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkAlgorithm), Deserialize_vtkAlgorithm);
      deserializer->RegisterConstructor("vtkAlgorithm", vtkAlgorithm::New);
      success = 1;
    }
  }
  return success;
}
