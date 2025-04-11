// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCommand.h"
#include "vtkDeserializer.h"
#include "vtkInformation.h"
#include "vtkInformationDataObjectKey.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIdTypeKey.h"
#include "vtkInformationInformationKey.h"
#include "vtkInformationInformationVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerPointerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationIterator.h"
#include "vtkInformationKey.h"
#include "vtkInformationKeyLookup.h"
#include "vtkInformationKeyVectorKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationObjectBaseVectorKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationUnsignedLongKey.h"
#include "vtkInformationVariantKey.h"
#include "vtkInformationVariantVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectBase.h"
#include "vtkSerializer.h"
#include "vtkVariantSerDesHelper.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include <string>
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkInformation
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkInformationSerDesHelper(void* ser, void* deser, void* invoker);
}

static nlohmann::json Serialize_vtkInformation(vtkObjectBase* object, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto* information = vtkInformation::SafeDownCast(object);
  if (auto f = serializer->GetHandler(typeid(vtkInformation::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkObject");
  vtkNew<vtkInformationIterator> iter;
  iter->SetInformation(information);
  iter->InitTraversal();
  auto& dst = state["Keys"] = json::array();
  while (!iter->IsDoneWithTraversal())
  {
    json keyState;
    vtkInformationKey* key = iter->GetCurrentKey();

    keyState["Name"] = key->GetName();
    keyState["Location"] = key->GetLocation();
    // keyState["ClassName"] = key->GetClassName();

    if (key->IsA("vtkInformationStringKey"))
    {
      keyState["Value"] = information->Get(static_cast<vtkInformationStringKey*>(key));
    }
    else if (key->IsA("vtkInformationIntegerKey"))
    {
      keyState["Value"] = information->Get(static_cast<vtkInformationIntegerKey*>(key));
    }
    else if (key->IsA("vtkInformationDoubleKey"))
    {
      keyState["Value"] = information->Get(static_cast<vtkInformationDoubleKey*>(key));
    }
    else if (key->IsA("vtkInformationIdTypeKey"))
    {
      keyState["Value"] = information->Get(static_cast<vtkInformationIdTypeKey*>(key));
    }
    else if (key->IsA("vtkInformationUnsignedLongKey"))
    {
      keyState["Value"] = information->Get(static_cast<vtkInformationUnsignedLongKey*>(key));
    }
    else if (key->IsA("vtkInformationVariantKey"))
    {
      const vtkVariant* variant = &information->Get(static_cast<vtkInformationVariantKey*>(key));
      keyState["Value"] = Serialize_vtkVariant(variant, serializer);
    }
    else if (key->IsA("vtkInformationIntegerVectorKey"))
    {
      auto& valArray = keyState["Value"] = json::array();
      int* ptr = information->Get(static_cast<vtkInformationIntegerVectorKey*>(key));
      int length = information->Length(static_cast<vtkInformationIntegerVectorKey*>(key));
      for (int i = 0; i < length; ++i)
      {
        valArray.push_back(ptr[i]);
      }
    }
    else if (key->IsA("vtkInformationStringVectorKey"))
    {
      auto& valArray = keyState["Value"] = json::array();
      int length = information->Length(static_cast<vtkInformationStringVectorKey*>(key));
      for (int i = 0; i < length; ++i)
      {
        valArray.push_back(information->Get(static_cast<vtkInformationStringVectorKey*>(key), i));
      }
    }
    else if (key->IsA("vtkInformationIntegerPointerKey"))
    {
      auto& valArray = keyState["Value"] = json::array();
      int* ptr = information->Get(static_cast<vtkInformationIntegerPointerKey*>(key));
      int length = information->Length(static_cast<vtkInformationIntegerPointerKey*>(key));
      for (int i = 0; i < length; ++i)
      {
        valArray.push_back(ptr[i]);
      }
    }
    else if (key->IsA("vtkInformationDoubleVectorKey"))
    {
      auto& valArray = keyState["Value"] = json::array();
      double* ptr = information->Get(static_cast<vtkInformationDoubleVectorKey*>(key));
      int length = information->Length(static_cast<vtkInformationDoubleVectorKey*>(key));
      for (int i = 0; i < length; ++i)
      {
        valArray.push_back(ptr[i]);
      }
    }
    else if (key->IsA("vtkInformationVariantVectorKey"))
    {
      auto& valArray = keyState["Value"] = json::array();
      const vtkVariant* ptr = information->Get(static_cast<vtkInformationVariantVectorKey*>(key));
      int length = information->Length(static_cast<vtkInformationVariantVectorKey*>(key));
      for (int i = 0; i < length; ++i)
      {
        valArray.push_back(Serialize_vtkVariant(&ptr[i], serializer));
      }
    }
    else if (key->IsA("vtkInformationKeyVectorKey"))
    {
      auto& valArray = keyState["Value"] = json::array();
      vtkInformationKey** ptr = information->Get(static_cast<vtkInformationKeyVectorKey*>(key));
      int length = information->Length(static_cast<vtkInformationKeyVectorKey*>(key));
      for (int i = 0; i < length; ++i)
      {
        json subkey;
        subkey["Name"] = ptr[i]->GetName();
        subkey["Location"] = ptr[i]->GetLocation();
        valArray.push_back(subkey);
      }
    }
    else if (key->IsA("vtkInformationInformationKey"))
    {
      keyState["Value"] = serializer->SerializeJSON(
        information->Get(static_cast<vtkInformationInformationKey*>(key)));
    }
    else if (key->IsA("vtkInformationInformationVectorKey"))
    {
      auto& valArray = keyState["Value"] = json::array();
      vtkSmartPointer<vtkInformationVector> infoVector =
        information->Get(static_cast<vtkInformationInformationVectorKey*>(key));
      int length = infoVector->GetNumberOfInformationObjects();
      for (int i = 0; i < length; ++i)
      {
        valArray.push_back(serializer->SerializeJSON(infoVector->GetInformationObject(i)));
      }
    }
    else if (key->IsA("vtkInformationObjectBaseKey"))
    {
      keyState["Value"] =
        serializer->SerializeJSON(information->Get(static_cast<vtkInformationObjectBaseKey*>(key)));
    }
    else if (key->IsA("vtkInformationObjectBaseVectorKey"))
    {
      auto& valArray = keyState["Value"] = json::array();
      int length = information->Length(static_cast<vtkInformationObjectBaseVectorKey*>(key));
      for (int i = 0; i < length; ++i)
      {
        valArray.push_back(serializer->SerializeJSON(
          information->Get(static_cast<vtkInformationObjectBaseVectorKey*>(key), i)));
      }
    }
    else if (key->IsA("vtkInformationDataObjectKey"))
    {
      keyState["Value"] = serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(
        information->Get(static_cast<vtkInformationDataObjectKey*>(key))));
    }
    dst.push_back(keyState);
    iter->GoToNextItem();
  }

  return state;
}

static void Deserialize_vtkInformation(
  const nlohmann::json& state, vtkObjectBase* object, vtkDeserializer* deserializer)
{
  auto* information = vtkInformation::SafeDownCast(object);
  if (auto f = deserializer->GetHandler(typeid(vtkInformation::Superclass)))
  {
    f(state, object, deserializer);
  }
  const auto* context = deserializer->GetContext();

  for (const auto& keyState : state["Keys"])
  {
    vtkInformationKey* key = vtkInformationKeyLookup::Find(keyState["Name"], keyState["Location"]);
    // std::string className = keyState["ClassName"];
    if (key->IsA("vtkInformationStringKey"))
    {
      information->Set(static_cast<vtkInformationStringKey*>(key), keyState["Value"]);
    }
    else if (key->IsA("vtkInformationIntegerKey"))
    {
      information->Set(static_cast<vtkInformationIntegerKey*>(key), keyState["Value"]);
    }
    else if (key->IsA("vtkInformationDoubleKey"))
    {
      information->Set(static_cast<vtkInformationDoubleKey*>(key), keyState["Value"]);
    }
    else if (key->IsA("vtkInformationIdTypeKey"))
    {
      information->Set(static_cast<vtkInformationIdTypeKey*>(key), keyState["Value"]);
    }
    else if (key->IsA("vtkInformationUnsignedLongKey"))
    {
      information->Set(static_cast<vtkInformationUnsignedLongKey*>(key), keyState["Value"]);
    }
    else if (key->IsA("vtkInformationVariantKey"))
    {
      vtkVariant variant;
      Deserialize_vtkVariant(keyState["Value"], &variant, deserializer);
      information->Set(static_cast<vtkInformationVariantKey*>(key), variant);
    }
    else if (key->IsA("vtkInformationIntegerVectorKey"))
    {
      std::vector<int> vec = keyState["Value"];
      information->Set(static_cast<vtkInformationIntegerVectorKey*>(key), vec.data(),
        static_cast<int>(vec.size()));
    }
    else if (key->IsA("vtkInformationStringVectorKey"))
    {
      for (const std::string item : keyState["Value"])
      {
        information->Append(static_cast<vtkInformationStringVectorKey*>(key), item);
      }
    }
    else if (key->IsA("vtkInformationIntegerPointerKey"))
    {
      int size = static_cast<int>(keyState["Value"].size());
      vtkNew<vtkIntArray> arr;

      int* ptr = new int[size];
      for (int i = 0; i < size; i++)
      {
        ptr[i] = keyState["Value"][i];
      }
      arr->SetArray(ptr, size, 0);

      // Add observer to delete arr/free ptr when information is deleted
      arr->Register(information);
      information->AddObserver(vtkCommand::DeleteEvent, arr.GetPointer(), &vtkIntArray::Delete);
      information->Set(static_cast<vtkInformationIntegerPointerKey*>(key), ptr, size);
    }
    else if (key->IsA("vtkInformationDoubleVectorKey"))
    {
      std::vector<double> vec = keyState["Value"];
      information->Set(
        static_cast<vtkInformationDoubleVectorKey*>(key), vec.data(), static_cast<int>(vec.size()));
    }
    else if (key->IsA("vtkInformationVariantVectorKey"))
    {
      for (const auto& item : keyState["Value"])
      {
        vtkVariant variant;
        Deserialize_vtkVariant(item, &variant, deserializer);
        information->Append(static_cast<vtkInformationVariantVectorKey*>(key), variant);
      }
    }
    else if (key->IsA("vtkInformationKeyVectorKey"))
    {
      for (const auto& item : keyState["Value"])
      {
        auto obj = context->GetObjectAtId(item["Id"]);
        vtkInformationKey* subkey = vtkInformationKeyLookup::Find(item["Name"], item["Location"]);
        information->Append(static_cast<vtkInformationKeyVectorKey*>(subkey), subkey);
      }
    }
    else if (key->IsA("vtkInformationInformationKey"))
    {
      auto obj = context->GetObjectAtId(keyState["Value"]["Id"]);
      deserializer->DeserializeJSON(keyState["Value"]["Id"], obj);
      information->Set(
        static_cast<vtkInformationInformationKey*>(key), vtkInformation::SafeDownCast(obj));
    }
    else if (key->IsA("vtkInformationInformationVectorKey"))
    {
      vtkSmartPointer<vtkInformationVector> infoVec = vtkInformationVector::New();
      information->Set(static_cast<vtkInformationInformationVectorKey*>(key), infoVec);
      infoVec->UnRegister(nullptr);
      for (const auto& item : keyState["Value"])
      {
        auto obj = context->GetObjectAtId(item["Id"]);
        deserializer->DeserializeJSON(item["Id"], obj);
        information->Get(static_cast<vtkInformationInformationVectorKey*>(key))
          ->Append(vtkInformation::SafeDownCast(obj));
      }
    }
    else if (key->IsA("vtkInformationObjectBaseKey"))
    {
      vtkSmartPointer<vtkObjectBase> obj;
      deserializer->DeserializeJSON(keyState["Value"]["Id"], obj);
      information->Set(static_cast<vtkInformationObjectBaseKey*>(key), obj);
    }
    else if (key->IsA("vtkInformationObjectBaseVectorKey"))
    {
      for (const auto& item : keyState["Value"])
      {
        vtkSmartPointer<vtkObjectBase> obj;
        deserializer->DeserializeJSON(item["Id"], obj);
        information->Append(static_cast<vtkInformationObjectBaseVectorKey*>(key), obj);
      }
    }
    else if (key->IsA("vtkInformationDataObjectKey"))
    {
      auto obj = context->GetObjectAtId(keyState["Value"]["Id"]);

      deserializer->DeserializeJSON(keyState["Value"]["Id"], obj);
      information->Set(static_cast<vtkInformationDataObjectKey*>(key),
        reinterpret_cast<vtkDataObject*>(obj.GetPointer()));
    }
  }
}

int RegisterHandlers_vtkInformationSerDesHelper(void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkInformation), Serialize_vtkInformation);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkInformation), Deserialize_vtkInformation);
      deserializer->RegisterConstructor("vtkInformation", vtkInformation::New);
      success = 1;
    }
  }
  return success;
}
