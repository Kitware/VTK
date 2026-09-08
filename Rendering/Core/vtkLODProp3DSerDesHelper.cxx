// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDeserializer.h"
#include "vtkImageMapper3D.h"
#include "vtkLODProp3D.h"
#include "vtkMarshalContext.h"
#include "vtkProp3D.h"
#include "vtkSerializer.h"
#include "vtkSmartPointer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

#include <string>
#include <type_traits>

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkLODProp3D
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkLODProp3DSerDesHelper(void* ser, void* deser, void* invoker);
}

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGCORE_NO_EXPORT vtkLODProp3DSerDesHelper
{
  template <typename T,
    typename = std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>>>
  static bool DeserializeLODProp3DEntryMember(
    vtkDeserializer* deserializer, const nlohmann::json& source, const char* key, T& target)
  {
    if (const auto iter = source.find(key); iter != source.end() && !iter->is_null())
    {
      target = iter->get<T>();
      return true;
    }
    else
    {
      vtkErrorWithObjectMacro(deserializer, << "A LOD entry has no '" << key << "'.");
      return false;
    }
  }

  static bool DeserializeLODProp3DEntryMember(vtkDeserializer* deserializer,
    const nlohmann::json& source, const char* key, vtkProp3D*& target)
  {
    vtkSmartPointer<vtkObjectBase> result;
    if (const auto prop3DIter = source.find("Prop3D"); prop3DIter != source.end())
    {
      if (prop3DIter->is_null())
      {
        // this is fine. the LOD was removed and Prop3D was nulled.
        target = nullptr;
        return true;
      }
      if (const auto idIter = prop3DIter->find("Id");
          idIter != prop3DIter->end() && idIter->is_number_unsigned())
      {
        const auto identifier = idIter->get<vtkTypeUInt32>();
        result = deserializer->GetContext()->GetObjectAtId(identifier);
        if (!deserializer->DeserializeJSON(identifier, result))
        {
          vtkErrorWithObjectMacro(
            deserializer, << "Failed to deserialize '" << key << "' of a LOD");
          return false;
        }
        if (target = vtkProp3D::SafeDownCast(result); target != nullptr)
        {
          target->Register(nullptr); // bump refcnt on target since result is destroyed on return.
          return true;
        }
        else
        {
          vtkErrorWithObjectMacro(
            deserializer, << "Expected a 'vtkProp3D' for '" << key << "' of a LOD");
          return false;
        }
      }
      else
      {
        vtkErrorWithObjectMacro(deserializer, << "Missing 'Id' for '" << key << "' of a LOD");
        return false;
      }
    }
    else
    {
      vtkErrorWithObjectMacro(deserializer, << "A LOD entry has no '" << key << "'.");
      return false;
    }
  }

public:
  static nlohmann::json Serialize_vtkLODProp3D(vtkObjectBase* object, vtkSerializer* serializer)
  {
    using json = nlohmann::json;
    if (auto* lodProp3D = vtkLODProp3D::SafeDownCast(object))
    {
      json state;
      if (auto superSerializer = serializer->GetHandler(typeid(vtkLODProp3D::Superclass)))
      {
        state = superSerializer(object, serializer);
      }
      state["SuperClassNames"].push_back("vtkProp3D");
      state["AutomaticLODSelection"] = lodProp3D->GetAutomaticLODSelection();
      state["SelectedLODID"] = lodProp3D->GetSelectedLODID();
      state["SelectedPickLODID"] = lodProp3D->GetSelectedPickLODID();
      state["AutomaticPickLODSelection"] = lodProp3D->GetAutomaticPickLODSelection();
      state["CurrentIndex"] = lodProp3D->CurrentIndex;

      auto& dst = state["LODs"] = json::array();
      for (int i = 0; i < lodProp3D->NumberOfEntries; ++i)
      {
        const auto& lodEntry = lodProp3D->LODs[i];
        json entry;
        entry["Prop3D"] = serializer->SerializeJSON(lodEntry.Prop3D);
        entry["Prop3DType"] = lodEntry.Prop3DType;
        entry["ID"] = lodEntry.ID;
        entry["EstimatedTime"] = lodEntry.EstimatedTime;
        entry["State"] = lodEntry.State;
        entry["Level"] = lodEntry.Level;
        dst.emplace_back(std::move(entry));
      }
      return state;
    }
    else
    {
      vtkWarningWithObjectMacro(serializer, << "Object is not a vtkLODProp3D!");
      return {};
    }
  }

  static bool Deserialize_vtkLODProp3D(
    const nlohmann::json& state, vtkObjectBase* object, vtkDeserializer* deserializer)
  {
    bool success = true;
    auto* lodProp3D = vtkLODProp3D::SafeDownCast(object);
    if (!lodProp3D)
    {
      vtkErrorWithObjectMacro(deserializer, << __func__ << ": object not a vtkLODProp3D");
      return false;
    }
    if (auto f = deserializer->GetHandler(typeid(vtkLODProp3D::Superclass)))
    {
      success &= f(state, object, deserializer);
    }
    if (!success)
    {
      vtkErrorWithObjectMacro(deserializer, << "Superclass deserialization failed");
      return false;
    }
    VTK_DESERIALIZE_VALUE_FROM_STATE(AutomaticLODSelection, vtkTypeBool, state, lodProp3D);
    VTK_DESERIALIZE_VALUE_FROM_STATE(SelectedLODID, int, state, lodProp3D);
    VTK_DESERIALIZE_VALUE_FROM_STATE(SelectedPickLODID, int, state, lodProp3D);
    VTK_DESERIALIZE_VALUE_FROM_STATE(AutomaticPickLODSelection, vtkTypeBool, state, lodProp3D);

    if (auto iter = state.find("LODs"); iter != state.end() && !iter->is_null())
    {
      // Populate a fresh array before releasing the current one.
      const auto count = iter.value().size();
      auto* newLODs = new vtkLODProp3DEntry_t[count]();
      std::size_t index = 0;
      for (const auto& entry : iter.value())
      {
        success &=
          DeserializeLODProp3DEntryMember(deserializer, entry, "Prop3D", newLODs[index].Prop3D);
        success &= DeserializeLODProp3DEntryMember(
          deserializer, entry, "Prop3DType", newLODs[index].Prop3DType);
        success &= DeserializeLODProp3DEntryMember(deserializer, entry, "ID", newLODs[index].ID);
        success &= DeserializeLODProp3DEntryMember(
          deserializer, entry, "EstimatedTime", newLODs[index].EstimatedTime);
        success &=
          DeserializeLODProp3DEntryMember(deserializer, entry, "State", newLODs[index].State);
        success &=
          DeserializeLODProp3DEntryMember(deserializer, entry, "Level", newLODs[index].Level);
        ++index;
      }

      lodProp3D->ClearLODs(); // deletes lodProp3D->LODs
      lodProp3D->LODs = newLODs;
      lodProp3D->NumberOfEntries = static_cast<int>(count);
      if (const auto currentIndexIter = state.find("CurrentIndex");
          currentIndexIter != state.end() && !currentIndexIter->is_null())
      {
        lodProp3D->CurrentIndex = currentIndexIter->get<int>();
      }
      lodProp3D->RebuildLODBookkeeping();
    }
    return success;
  }
};

VTK_ABI_NAMESPACE_END

int RegisterHandlers_vtkLODProp3DSerDesHelper(void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(
        typeid(vtkLODProp3D), vtkLODProp3DSerDesHelper::Serialize_vtkLODProp3D);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(
        typeid(vtkLODProp3D), vtkLODProp3DSerDesHelper::Deserialize_vtkLODProp3D);
      deserializer->RegisterConstructor("vtkLODProp3D", vtkLODProp3D::New);
      success = 1;
    }
  }
  return success;
}
