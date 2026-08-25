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

#include <algorithm>   // for any_of, transform
#include <cstddef>     // for size_t
#include <cstdint>     // for fixed width integer types
#include <string>      // for string
#include <type_traits> // for conditional, is_integral, is_signed
#include <vector>      // for vector

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

/**
 * Copy `numValues` values out of `content` into `out`, widening or narrowing them
 * when the writer's value width `srcDataTypeSize` differs from `sizeof(DstT)`.
 *
 * The class name recorded in the state already states signedness and
 * integral-vs-floating, so the byte width alone is sufficient to reconstruct the
 * writer's value type. Returns false and reports an error when the blob is too
 * small for the array, or when the two widths cannot be reconciled.
 */
template <typename DstT, typename OutIteratorT>
bool DeserializeValues(const nlohmann::json::binary_t& content, int srcDataTypeSize,
  vtkIdType numValues, OutIteratorT out, vtkDeserializer* deserializer)
{
  constexpr int dstDataTypeSize = static_cast<int>(sizeof(DstT));
  auto* context = deserializer->GetContext();
  if (srcDataTypeSize <= 0)
  {
    vtkErrorWithObjectMacro(context,
      << deserializer->GetObjectDescription() << " got invalid DataTypeSize=" << srcDataTypeSize);
    return false;
  }
  const std::size_t requiredSize =
    static_cast<std::size_t>(numValues) * static_cast<std::size_t>(srcDataTypeSize);
  if (requiredSize > content.size())
  {
    vtkErrorWithObjectMacro(context, << deserializer->GetObjectDescription() << " needs "
                                     << requiredSize << " bytes for " << numValues << " values of "
                                     << srcDataTypeSize << " bytes, but the blob holds only "
                                     << content.size() << " bytes");
    return false;
  }
  if (srcDataTypeSize == dstDataTypeSize)
  {
    std::copy_n(reinterpret_cast<const DstT*>(content.data()), numValues, out);
    return true;
  }
  if constexpr (std::is_integral<DstT>::value)
  {
    constexpr bool isSigned = std::is_signed<DstT>::value;
    using WideT = std::conditional_t<isSigned, std::int64_t, std::uint64_t>;
    using NarrowT = std::conditional_t<isSigned, std::int32_t, std::uint32_t>;
    if (srcDataTypeSize == 8 && dstDataTypeSize == 4)
    {
      const auto* src = reinterpret_cast<const WideT*>(content.data());
      std::transform(
        src, src + numValues, out, [](WideT value) { return static_cast<DstT>(value); });
      return true;
    }
    if (srcDataTypeSize == 4 && dstDataTypeSize == 8)
    {
      const auto* src = reinterpret_cast<const NarrowT*>(content.data());
      std::transform(
        src, src + numValues, out, [](NarrowT value) { return static_cast<DstT>(value); });
      return true;
    }
  }
  vtkErrorWithObjectMacro(context, << deserializer->GetObjectDescription() << " cannot convert "
                                   << srcDataTypeSize << "-byte values into " << dstDataTypeSize
                                   << "-byte values");
  return false;
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
    // Record our value width so a peer with a different vtkIdType/long width can convert.
    state["DataTypeSize"] = array->GetDataTypeSize();
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
    // Record our value width so a peer with a different vtkIdType/long width can convert.
    state["DataTypeSize"] = array->GetDataTypeSize();
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
    int srcDataTypeSize = array->GetDataTypeSize();
    if (const auto it = state.find("DataTypeSize"); it != state.end())
    {
      srcDataTypeSize = it->get<int>();
    }
    if (!DeserializeValues<ValueT>(
          content, srcDataTypeSize, array->GetNumberOfValues(), array->GetPointer(0), deserializer))
    {
      success = false;
      return;
    }
    const auto id = state["Id"].get<vtkTypeUInt32>();
    const auto hash = state["Hash"].get<std::string>();
    if (deserializer->GetContext()->ShouldDataArrayBeModified(id, hash))
    {
      array->Modified();
    }
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
    int srcDataTypeSize = array->GetDataTypeSize();
    if (const auto it = state.find("DataTypeSize"); it != state.end())
    {
      srcDataTypeSize = it->get<int>();
    }
    bool copied = false;
    switch (array->GetDataType())
    {
      vtkTemplateMacro(
        copied = DeserializeValues<VTK_TT>(content, srcDataTypeSize, array->GetNumberOfValues(),
          vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, VTK_TT>(array).begin(),
          deserializer));
    }
    if (!copied)
    {
      success = false;
      return;
    }
    const auto id = state["Id"].get<vtkTypeUInt32>();
    const auto hash = state["Hash"].get<std::string>();
    if (deserializer->GetContext()->ShouldDataArrayBeModified(id, hash))
    {
      array->Modified();
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
    const auto id = state["Id"].get<vtkTypeUInt32>();
    const auto hash = state["Hash"].get<std::string>();
    if (deserializer->GetContext()->ShouldDataArrayBeModified(id, hash))
    {
      array->Modified();
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
