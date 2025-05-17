// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDeserializer.h"
#include "vtkFieldData.h"
#include "vtkObjectBase.h"
#include "vtkSerializer.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkFieldData
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkFieldDataSerDesHelper(void* ser, void* deser, void* invoker);
}

VTK_ABI_NAMESPACE_BEGIN

class VTKCOMMONDATAMODEL_NO_EXPORT vtkFieldDataSerDesHelper
{
public:
  //----------------------------------------------------------------------------
  static nlohmann::json Serialize_vtkFieldData(vtkObjectBase* object, vtkSerializer* serializer)
  {
    using nlohmann::json;
    if (auto* fd = vtkFieldData::SafeDownCast(object))
    {
      json state;
      if (auto superSerializer = serializer->GetHandler(typeid(vtkFieldData::Superclass)))
      {
        state = superSerializer(object, serializer);
      }
      state["NumberOfArrays"] = fd->GetNumberOfArrays();
      auto& dst = state["Arrays"] = json::array();
      for (int i = 0; i < fd->GetNumberOfArrays(); ++i)
      {
        dst.push_back(serializer->SerializeJSON(fd->Data[i]));
      }
      state["GhostsToSkip"] = fd->GetGhostsToSkip();
      state["NumberOfTuples"] = fd->GetNumberOfTuples();
      return state;
    }
    else
    {
      return {};
    }
  }

  //----------------------------------------------------------------------------
  static void Deserialize_vtkFieldData(
    const nlohmann::json& state, vtkObjectBase* object, vtkDeserializer* deserializer)
  {
    using nlohmann::json;
    if (auto* fd = vtkFieldData::SafeDownCast(object))
    {
      if (auto superDeserializer = deserializer->GetHandler(typeid(vtkFieldData::Superclass)))
      {
        superDeserializer(state, object, deserializer);
      }
      auto* context = deserializer->GetContext();
      const auto& stateOfArrays = state["Arrays"];
      // vector used to keep existing arrays alive so that fd->RemoveArray doesn't destroy the
      // vtkAbstractArray object.
      std::vector<vtkSmartPointer<vtkAbstractArray>> arrays;
      for (auto& stateOfarray : stateOfArrays)
      {
        const auto identifier = stateOfarray["Id"].get<vtkTypeUInt32>();
        auto subObject = context->GetObjectAtId(identifier);
        deserializer->DeserializeJSON(identifier, subObject);
        if (auto* array = vtkAbstractArray::SafeDownCast(subObject))
        {
          arrays.emplace_back(array);
        }
      }
      // Now remove arrays from the collection.
      // If arrays already existed before entering this function, it does not invoke
      // destructor on the vtkAbstractArray because a reference is held by the vector of arrays.
      if (static_cast<std::size_t>(fd->GetNumberOfArrays()) != arrays.size())
      {
        while (fd->GetNumberOfArrays() > 0)
        {
          auto* array = fd->GetAbstractArray(0);
          context->UnRegisterObject(context->GetId(array));
          fd->RemoveArray(0);
        }
        for (const auto& array : arrays)
        {
          fd->AddArray(array);
        }
      }
      else
      {
        int i = 0;
        for (const auto& array : arrays)
        {
          // vtkFieldData::SetArray only marks the vtkFieldData as modified if the array is
          // different from the one already present in the vtkFieldData. This is important because
          // the vtkFieldData::MTime affects the MTime of a vtkPolyData object. We need to be very
          // careful here because unnecessary modification of the vtkFieldData::MTime will cause the
          // vtkPolyData to be marked as modified and, in turn, will force a mapper to upload the
          // data again.
          fd->SetArray(i, array);
          ++i;
        }
      }
      VTK_DESERIALIZE_VALUE_FROM_STATE(NumberOfTuples, int, state, fd);
      VTK_DESERIALIZE_VALUE_FROM_STATE(GhostsToSkip, int, state, fd);
    }
  }
};

VTK_ABI_NAMESPACE_END

int RegisterHandlers_vtkFieldDataSerDesHelper(void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(
        typeid(vtkFieldData), vtkFieldDataSerDesHelper::Serialize_vtkFieldData);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(
        typeid(vtkFieldData), vtkFieldDataSerDesHelper::Deserialize_vtkFieldData);
      deserializer->RegisterConstructor("vtkFieldData", vtkFieldData::New);
      success = 1;
    }
  }
  return success;
}
