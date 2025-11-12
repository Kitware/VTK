// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAffineArray.h"
#include "vtkAffineTypeFloat32Array.h"
#include "vtkAffineTypeFloat64Array.h"
#include "vtkAffineTypeInt16Array.h"
#include "vtkAffineTypeInt32Array.h"
#include "vtkAffineTypeInt64Array.h"
#include "vtkAffineTypeInt8Array.h"
#include "vtkAffineTypeUInt16Array.h"
#include "vtkAffineTypeUInt32Array.h"
#include "vtkAffineTypeUInt64Array.h"
#include "vtkAffineTypeUInt8Array.h"
#include "vtkArrayDispatch.h"
#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkConstantArray.h"
#include "vtkConstantTypeFloat32Array.h"
#include "vtkConstantTypeFloat64Array.h"
#include "vtkConstantTypeInt16Array.h"
#include "vtkConstantTypeInt32Array.h"
#include "vtkConstantTypeInt64Array.h"
#include "vtkConstantTypeInt8Array.h"
#include "vtkConstantTypeUInt16Array.h"
#include "vtkConstantTypeUInt32Array.h"
#include "vtkConstantTypeUInt64Array.h"
#include "vtkConstantTypeUInt8Array.h"
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
   * Register the (de)serialization handlers of vtkDataArray subclasses
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
#define TYPE_INFO_MACRO(className)                                                                 \
  {                                                                                                \
    #className, className::New, typeid(className)                                                  \
  }
#define TTYPE_INFO_MACRO(className)                                                                \
  {                                                                                                \
    typeid(className).name(), className::New, typeid(className)                                    \
  }

// clang-format off
#define TEMPLATED_ARRAY_TYPES_INFO_MACRO(className)                                                \
    TTYPE_INFO_MACRO(className<char>),                                                             \
    TTYPE_INFO_MACRO(className<double>),                                                           \
    TTYPE_INFO_MACRO(className<float>),                                                            \
    TTYPE_INFO_MACRO(className<int>),                                                              \
    TTYPE_INFO_MACRO(className<long>),                                                             \
    TTYPE_INFO_MACRO(className<long long>),                                                        \
    TTYPE_INFO_MACRO(className<short>),                                                            \
    TTYPE_INFO_MACRO(className<signed char>),                                                      \
    TTYPE_INFO_MACRO(className<unsigned char>),                                                    \
    TTYPE_INFO_MACRO(className<unsigned int>),                                                     \
    TTYPE_INFO_MACRO(className<unsigned long>),                                                    \
    TTYPE_INFO_MACRO(className<unsigned long long>),                                               \
    TTYPE_INFO_MACRO(className<unsigned short>)

#define CONCRETE_ARRAY_TYPES_INFO_MACRO(type)                                                      \
    TYPE_INFO_MACRO(type##TypeInt8Array),                                                          \
    TYPE_INFO_MACRO(type##TypeInt16Array),                                                         \
    TYPE_INFO_MACRO(type##TypeInt32Array),                                                         \
    TYPE_INFO_MACRO(type##TypeInt64Array),                                                         \
    TYPE_INFO_MACRO(type##TypeUInt8Array),                                                         \
    TYPE_INFO_MACRO(type##TypeUInt16Array),                                                        \
    TYPE_INFO_MACRO(type##TypeUInt32Array),                                                        \
    TYPE_INFO_MACRO(type##TypeUInt64Array),                                                        \
    TYPE_INFO_MACRO(type##TypeFloat32Array),                                                       \
    TYPE_INFO_MACRO(type##TypeFloat64Array)

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
  TEMPLATED_ARRAY_TYPES_INFO_MACRO(vtkAOSDataArrayTemplate),
  CONCRETE_ARRAY_TYPES_INFO_MACRO(vtk),
  TEMPLATED_ARRAY_TYPES_INFO_MACRO(vtkAffineArray),
  CONCRETE_ARRAY_TYPES_INFO_MACRO(vtkAffine),
  TEMPLATED_ARRAY_TYPES_INFO_MACRO(vtkConstantArray),
  CONCRETE_ARRAY_TYPES_INFO_MACRO(vtkConstant)

};

typedef vtkTypeList::Create<
  vtkAffineArray<char>,
  vtkAffineArray<double>,
  vtkAffineArray<float>,
  vtkAffineArray<int>,
  vtkAffineArray<long>,
  vtkAffineArray<long long>,
  vtkAffineArray<short>,
  vtkAffineArray<signed char>,
  vtkAffineArray<unsigned char>,
  vtkAffineArray<unsigned int>,
  vtkAffineArray<unsigned long>,
  vtkAffineArray<unsigned long long>,
  vtkAffineArray<unsigned short>>
  AffineArrays;

typedef vtkTypeList::Create<
  vtkConstantArray<char>,
  vtkConstantArray<double>,
  vtkConstantArray<float>,
  vtkConstantArray<int>,
  vtkConstantArray<long>,
  vtkConstantArray<long long>,
  vtkConstantArray<short>,
  vtkConstantArray<signed char>,
  vtkConstantArray<unsigned char>,
  vtkConstantArray<unsigned int>,
  vtkConstantArray<unsigned long>,
  vtkConstantArray<unsigned long long>,
  vtkConstantArray<unsigned short>>
  ConstantArrays;

typedef vtkTypeList::Append<vtkArrayDispatch::Arrays,
  vtkBitArray,
  AffineArrays,
  ConstantArrays>::Result
  DispatchTypeList;

// clang-format on

void Serialize_Blob(vtkTypeUInt8Array* blob, nlohmann::json& state, vtkSerializer* serializer)
{
  auto context = serializer->GetContext();
  std::string hash;
  if (context->RegisterBlob(blob, hash))
  {
    state["Hash"] = hash;
  }
  else
  {
    vtkErrorWithObjectMacro(context, << serializer->GetObjectDescription() << " failed to add blob "
                                     << blob->GetObjectDescription());
    return;
  }
}

bool Deserialize_Blob(
  nlohmann::json& blob, const nlohmann::json& state, vtkDeserializer* deserializer)
{
  auto* context = deserializer->GetContext();
  const auto& hash = state["Hash"].get<std::string>();
  const auto& blobs = context->Blobs();
  const auto blobIter = blobs.find(hash);
  if (blobIter == blobs.end())
  {
    vtkErrorWithObjectMacro(
      context, << deserializer->GetObjectDescription() << " failed to find blob for hash=" << hash);
    return false;
  }
  if (!blobIter.value().is_binary())
  {
    vtkErrorWithObjectMacro(
      context, << deserializer->GetObjectDescription() << " failed to find blob for hash=" << hash);
    return false;
  }
  blob = blobIter.value();
  return true;
}

struct vtkDataArraySerializer
{
  template <typename ValueT>
  void operator()(
    vtkAffineArray<ValueT>* array, nlohmann::json& state, vtkSerializer* vtkNotUsed(serializer))
  {
    if (array == nullptr)
    {
      return;
    }

    auto backend = array->GetBackend();
    state["Slope"] = backend->Slope;
    state["Intercept"] = backend->Intercept;
  }

  template <typename ValueT>
  void operator()(
    vtkConstantArray<ValueT>* array, nlohmann::json& state, vtkSerializer* vtkNotUsed(serializer))
  {
    if (array == nullptr)
    {
      return;
    }

    auto backend = array->GetBackend();
    state["Value"] = backend->Value;
  }

  template <typename ValueT>
  void operator()(
    vtkAOSDataArrayTemplate<ValueT>* array, nlohmann::json& state, vtkSerializer* serializer)
  {
    if (array == nullptr)
    {
      return;
    }
    else if (!array->GetNumberOfValues())
    {
      return;
    }

    auto blob = vtk::TakeSmartPointer(vtkTypeUInt8Array::New());
    vtkIdType arrSize = array->GetNumberOfValues() * array->GetDataTypeSize();
    blob->SetArray(reinterpret_cast<vtkTypeUInt8*>(array->GetPointer(0)), arrSize, 1);
    Serialize_Blob(blob, state, serializer);

    if (auto lt = array->GetLookupTable())
    {
      state["LookupTable"] = serializer->SerializeJSON(lt);
    }
  }

  void operator()(vtkDataArray* array, nlohmann::json& state, vtkSerializer* serializer)
  {
    if (array == nullptr)
    {
      return;
    }
    else if (!array->GetNumberOfValues())
    {
      return;
    }

    auto blob = vtk::TakeSmartPointer(vtkTypeUInt8Array::New());
    vtkIdType arrSize = array->GetNumberOfValues() * array->GetDataTypeSize();
    blob->SetArray(reinterpret_cast<vtkTypeUInt8*>(array->GetVoidPointer(0)), arrSize, 1);
    Serialize_Blob(blob, state, serializer);

    if (auto lt = array->GetLookupTable())
    {
      state["LookupTable"] = serializer->SerializeJSON(lt);
    }
  }

  void operator()(vtkBitArray* array, nlohmann::json& state, vtkSerializer* serializer)
  {
    if (array == nullptr)
    {
      return;
    }
    else if (!array->GetNumberOfValues())
    {
      return;
    }

    auto blob = vtk::TakeSmartPointer(vtkTypeUInt8Array::New());
    vtkIdType arrSize = (array->GetNumberOfValues() + 7) / 8;
    state["NumberOfBits"] = array->GetNumberOfValues();
    blob->SetArray(array->GetPointer(0), arrSize, 1);
    Serialize_Blob(blob, state, serializer);

    if (auto lt = array->GetLookupTable())
    {
      state["LookupTable"] = serializer->SerializeJSON(lt);
    }
  }
};

struct vtkDataArrayDeserializer
{
  template <typename ValueT>
  void operator()(vtkAffineArray<ValueT>* array, const nlohmann::json& state,
    vtkDeserializer* vtkNotUsed(deserializer))
  {
    ValueT slope = state["Slope"].get<ValueT>();
    ValueT intercept = state["Intercept"].get<ValueT>();
    array->SetBackend(std::make_shared<vtkAffineImplicitBackend<ValueT>>(slope, intercept));
  }

  template <typename ValueT>
  void operator()(vtkConstantArray<ValueT>* array, const nlohmann::json& state,
    vtkDeserializer* vtkNotUsed(deserializer))
  {
    ValueT value = state["Value"].get<ValueT>();
    array->SetBackend(std::make_shared<vtkConstantImplicitBackend<ValueT>>(value));
  }

  template <typename ValueT>
  void operator()(vtkAOSDataArrayTemplate<ValueT>* array, const nlohmann::json& state,
    vtkDeserializer* deserializer)
  {
    nlohmann::json blob;
    if (!Deserialize_Blob(blob, state, deserializer))
    {
      return;
    }
    const auto& content = blob.get_binary();
    const ValueT* c_ptr = reinterpret_cast<const ValueT*>(content.data());
    auto src = const_cast<ValueT*>(c_ptr);
    auto dst = array->GetPointer(0);
    std::copy(src, src + array->GetNumberOfValues(), dst);
    VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE(LookupTable, vtkLookupTable, state, array, deserializer);
  }

  void operator()(vtkDataArray* array, const nlohmann::json& state, vtkDeserializer* deserializer)
  {
    nlohmann::json blob;
    if (!Deserialize_Blob(blob, state, deserializer))
    {
      return;
    }
    const auto& content = blob.get_binary();

    switch (array->GetDataType())
    {
      vtkTemplateMacro(const VTK_TT* c_ptr = reinterpret_cast<const VTK_TT*>(content.data());
                       auto src = const_cast<VTK_TT*>(c_ptr);
                       auto dst = reinterpret_cast<VTK_TT*>(array->GetVoidPointer(0));
                       std::copy(src, src + array->GetNumberOfValues(), dst));
    }
    // nifty memory savings below, unfortunately, doesn't work correctly when there are point
    // scalars.
    // array->SetVoidArray(const_cast<void*>(c_ptr), array->GetNumberOfValues(), 1);
    VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE(LookupTable, vtkLookupTable, state, array, deserializer);
  }

  void operator()(vtkBitArray* array, const nlohmann::json& state, vtkDeserializer* deserializer)
  {
    nlohmann::json blob;
    if (!Deserialize_Blob(blob, state, deserializer))
    {
      return;
    }
    const auto& content = blob.get_binary();
    std::copy(content.data(), content.data() + ((array->GetNumberOfValues() + 7) / 8),
      array->GetPointer(0));
    array->SetNumberOfValues(state["NumberOfBits"]);
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
  using Dispatch = vtkArrayDispatch::DispatchByArray<DispatchTypeList>;
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
  using Dispatch = vtkArrayDispatch::DispatchByArray<DispatchTypeList>;
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
