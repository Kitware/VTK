// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAlgorithm.h"
#include "vtkDataObject.h"
#include "vtkDeserializer.h"
#include "vtkInformation.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkAlgorithm
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkAlgorithmSerDesHelper(void* ser, void* deser, void* invoker);
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
    // the pipeline is servered here by capturing only the input data objects
    // in the state.
    auto& statesOfInputDataObjects = state["InputDataObjects"] = json::array();
    for (int port = 0; port < algorithm->GetNumberOfInputPorts(); ++port)
    {
      auto stateOfInputDataObjects = json::array();
      for (int index = 0; index < algorithm->GetNumberOfInputConnections(port); ++index)
      {
        auto* inputAlgorithm = algorithm->GetInputAlgorithm(port, index);
        inputAlgorithm->Update();
        auto* inputDataObject = algorithm->GetInputDataObject(port, index);
        stateOfInputDataObjects.push_back(serializer->SerializeJSON(inputDataObject));
      }
      statesOfInputDataObjects.push_back(stateOfInputDataObjects);
    }

    state["Information"] =
      serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(algorithm->GetInformation()));

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
  VTK_DESERIALIZE_VALUE_FROM_STATE(AbortExecute, int, state, algorithm);

  VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE(
    Information, vtkInformation, state, algorithm, deserializer);

  {
    const auto* context = deserializer->GetContext();
    const auto iter = state.find("InputDataObjects");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto statesOfInputDataObjects = iter->get<json::array_t>();
      if (algorithm->GetNumberOfInputPorts() != static_cast<int>(statesOfInputDataObjects.size()))
      {
        vtkWarningWithObjectMacro(context,
          << deserializer->GetObjectDescription()
          << " failed because number of input ports in state (" << statesOfInputDataObjects.size()
          << ") does not match for algorithm=" << algorithm->GetObjectDescription() << " ("
          << algorithm->GetNumberOfInputPorts() << ")");
        return;
      }
      for (int port = 0; port < algorithm->GetNumberOfInputPorts(); ++port)
      {
        std::vector<vtkSmartPointer<vtkDataObject>> inputDataObjects;
        auto stateOfInputDataObjects = statesOfInputDataObjects[port].get<json::array_t>();
        const bool hasMultipleConnections = stateOfInputDataObjects.size() > 1;
        for (std::size_t index = 0; index < stateOfInputDataObjects.size(); ++index)
        {
          const auto identifier = stateOfInputDataObjects[index]["Id"].get<vtkTypeUInt32>();
          auto subObject = context->GetObjectAtId(identifier);
          deserializer->DeserializeJSON(identifier, subObject);
          if (auto* dataObject = vtkDataObject::SafeDownCast(subObject))
          {
            if (hasMultipleConnections)
            {
              inputDataObjects.emplace_back(dataObject);
            }
            else
            {
              algorithm->SetInputDataObject(port, dataObject);
            }
          }
        }
        if (hasMultipleConnections)
        {
          algorithm->RemoveAllInputConnections(port);
          for (auto& dataObject : inputDataObjects)
          {
            algorithm->AddInputDataObject(port, dataObject);
          }
        }
      }
    }
  }
}

int RegisterHandlers_vtkAlgorithmSerDesHelper(void* ser, void* deser, void* vtkNotUsed(invoker))
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
