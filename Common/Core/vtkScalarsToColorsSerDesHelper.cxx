// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAbstractArray.h"
#include "vtkDeserializer.h"
#include "vtkObjectBase.h"
#include "vtkScalarsToColors.h"
#include "vtkSerializer.h"
#include "vtkStringArray.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include <cstdint>
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkScalarsToColors
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkScalarsToColorsSerDesHelper(void* ser, void* deser, void* invoker);
}

static nlohmann::json Serialize_vtkScalarsToColors(vtkObjectBase* object, vtkSerializer* serializer)
{
  using nlohmann::json;
  if (auto* stc = vtkScalarsToColors::SafeDownCast(object))
  {
    json state;
    if (auto superSerializer = serializer->GetHandler(typeid(vtkScalarsToColors::Superclass)))
    {
      state = superSerializer(object, serializer);
    }
    state["SuperClassNames"].push_back("vtkObject");

    if (auto ptr = stc->GetRange())
    {
      auto& dst = state["Range"] = json::array();
      for (int i = 0; i < 2; ++i)
      {
        dst.push_back(ptr[i]);
      }
    }
    state["Alpha"] = stc->GetAlpha();
    state["VectorMode"] = stc->GetVectorMode();
    state["VectorComponent"] = stc->GetVectorComponent();
    state["VectorSize"] = stc->GetVectorSize();
    state["IndexedLookup"] = stc->GetIndexedLookup();

    state["AnnotatedValues"] = serializer->SerializeJSON(stc->GetAnnotatedValues());
    state["Annotations"] = serializer->SerializeJSON(stc->GetAnnotations());

    return state;
  }
  else
  {
    return {};
  }
}

static void Deserialize_vtkScalarsToColors(
  const nlohmann::json& state, vtkObjectBase* object, vtkDeserializer* deserializer)
{
  using nlohmann::json;
  if (auto* stc = vtkScalarsToColors::SafeDownCast(object))
  {
    if (auto superDeserializer = deserializer->GetHandler(typeid(vtkScalarsToColors::Superclass)))
    {
      superDeserializer(state, object, deserializer);
    }
    {
      const auto iter = state.find("Range");
      if ((iter != state.end()) && !iter->is_null())
      {
        auto values = iter->get<std::vector<double>>();
        stc->SetRange(values[0], values[1]);
      }
    }

    VTK_DESERIALIZE_VALUE_FROM_STATE(VectorMode, int, state, stc);
    VTK_DESERIALIZE_VALUE_FROM_STATE(VectorComponent, int, state, stc);
    VTK_DESERIALIZE_VALUE_FROM_STATE(VectorSize, int, state, stc);
    VTK_DESERIALIZE_VALUE_FROM_STATE(IndexedLookup, int, state, stc);
    VTK_DESERIALIZE_VALUE_FROM_STATE(Alpha, double, state, stc);

    if (!state["AnnotatedValues"].contains("Id") || !state["Annotations"].contains("Id"))
    {
      return;
    }

    vtkSmartPointer<vtkAbstractArray> av;
    vtkSmartPointer<vtkStringArray> annotations;
    const auto* context = deserializer->GetContext();
    {
      const auto identifier = state["AnnotatedValues"]["Id"].get<vtkTypeUInt32>();
      auto subObject = context->GetObjectAtId(identifier);
      deserializer->DeserializeJSON(identifier, subObject);
      av = vtkAbstractArray::SafeDownCast(subObject);
    }
    {
      const auto identifier = state["Annotations"]["Id"].get<vtkTypeUInt32>();
      auto subObject = context->GetObjectAtId(identifier);
      deserializer->DeserializeJSON(identifier, subObject);
      annotations = vtkStringArray::SafeDownCast(subObject);
    }
    if (av == nullptr)
    {
      vtkErrorWithObjectMacro(context, << deserializer->GetObjectDescription()
                                       << " gave AnnotatedValues=nullptr for "
                                       << stc->GetObjectDescription());
    }
    else if (annotations == nullptr)
    {
      vtkErrorWithObjectMacro(context, << deserializer->GetObjectDescription()
                                       << " gave Annotations=nullptr for "
                                       << stc->GetObjectDescription());
    }
    else
    {
      stc->SetAnnotations(av, annotations);
    }
  }
}

int RegisterHandlers_vtkScalarsToColorsSerDesHelper(
  void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkScalarsToColors), Serialize_vtkScalarsToColors);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkScalarsToColors), Deserialize_vtkScalarsToColors);
      deserializer->RegisterConstructor("vtkScalarsToColors", vtkScalarsToColors::New);
      success = 1;
    }
  }
  return success;
}
