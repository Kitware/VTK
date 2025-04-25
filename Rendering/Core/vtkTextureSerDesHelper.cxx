// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDeserializer.h"
#include "vtkScalarsToColors.h"
#include "vtkSerializer.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"
#include "vtkTransform.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkTexture
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkTextureSerDesHelper(void* ser, void* deser, void* invoker);
}

static nlohmann::json Serialize_vtkTexture(vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto object = vtkTexture::SafeDownCast(objectBase);
  if (auto f = serializer->GetHandler(typeid(vtkTexture::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkImageAlgorithm");
  state["Interpolate"] = object->GetInterpolate();
  state["Mipmap"] = object->GetMipmap();
  state["MaximumAnisotropicFiltering"] = object->GetMaximumAnisotropicFiltering();
  state["Quality"] = object->GetQuality();
  state["ColorMode"] = object->GetColorMode();
  if (object->GetLookupTable())
  {
    state["LookupTable"] =
      serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(object->GetLookupTable()));
  }
  if (object->GetTransform())
  {
    state["Transform"] =
      serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(object->GetTransform()));
  }
  state["BlendingMode"] = object->GetBlendingMode();
  state["PremultipliedAlpha"] = object->GetPremultipliedAlpha();
  state["RestrictPowerOf2ImageSmaller"] = object->GetRestrictPowerOf2ImageSmaller();

  state["CubeMap"] = object->GetCubeMap();
  state["UseSRGBColorSpace"] = object->GetUseSRGBColorSpace();
  if (auto ptr = object->GetBorderColor())
  {
    auto& dst = state["BorderColor"] = json::array();
    for (int i = 0; i < 4; ++i)
    {
      dst.push_back(ptr[i]);
    }
  }
  state["Wrap"] = object->GetWrap();
  return state;
}

static void Deserialize_vtkTexture(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  auto object = vtkTexture::SafeDownCast(objectBase);
  // Cubemap property changes the number of input ports.
  // deserialize it before Deserialize_vtkAlgorithm gets to see the state.
  VTK_DESERIALIZE_VALUE_FROM_STATE(CubeMap, bool, state, object);
  if (auto f = deserializer->GetHandler(typeid(vtkTexture::Superclass)))
  {
    f(state, object, deserializer);
  }
  VTK_DESERIALIZE_VALUE_FROM_STATE(Interpolate, int, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(Mipmap, bool, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(MaximumAnisotropicFiltering, float, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(Quality, int, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(ColorMode, int, state, object);
  VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE(
    LookupTable, vtkScalarsToColors, state, object, deserializer);
  VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE(Transform, vtkTransform, state, object, deserializer);
  VTK_DESERIALIZE_VALUE_FROM_STATE(BlendingMode, int, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(PremultipliedAlpha, bool, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(RestrictPowerOf2ImageSmaller, int, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(UseSRGBColorSpace, bool, state, object);
  VTK_DESERIALIZE_VECTOR_FROM_STATE(BorderColor, float, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(Wrap, int, state, object);
}

int RegisterHandlers_vtkTextureSerDesHelper(void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkTexture), Serialize_vtkTexture);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkTexture), Deserialize_vtkTexture);
      deserializer->RegisterConstructor("vtkTexture", []() { return vtkTexture::New(); });
      success = 1;
    }
  }
  return success;
}
