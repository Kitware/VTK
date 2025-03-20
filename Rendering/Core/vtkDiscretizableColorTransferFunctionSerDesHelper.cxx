// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDeserializer.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkDiscretizableColorTransferFunction
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkDiscretizableColorTransferFunctionSerDesHelper(
    void* ser, void* deser, void* invoker);
}

static nlohmann::json Serialize_vtkDiscretizableColorTransferFunction(
  vtkObjectBase* object, vtkSerializer* serializer)
{
  using nlohmann::json;
  if (auto* dctf = vtkDiscretizableColorTransferFunction::SafeDownCast(object))
  {
    json state;
    if (auto superSerializer =
          serializer->GetHandler(typeid(vtkDiscretizableColorTransferFunction::Superclass)))
    {
      state = superSerializer(object, serializer);
    }

    state["SuperClassNames"].push_back("vtkColorTransferFunction");
    state["NumberOfIndexedColors"] = dctf->GetNumberOfIndexedColors();
    state["Discretize"] = dctf->GetDiscretize();
    state["UseLogScale"] = dctf->GetUseLogScale();
    state["NumberOfValues"] = dctf->GetNumberOfValues();

    if (dctf->GetNumberOfIndexedColors() > 0)
    {
      auto& dst = state["IndexedColors"] = json::array();
      for (unsigned int i = 0; i < dctf->GetNumberOfIndexedColors(); ++i)
      {
        double rgba[4];
        dctf->GetIndexedColor(i, rgba);
        dst.push_back(rgba);
      }
    }

    {
      auto value = dctf->GetScalarOpacityFunction();
      if (value)
      {
        state["ScalarOpacityFunction"] =
          serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(value));
      }
    }

    state["EnableOpacityMapping"] = dctf->GetEnableOpacityMapping();
    return state;
  }
  else
  {
    return {};
  }
}

static void Deserialize_vtkDiscretizableColorTransferFunction(
  const nlohmann::json& state, vtkObjectBase* object, vtkDeserializer* deserializer)
{
  using nlohmann::json;
  if (auto* dctf = vtkDiscretizableColorTransferFunction::SafeDownCast(object))
  {
    if (auto superDeserializer =
          deserializer->GetHandler(typeid(vtkDiscretizableColorTransferFunction::Superclass)))
    {
      superDeserializer(state, object, deserializer);
    }

    {
      const auto iter = state.find("NumberOfIndexedColors");
      if ((iter != state.end()) && !iter->is_null())
      {
        dctf->SetNumberOfIndexedColors(iter->get<unsigned int>());
      }
    }
    {
      const auto iter = state.find("Discretize");
      if ((iter != state.end()) && !iter->is_null())
      {
        dctf->SetDiscretize(iter->get<int>());
      }
    }
    {
      const auto iter = state.find("UseLogScale");
      if ((iter != state.end()) && !iter->is_null())
      {
        dctf->SetUseLogScale(iter->get<int>());
      }
    }
    {
      const auto iter = state.find("NumberOfValues");
      if ((iter != state.end()) && !iter->is_null())
      {
        dctf->SetNumberOfValues(iter->get<long long>());
      }
    }
    {
      auto iter = state.find("ScalarOpacityFunction");
      if ((iter != state.end()) && !iter->is_null())
      {
        const auto* context = deserializer->GetContext();
        const auto identifier = iter->at("Id").get<vtkTypeUInt32>();
        auto subObject = context->GetObjectAtId(identifier);
        deserializer->DeserializeJSON(identifier, subObject);
        if (auto* pwf = vtkPiecewiseFunction::SafeDownCast(subObject))
        {
          dctf->SetScalarOpacityFunction(pwf);
        }
      }
    }
    {
      const auto iter = state.find("EnableOpacityMapping");
      if ((iter != state.end()) && !iter->is_null())
      {
        dctf->SetEnableOpacityMapping(iter->get<bool>());
      }
    }

    {
      const auto iter = state.find("IndexedColors");
      if ((iter != state.end()) && !iter->is_null())
      {
        auto indexedColors = iter->get<std::vector<std::vector<double>>>();
        for (unsigned int i = 0; i < indexedColors.size(); ++i)
        {
          dctf->SetIndexedColorRGBA(i, indexedColors[i].data());
        }
      }
    }
  }
}

int RegisterHandlers_vtkDiscretizableColorTransferFunctionSerDesHelper(
  void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkDiscretizableColorTransferFunction),
        Serialize_vtkDiscretizableColorTransferFunction);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkDiscretizableColorTransferFunction),
        Deserialize_vtkDiscretizableColorTransferFunction);
      deserializer->RegisterConstructor("vtkDiscretizableColorTransferFunction",
        []() { return vtkDiscretizableColorTransferFunction::New(); });
      success = 1;
    }
  }
  return success;
}
