// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkDataObject.h"
#include "vtkDeserializer.h"
#include "vtkScalarsToColors.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkCompositeDataDisplayAttributes
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkCompositeDataDisplayAttributesSerDesHelper(
    void* ser, void* deser, void* invoker);
}

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGCORE_NO_EXPORT vtkCompositeDataDisplayAttributesSerDesHelper
{
public:
#define SERIALIZE_MAP_SIMPLE(name)                                                                 \
  do                                                                                               \
  {                                                                                                \
    auto& dst = state[#name] = json::array();                                                      \
    for (auto& iter : object->Block##name)                                                         \
    {                                                                                              \
      dst.push_back(                                                                               \
        { { "Key", serializer->SerializeJSON(iter.first) }, { "Value", iter.second } });           \
    }                                                                                              \
  } while (0)

#define SERIALIZE_MAP_OF_VTK_COLOR3D(name)                                                         \
  do                                                                                               \
  {                                                                                                \
    auto& dst = state[#name] = json::array();                                                      \
    for (auto& iter : object->Block##name)                                                         \
    {                                                                                              \
      dst.push_back({ { "Key", serializer->SerializeJSON(iter.first) },                            \
        { "Values", { iter.second.GetRed(), iter.second.GetGreen(), iter.second.GetBlue() } } });  \
    }                                                                                              \
  } while (0)

#define SERIALIZE_MAP_OF_VTK_VECTOR2D(name)                                                        \
  do                                                                                               \
  {                                                                                                \
    auto& dst = state[#name] = json::array();                                                      \
    for (auto& iter : object->Block##name)                                                         \
    {                                                                                              \
      dst.push_back({ { "Key", serializer->SerializeJSON(iter.first) },                            \
        { "Values", { iter.second[0], iter.second[1] } } });                                       \
    }                                                                                              \
  } while (0)

#define SERIALIZE_MAP_OF_VTK_OBJECTS(name)                                                         \
  do                                                                                               \
  {                                                                                                \
    auto& dst = state[#name] = json::array();                                                      \
    for (auto& iter : object->Block##name)                                                         \
    {                                                                                              \
      dst.push_back({ { "Key", serializer->SerializeJSON(iter.first) },                            \
        { "Value", serializer->SerializeJSON(iter.second) } });                                    \
    }                                                                                              \
  } while (0)

  //----------------------------------------------------------------------------
  static nlohmann::json Serialize_vtkCompositeDataDisplayAttributes(
    vtkObjectBase* objectBase, vtkSerializer* serializer)
  {
    using json = nlohmann::json;
    json fullState;
    auto object = vtkCompositeDataDisplayAttributes::SafeDownCast(objectBase);
    if (auto f = serializer->GetHandler(typeid(vtkCompositeDataDisplayAttributes::Superclass)))
    {
      fullState = f(object, serializer);
    }
    fullState["SuperClassNames"].push_back("vtkObject");
    json state;
    SERIALIZE_MAP_SIMPLE(Visibilities);
    SERIALIZE_MAP_OF_VTK_COLOR3D(Colors);
    SERIALIZE_MAP_SIMPLE(Opacities);
    SERIALIZE_MAP_SIMPLE(Materials);
    SERIALIZE_MAP_SIMPLE(Pickabilities);
    SERIALIZE_MAP_SIMPLE(ScalarVisibilities);
    SERIALIZE_MAP_SIMPLE(UseLookupTableScalarRanges);
    SERIALIZE_MAP_SIMPLE(InterpolateScalarsBeforeMappings);
    SERIALIZE_MAP_SIMPLE(ColorModes);
    SERIALIZE_MAP_SIMPLE(ScalarModes);
    SERIALIZE_MAP_SIMPLE(ArrayAccessModes);
    SERIALIZE_MAP_SIMPLE(ArrayComponents);
    SERIALIZE_MAP_SIMPLE(ArrayIds);
    SERIALIZE_MAP_OF_VTK_VECTOR2D(ScalarRanges);
    SERIALIZE_MAP_SIMPLE(ArrayNames);
    SERIALIZE_MAP_OF_VTK_OBJECTS(LookupTables);
    SERIALIZE_MAP_SIMPLE(FieldDataTupleIds);
    fullState.insert(state.begin(), state.end());
    return fullState;
  }

#define DESERIALIZE_MAP_SIMPLE(name, type)                                                         \
  do                                                                                               \
  {                                                                                                \
    const auto propertyIter = state.find(#name);                                                   \
    if ((propertyIter != state.end()) && propertyIter->is_array())                                 \
    {                                                                                              \
      const auto items = propertyIter->get<nlohmann::json::array_t>();                             \
      for (auto& item : items)                                                                     \
      {                                                                                            \
        const auto* context = deserializer->GetContext();                                          \
        const auto keyIdentifier = item["Key"].at("Id").get<vtkTypeUInt32>();                      \
        auto subObject = context->GetObjectAtId(keyIdentifier);                                    \
        deserializer->DeserializeJSON(keyIdentifier, subObject);                                   \
        if (auto* dataObject = vtkDataObject::SafeDownCast(subObject))                             \
        {                                                                                          \
          auto value = item["Value"].get<type>();                                                  \
          object->Block##name[dataObject] = value;                                                 \
        }                                                                                          \
      }                                                                                            \
    }                                                                                              \
  } while (0)

#define DESERIALIZE_MAP_OF_VTK_COLOR3D(name)                                                       \
  do                                                                                               \
  {                                                                                                \
    const auto propertyIter = state.find(#name);                                                   \
    if ((propertyIter != state.end()) && propertyIter->is_array())                                 \
    {                                                                                              \
      const auto items = propertyIter->get<nlohmann::json::array_t>();                             \
      for (auto& item : items)                                                                     \
      {                                                                                            \
        const auto* context = deserializer->GetContext();                                          \
        const auto keyIdentifier = item["Key"].at("Id").get<vtkTypeUInt32>();                      \
        auto keyObject = context->GetObjectAtId(keyIdentifier);                                    \
        deserializer->DeserializeJSON(keyIdentifier, keyObject);                                   \
        if (auto* dataObject = vtkDataObject::SafeDownCast(keyObject))                             \
        {                                                                                          \
          auto values = item["Values"].get<json::array_t>();                                       \
          object->Block##name[dataObject] = vtkColor3d(values[0], values[1], values[2]);           \
        }                                                                                          \
      }                                                                                            \
    }                                                                                              \
  } while (0)

#define DESERIALIZE_MAP_OF_VTK_VECTOR2D(name)                                                      \
  do                                                                                               \
  {                                                                                                \
    const auto propertyIter = state.find(#name);                                                   \
    if ((propertyIter != state.end()) && propertyIter->is_array())                                 \
    {                                                                                              \
      const auto items = propertyIter->get<nlohmann::json::array_t>();                             \
      for (auto& item : items)                                                                     \
      {                                                                                            \
        const auto* context = deserializer->GetContext();                                          \
        const auto keyIdentifier = item["Key"].at("Id").get<vtkTypeUInt32>();                      \
        auto keyObject = context->GetObjectAtId(keyIdentifier);                                    \
        deserializer->DeserializeJSON(keyIdentifier, keyObject);                                   \
        if (auto* dataObject = vtkDataObject::SafeDownCast(keyObject))                             \
        {                                                                                          \
          auto values = item["Values"].get<json::array_t>();                                       \
          object->Block##name[dataObject] = vtkVector2d(values[0], values[1]);                     \
        }                                                                                          \
      }                                                                                            \
    }                                                                                              \
  } while (0)

#define DESERIALIZE_MAP_OF_VTK_OBJECTS(name, type)                                                 \
  do                                                                                               \
  {                                                                                                \
    const auto propertyIter = state.find(#name);                                                   \
    if ((propertyIter != state.end()) && propertyIter->is_array())                                 \
    {                                                                                              \
      const auto items = propertyIter->get<nlohmann::json::array_t>();                             \
      for (auto& item : items)                                                                     \
      {                                                                                            \
        const auto* context = deserializer->GetContext();                                          \
        const auto keyIdentifier = item["Key"].at("Id").get<vtkTypeUInt32>();                      \
        auto keyObject = context->GetObjectAtId(keyIdentifier);                                    \
        deserializer->DeserializeJSON(keyIdentifier, keyObject);                                   \
        if (auto* dataObject = vtkDataObject::SafeDownCast(keyObject))                             \
        {                                                                                          \
          const auto valueIdentifier = item["Value"].at("Id").get<vtkTypeUInt32>();                \
          auto valueObject = context->GetObjectAtId(valueIdentifier);                              \
          deserializer->DeserializeJSON(valueIdentifier, valueObject);                             \
          object->Block##name[dataObject] = type::SafeDownCast(valueObject);                       \
        }                                                                                          \
      }                                                                                            \
    }                                                                                              \
  } while (0)

  //----------------------------------------------------------------------------
  static void Deserialize_vtkCompositeDataDisplayAttributes(
    const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
  {
    auto object = vtkCompositeDataDisplayAttributes::SafeDownCast(objectBase);
    using json = nlohmann::json;
    DESERIALIZE_MAP_SIMPLE(Visibilities, bool);
    DESERIALIZE_MAP_OF_VTK_COLOR3D(Colors);
    DESERIALIZE_MAP_SIMPLE(Opacities, double);
    DESERIALIZE_MAP_SIMPLE(Materials, std::string);
    DESERIALIZE_MAP_SIMPLE(Pickabilities, bool);
    DESERIALIZE_MAP_SIMPLE(ScalarVisibilities, bool);
    DESERIALIZE_MAP_SIMPLE(UseLookupTableScalarRanges, bool);
    DESERIALIZE_MAP_SIMPLE(InterpolateScalarsBeforeMappings, bool);
    DESERIALIZE_MAP_SIMPLE(ColorModes, int);
    DESERIALIZE_MAP_SIMPLE(ScalarModes, int);
    DESERIALIZE_MAP_SIMPLE(ArrayAccessModes, int);
    DESERIALIZE_MAP_SIMPLE(ArrayComponents, int);
    DESERIALIZE_MAP_SIMPLE(ArrayIds, int);
    DESERIALIZE_MAP_OF_VTK_VECTOR2D(ScalarRanges);
    DESERIALIZE_MAP_SIMPLE(ArrayNames, std::string);
    DESERIALIZE_MAP_OF_VTK_OBJECTS(LookupTables, vtkScalarsToColors);
    DESERIALIZE_MAP_SIMPLE(FieldDataTupleIds, vtkIdType);
  }
};

VTK_ABI_NAMESPACE_END

int RegisterHandlers_vtkCompositeDataDisplayAttributesSerDesHelper(
  void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkCompositeDataDisplayAttributes),
        vtkCompositeDataDisplayAttributesSerDesHelper::Serialize_vtkCompositeDataDisplayAttributes);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkCompositeDataDisplayAttributes),
        vtkCompositeDataDisplayAttributesSerDesHelper::
          Deserialize_vtkCompositeDataDisplayAttributes);
      deserializer->RegisterConstructor("vtkCompositeDataDisplayAttributes",
        []() { return vtkCompositeDataDisplayAttributes::New(); });
      success = 1;
    }
  }
  return success;
}
