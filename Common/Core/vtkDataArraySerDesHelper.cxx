// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkArrayDispatch.h"
#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDeserializer.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkInvoker.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkLookupTable.h"
#include "vtkSerializer.h"
#include "vtkSetGet.h"
#include "vtkShortArray.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeFloat64Array.h"
#include "vtkTypeInt16Array.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkTypeInt8Array.h"
#include "vtkTypeUInt16Array.h"
#include "vtkTypeUInt32Array.h"
#include "vtkTypeUInt64Array.h"
#include "vtkTypeUInt8Array.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedLongLongArray.h"
#include "vtkUnsignedShortArray.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkDataArray
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkDataArraySerDesHelper(void* ser, void* deser, void* invoker);
}

namespace
{
struct ArrayTypeInfo
{
  const std::string Name;
  std::function<vtkObjectBase*()> New;
  const std::type_info& TypeInfo;
};
// clang-format off
#define TYPE_INFO_MACRO(className) \
  { #className, className::New, typeid(className) }
std::vector<ArrayTypeInfo> ArrayTypes = {
  TYPE_INFO_MACRO(vtkBitArray),
  TYPE_INFO_MACRO(vtkCharArray),
  TYPE_INFO_MACRO(vtkDoubleArray),
  TYPE_INFO_MACRO(vtkFloatArray),
  TYPE_INFO_MACRO(vtkIdTypeArray),
  TYPE_INFO_MACRO(vtkIntArray),
  TYPE_INFO_MACRO(vtkLongArray),
  TYPE_INFO_MACRO(vtkLongLongArray),
  TYPE_INFO_MACRO(vtkShortArray),
  TYPE_INFO_MACRO(vtkSignedCharArray),
  TYPE_INFO_MACRO(vtkUnsignedCharArray),
  TYPE_INFO_MACRO(vtkUnsignedIntArray),
  TYPE_INFO_MACRO(vtkUnsignedLongArray),
  TYPE_INFO_MACRO(vtkUnsignedLongLongArray),
  TYPE_INFO_MACRO(vtkUnsignedShortArray),
  TYPE_INFO_MACRO(vtkTypeFloat32Array),
  TYPE_INFO_MACRO(vtkTypeFloat64Array),
  TYPE_INFO_MACRO(vtkTypeInt8Array),
  TYPE_INFO_MACRO(vtkTypeInt16Array),
  TYPE_INFO_MACRO(vtkTypeInt32Array),
  TYPE_INFO_MACRO(vtkTypeInt64Array),
  TYPE_INFO_MACRO(vtkTypeUInt8Array),
  TYPE_INFO_MACRO(vtkTypeUInt16Array),
  TYPE_INFO_MACRO(vtkTypeUInt32Array),
  TYPE_INFO_MACRO(vtkTypeUInt64Array)
};
// clang-format on

struct vtkDataArraySerializer
{
  template <typename ArrayT>
  void operator()(ArrayT* array, nlohmann::json& state, vtkSerializer* serializer)
  {
    if (array == nullptr)
    {
      return;
    }
    else if (!array->GetNumberOfValues())
    {
      return;
    }

    auto values = vtk::DataArrayValueRange(array);
    auto context = serializer->GetContext();
    auto blob = vtk::TakeSmartPointer(vtkTypeUInt8Array::New());
    vtkIdType arrSize = values.size() * array->GetDataTypeSize();
    if (array->IsA("vtkBitArray"))
    {
      arrSize = (values.size() + 7) / 8;
      state["NumberOfBits"] = values.size();
    }
    blob->SetArray(reinterpret_cast<vtkTypeUInt8*>(values.data()), arrSize, 1);

    std::string hash;
    if (context->RegisterBlob(blob, hash))
    {
      state["Hash"] = hash;
    }
    else
    {
      vtkErrorWithObjectMacro(context, << serializer->GetObjectDescription()
                                       << " failed to add blob " << blob->GetObjectDescription());
      return;
    }
    if (auto lt = array->GetLookupTable())
    {
      state["LookupTable"] = serializer->SerializeJSON(lt);
    }
  }
};

struct vtkDataArrayDeserializer
{
  template <typename ArrayT>
  void operator()(ArrayT* array, const nlohmann::json& state, vtkDeserializer* deserializer)
  {
    {
      auto* context = deserializer->GetContext();
      const auto& hash = state["Hash"].get<std::string>();
      const auto& blobs = context->Blobs();
      const auto blobIter = blobs.find(hash);
      if (blobIter == blobs.end())
      {
        vtkErrorWithObjectMacro(context,
          << deserializer->GetObjectDescription() << " failed to find blob for hash=" << hash);
        return;
      }
      if (!blobIter.value().is_binary())
      {
        vtkErrorWithObjectMacro(context,
          << deserializer->GetObjectDescription() << " failed to find blob for hash=" << hash);
        return;
      }
      const auto& content = blobIter.value().get_binary();
      auto dst = vtk::DataArrayValueRange(array);

      if (array->IsA("vtkBitArray"))
      {
        std::copy(content.data(), content.data() + ((dst.size() + 7) / 8),
          reinterpret_cast<unsigned char*>(dst.data()));
        array->SetNumberOfValues(state["NumberOfBits"]);
      }
      else
      {
        using APIType = vtk::GetAPIType<ArrayT>;
        const APIType* c_ptr = reinterpret_cast<const APIType*>(content.data());
        auto src = const_cast<APIType*>(c_ptr);
        std::copy(src, src + dst.size(), dst.data());
      }
      // nifty memory savings below, unfortunately, doesn't work correctly when there are point
      // scalars.
      // array->SetVoidArray(const_cast<void*>(c_ptr), array->GetNumberOfValues(), 1);
    }
    VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE(LookupTable, vtkLookupTable, state, array, deserializer);
  }
};
}

static nlohmann::json Serialize_vtkDataArray(vtkObjectBase* object, vtkSerializer* serializer)
{
  auto da = vtkDataArray::SafeDownCast(object);
  if (!da)
  {
    return {};
  }
  nlohmann::json state;
  if (auto superSerializer = serializer->GetHandler(typeid(vtkDataArray::Superclass)))
  {
    state = superSerializer(da, serializer);
  }
  vtkDataArraySerializer serializeWorker;
  using Dispatch = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;
  if (!Dispatch::Execute(da, serializeWorker, state, serializer))
  {
    serializeWorker(da, state, serializer);
  }
  auto& superClasses = state["SuperClassNames"];

  superClasses.push_back("vtkAbstractArray");
  superClasses.push_back("vtkDataArray");

  for (const auto& arrayType : ArrayTypes)
  {
    if (da->IsA(arrayType.Name.c_str()) && da->GetClassName() != arrayType.Name)
    {
      superClasses.push_back(arrayType.Name);
    }
  }
  return state;
}

static void Deserialize_vtkDataArray(
  const nlohmann::json& state, vtkObjectBase* object, vtkDeserializer* deserializer)
{
  if (object == nullptr)
  {
    return;
  }
  if (const auto superDeserializer = deserializer->GetHandler(typeid(vtkDataArray::Superclass)))
  {
    superDeserializer(state, object, deserializer);
  }
  auto* da = vtkDataArray::SafeDownCast(object);
  if (!da->GetNumberOfValues())
  {
    return;
  }
  vtkDataArrayDeserializer deserializeWorker;
  using Dispatch = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;
  if (!Dispatch::Execute(da, deserializeWorker, state, deserializer))
  {
    deserializeWorker(da, state, deserializer);
  }
}

int RegisterHandlers_vtkDataArraySerDesHelper(void* ser, void* deser, void* invoker)
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      for (auto& arrayType : ArrayTypes)
      {
        serializer->RegisterHandler(arrayType.TypeInfo, Serialize_vtkDataArray);
      }
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      for (auto& arrayType : ArrayTypes)
      {
        deserializer->RegisterConstructor(arrayType.Name, arrayType.New);
        deserializer->RegisterHandler(arrayType.TypeInfo, Deserialize_vtkDataArray);
      }
      success = 1;
    }
  }
  // copy invokers
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(invoker))
  {
    if (auto* invokerObject = vtkInvoker::SafeDownCast(asObjectBase))
    {
      for (auto& arrayType : ArrayTypes)
      {
        invokerObject->RegisterHandler(
          arrayType.TypeInfo, invokerObject->GetHandler(typeid(vtkDataArray)));
      }
    }
  }
  return success;
}
