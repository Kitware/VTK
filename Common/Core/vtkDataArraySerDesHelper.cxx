// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAffineArray.h"
#include "vtkArrayDispatch.h"
#include "vtkBitArray.h"
#include "vtkConstantArray.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDeserializer.h"
#include "vtkInvoker.h"
#include "vtkLookupTable.h"
#include "vtkSerializer.h"
#include "vtkSetGet.h"
#include "vtkTypeUInt8Array.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

#include <algorithm> // for any_of
#include <string>    // for string
#include <vector>    // for vector

extern "C"
{
  /**
   * Register the (de)serialization and invocation handlers of vtkDataArray subclasses
   * @param ser     a vtkSerializer instance
   * @param deser   a vtkDeserializer instance
   * @param invoker a vtkInvoker instance
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
#define TTYPE_INFO_MACRO(className) { #className, className::New, typeid(className) }

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

// The templated types should match those in the TEMPLATED_ARRAY_TYPES_INFO_MACRO.
#define TEMPLATED_ARRAY_NAME_DEMANGLE_MACRO(className, ValueType, valueTypeName)                   \
  template <>                                                                                      \
  const char* GetDemangledClassNameFor## className<ValueType>(const char* templateArrayClassName)  \
  {                                                                                                \
    if (!strcmp(templateArrayClassName, #className))                                               \
    {                                                                                              \
      return #className "<" #valueTypeName ">";                                                    \
    }                                                                                              \
    vtkLogF(ERROR, "Specialization missing for " #className "<" #ValueType ">");                   \
    return nullptr;                                                                                \
  }

#define TEMPLATED_ARRAY_NAMES_DEMANGLE_MACRO(className)                                            \
  template <typename ValueType>                                                                    \
  const char* GetDemangledClassNameFor## className(const char* templateArrayClassName)             \
  {                                                                                                \
    (void)templateArrayClassName;                                                                  \
    vtkLogF(ERROR, "Specialization missing for " #className "<ValueType>");                        \
    return nullptr;                                                                                \
  }                                                                                                \
  TEMPLATED_ARRAY_NAME_DEMANGLE_MACRO(className, char, char);                                      \
  TEMPLATED_ARRAY_NAME_DEMANGLE_MACRO(className, double, double);                                  \
  TEMPLATED_ARRAY_NAME_DEMANGLE_MACRO(className, float, float);                                    \
  TEMPLATED_ARRAY_NAME_DEMANGLE_MACRO(className, int, int);                                        \
  TEMPLATED_ARRAY_NAME_DEMANGLE_MACRO(className, long, long);                                      \
  TEMPLATED_ARRAY_NAME_DEMANGLE_MACRO(className, long long, long long);                            \
  TEMPLATED_ARRAY_NAME_DEMANGLE_MACRO(className, short, short);                                    \
  TEMPLATED_ARRAY_NAME_DEMANGLE_MACRO(className, signed char, signed char);                        \
  TEMPLATED_ARRAY_NAME_DEMANGLE_MACRO(className, unsigned char, unsigned char);                    \
  TEMPLATED_ARRAY_NAME_DEMANGLE_MACRO(className, unsigned int, unsigned int);                      \
  TEMPLATED_ARRAY_NAME_DEMANGLE_MACRO(className, unsigned long, unsigned long);                    \
  TEMPLATED_ARRAY_NAME_DEMANGLE_MACRO(className, unsigned long long, unsigned long long);          \
  TEMPLATED_ARRAY_NAME_DEMANGLE_MACRO(className, unsigned short, unsigned short)

// The wrapping tools skip templated classes, so the templated array classes have no generated
// (de)serializer, constructor or invoker. This helper registers the handlers of vtkDataArray
// for them. The concrete array classes are marshalled by generated code that chains into the
// handlers which this helper registers for vtkDataArray and for the templated array classes.
std::vector<ArrayTypeInfo> TemplatedArrayTypes = {
  TEMPLATED_ARRAY_TYPES_INFO_MACRO(vtkAOSDataArrayTemplate),
  TEMPLATED_ARRAY_TYPES_INFO_MACRO(vtkAffineArray),
  TEMPLATED_ARRAY_TYPES_INFO_MACRO(vtkConstantArray)
};

/**
 * Whether `array` is an instance of a templated array class, i.e a class that the wrapping
 * tools skip and for which this helper is the only handler.
 */
bool IsTemplatedArray(vtkDataArray* array)
{
  const auto& typeInfo = typeid(*array);
  return std::any_of(TemplatedArrayTypes.begin(), TemplatedArrayTypes.end(),
    [&typeInfo](const ArrayTypeInfo& arrayType) { return arrayType.TypeInfo == typeInfo; });
}

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

typedef vtkTypeList::Append<vtkArrayDispatch::AOSArrays,
  vtkBitArray,
  AffineArrays,
  ConstantArrays>::Result
  DispatchTypeList;

// clang-format on

TEMPLATED_ARRAY_NAMES_DEMANGLE_MACRO(vtkAOSDataArrayTemplate);
TEMPLATED_ARRAY_NAMES_DEMANGLE_MACRO(vtkAffineArray);
TEMPLATED_ARRAY_NAMES_DEMANGLE_MACRO(vtkConstantArray);

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
    // demangle and record the actual templated class name prior to early outs
    // to ensure the deserializer on the other end can create the correct type.
    if (strstr(array->GetClassName(), "vtkAffineArray"))
    {
      // demangle and record the actual templated class name
      state["ClassName"] = GetDemangledClassNameForvtkAffineArray<ValueT>("vtkAffineArray");
    }
    if (array->GetBackend() == nullptr)
    {
      vtkLogF(ERROR, "AffineArray backend is null");
      return;
    }
    state["Slope"] = array->GetSlope();
    state["Intercept"] = array->GetIntercept();
  }

  template <typename ValueT>
  void operator()(
    vtkConstantArray<ValueT>* array, nlohmann::json& state, vtkSerializer* vtkNotUsed(serializer))
  {
    if (array == nullptr)
    {
      return;
    }
    // demangle and record the actual templated class name prior to early outs
    // to ensure the deserializer on the other end can create the correct type.
    if (strstr(array->GetClassName(), "vtkConstantArray"))
    {
      // demangle and record the actual templated class name
      state["ClassName"] = GetDemangledClassNameForvtkConstantArray<ValueT>("vtkConstantArray");
    }
    if (array->GetBackend() == nullptr)
    {
      vtkLogF(ERROR, "ConstantArray backend is null");
      return;
    }
    state["Value"] = array->GetConstantValue();
  }

  template <typename ValueT>
  void operator()(
    vtkAOSDataArrayTemplate<ValueT>* array, nlohmann::json& state, vtkSerializer* serializer)
  {
    if (array == nullptr)
    {
      return;
    }
    // demangle and record the actual templated class name prior to early outs
    // to ensure the deserializer on the other end can create the correct type.
    if (strstr(array->GetClassName(), "vtkAOSDataArrayTemplate"))
    {
      // demangle and record the actual templated class name
      state["ClassName"] =
        GetDemangledClassNameForvtkAOSDataArrayTemplate<ValueT>("vtkAOSDataArrayTemplate");
    }
    if (!array->GetNumberOfValues())
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
    if (!array->GetNumberOfValues())
    {
      return;
    }

    auto blob = vtk::TakeSmartPointer(vtkTypeUInt8Array::New());
    vtkIdType arrSize = array->GetNumberOfValues() * array->GetDataTypeSize();
    // copy data into blob
    blob->SetNumberOfValues(arrSize);
    switch (array->GetDataType())
    {
      vtkTemplateMacro(
        std::copy_n(vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, VTK_TT>(array).begin(),
          array->GetNumberOfValues(), reinterpret_cast<VTK_TT*>(blob->GetPointer(0))));
    }
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
    if (!array->GetNumberOfValues())
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
    vtkDeserializer* vtkNotUsed(deserializer), bool& vtkNotUsed(success))
  {
    ValueT slope = state["Slope"].get<ValueT>();
    ValueT intercept = state["Intercept"].get<ValueT>();
    array->ConstructBackend(slope, intercept);
  }

  template <typename ValueT>
  void operator()(vtkConstantArray<ValueT>* array, const nlohmann::json& state,
    vtkDeserializer* vtkNotUsed(deserializer), bool& vtkNotUsed(success))
  {
    ValueT value = state["Value"].get<ValueT>();
    array->ConstructBackend(value);
  }

  template <typename ValueT>
  void operator()(vtkAOSDataArrayTemplate<ValueT>* array, const nlohmann::json& state,
    vtkDeserializer* deserializer, bool& success)
  {
    nlohmann::json blob;
    if (!Deserialize_Blob(blob, state, deserializer))
    {
      success = false;
      return;
    }
    const auto& content = blob.get_binary();
    std::copy_n(reinterpret_cast<const ValueT*>(content.data()), array->GetNumberOfValues(),
      array->GetPointer(0));
    VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE(LookupTable, vtkLookupTable, state, array, deserializer);
  }

  void operator()(
    vtkDataArray* array, const nlohmann::json& state, vtkDeserializer* deserializer, bool& success)
  {
    nlohmann::json blob;
    if (!Deserialize_Blob(blob, state, deserializer))
    {
      success = false;
      return;
    }
    const auto& content = blob.get_binary();
    switch (array->GetDataType())
    {
      vtkTemplateMacro(
        std::copy_n(reinterpret_cast<const VTK_TT*>(content.data()), array->GetNumberOfValues(),
          vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, VTK_TT>(array).begin()));
    }
    VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE(LookupTable, vtkLookupTable, state, array, deserializer);
  }

  void operator()(
    vtkBitArray* array, const nlohmann::json& state, vtkDeserializer* deserializer, bool& success)
  {
    nlohmann::json blob;
    if (!Deserialize_Blob(blob, state, deserializer))
    {
      success = false;
      return;
    }
    const auto& content = blob.get_binary();
    std::copy_n(content.data(), (array->GetNumberOfValues() + 7) / 8, array->GetPointer(0));
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
  if (auto* context = serializer->GetContext())
  {
    const auto id = context->GetId(object);
    if (id > 0)
    {
      state = context->GetState(id);
      if (auto iter = state.find("MTime"); (iter != state.end() && !iter->is_null()))
      {
        const auto stateMTime = state.at("MTime").get<vtkMTimeType>();
        if (da->GetMTime() <= stateMTime)
        {
          return state;
        }
        vtkVLog(serializer->GetSerializerLogVerbosity(),
          << "Reserialize because object-mtime=" << da->GetMTime()
          << " >= state-mtime=" << stateMTime);
      }
    }
  }
  else
  {
    vtkErrorWithObjectMacro(serializer, "Serializer does not have a marshal context!");
    return state;
  }
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
  // the generated serializer of a concrete array class reports vtkDataArray, and every class
  // in between, as its superclass. A templated array class has no generated serializer, so
  // report it here for those.
  if (IsTemplatedArray(da))
  {
    superClasses.push_back("vtkDataArray");
  }
  return state;
}

static bool Deserialize_vtkDataArray(
  const nlohmann::json& state, vtkObjectBase* object, vtkDeserializer* deserializer)
{
  auto* da = vtkDataArray::SafeDownCast(object);
  if (!da)
  {
    vtkErrorWithObjectMacro(deserializer, << __func__ << ": object not a vtkDataArray");
    return false;
  }
  bool success = true;
  if (const auto superDeserializer = deserializer->GetHandler(typeid(vtkDataArray::Superclass)))
  {
    success &= superDeserializer(state, object, deserializer);
  }
  if (!success)
  {
    return false;
  }
  if (!da->GetNumberOfValues())
  {
    return success;
  }
  vtkDataArrayDeserializer deserializeWorker;
  using Dispatch = vtkArrayDispatch::DispatchByArray<DispatchTypeList>;
  if (!Dispatch::Execute(da, deserializeWorker, state, deserializer, success))
  {
    deserializeWorker(da, state, deserializer, success);
  }
  return success;
}

int RegisterHandlers_vtkDataArraySerDesHelper(void* ser, void* deser, void* invoker)
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkDataArray), Serialize_vtkDataArray);
      for (auto& arrayType : TemplatedArrayTypes)
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
      deserializer->RegisterHandler(typeid(vtkDataArray), Deserialize_vtkDataArray);
      for (auto& arrayType : TemplatedArrayTypes)
      {
        // the name of a templated array class is the demangled name recorded by the serializer.
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
      for (auto& arrayType : TemplatedArrayTypes)
      {
        invokerObject->RegisterHandler(
          arrayType.TypeInfo, invokerObject->GetHandler(typeid(vtkDataArray)));
      }
      success = 1;
    }
  }
  return success;
}
