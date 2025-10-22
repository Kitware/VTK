// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkArrayDispatch_txx
#define vtkArrayDispatch_txx

#include "vtkArrayDispatch.h"

#include "vtkSetGet.h" // For warning macros.

#include <array>
#include <cstddef>
#include <tuple>   // For std::tuple, std::apply (C++17)
#include <utility> // For std::forward, std::move, std::apply

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
VTK_ABI_NAMESPACE_END

namespace vtkArrayDispatch
{
namespace impl
{
VTK_ABI_NAMESPACE_BEGIN

static constexpr int NumValueTypes = VTK_OBJECT + 1;
static constexpr int NumArrayTypes = vtkArrayTypes::VTK_NUM_ARRAY_TYPES;
static constexpr int NumArrays = NumValueTypes * NumArrayTypes;

struct ArrayListIndexMap
{
  template <std::size_t... I>
  static constexpr std::array<int, NumArrays> InitArrayMap(std::index_sequence<I...>)
  {
    // Each element expands to NumArrays
    return { { ((void)I, NumArrays)... } };
  }

  template <typename ArrayList, int Index>
  static constexpr void Fill(std::array<int, NumArrays>& result)
  {
    if constexpr (!std::is_same_v<ArrayList, vtkTypeList::NullType>)
    {
      using ArrayType = typename ArrayList::Head;
      using ArrayTypeTag = typename ArrayType::ArrayTypeTag;
      using DataTypeTag = typename ArrayType::DataTypeTag;
      constexpr int arrayTypeId = ArrayTypeTag::value;
      constexpr int dataTypeId = DataTypeTag::value;
      constexpr int arrayId = arrayTypeId * NumValueTypes + dataTypeId;
      result[arrayId] = Index;
      if constexpr (dataTypeId == VTK_ID_TYPE_IMPL)
      {
        // Also fill in the entry for VTK_ID_TYPE
        constexpr int arrayIdVTKIdType = arrayTypeId * NumValueTypes + VTK_ID_TYPE;
        result[arrayIdVTKIdType] = Index;
      }
      // Recurse for remaining types
      Fill<typename ArrayList::Tail, Index + 1>(result);
    }
  }

  template <typename ArrayList>
  static constexpr std::array<int, NumArrays> GetCompactArrayMap()
  {
    // Default initialize all entries to NumArrayTypes
    auto result = InitArrayMap(std::make_index_sequence<NumArrays>{});
    // Fill valid indices from ArrayList
    Fill<ArrayList, 0>(result);
    return result;
  }
};

//------------------------------------------------------------------------------
// Implementation of the single-array dispatch mechanism.
template <typename ArrayList>
struct Dispatch;

using KnownArrayTypeTagList =
  vtkTypeList::Create<std::integral_constant<int, vtkArrayTypes::VTK_STRING_ARRAY>,
    std::integral_constant<int, vtkArrayTypes::VTK_VARIANT_ARRAY>,
    std::integral_constant<int, vtkArrayTypes::VTK_BIT_ARRAY>,
    std::integral_constant<int, vtkArrayTypes::VTK_AOS_DATA_ARRAY>,
    std::integral_constant<int, vtkArrayTypes::VTK_SOA_DATA_ARRAY>,
    std::integral_constant<int, vtkArrayTypes::VTK_SCALED_SOA_DATA_ARRAY>,
    std::integral_constant<int, vtkArrayTypes::VTKM_DATA_ARRAY>,
    std::integral_constant<int, vtkArrayTypes::VTK_PERIODIC_DATA_ARRAY>,
    std::integral_constant<int, vtkArrayTypes::VTK_AFFINE_ARRAY>,
    std::integral_constant<int, vtkArrayTypes::VTK_COMPOSITE_ARRAY>,
    std::integral_constant<int, vtkArrayTypes::VTK_CONSTANT_ARRAY>,
    std::integral_constant<int, vtkArrayTypes::VTK_INDEXED_ARRAY>,
    std::integral_constant<int, vtkArrayTypes::VTK_STD_FUNCTION_ARRAY>,
    std::integral_constant<int, vtkArrayTypes::VTK_STRIDED_ARRAY>,
    std::integral_constant<int, vtkArrayTypes::VTK_STRUCTURED_POINT_ARRAY>>;

// Recursive case:
template <typename ArrayHead, typename ArrayTail>
struct Dispatch<vtkTypeList::TypeList<ArrayHead, ArrayTail>>
{
  using ArrayList = vtkTypeList::TypeList<ArrayHead, ArrayTail>;

  using KnownArrayList =
    typename FilterArraysByArrayTypeTag<ArrayList, KnownArrayTypeTagList>::Result;

  static constexpr int NumKnownArrays = vtkTypeList::Size<KnownArrayList>::Result;
  static constexpr auto CompactArrayMap = ArrayListIndexMap::GetCompactArrayMap<KnownArrayList>();
  static constexpr int GetCompactArrayIndex(int arrayIndex) { return CompactArrayMap[arrayIndex]; }

  template <typename... Params>
  using ParamsTuple = std::tuple<Params...>;

  template <typename Worker, typename... Params>
  using DispatchFunction = bool (*)(
    vtkAbstractArray* inArray, Worker&& worker, ParamsTuple<Params...> paramsTuple);

  template <typename TArrayList, typename Worker, typename... Params>
  static bool ExecuteUnknown(vtkAbstractArray* inArray, Worker&& worker, Params&&... params)
  {
    if constexpr (!std::is_same_v<TArrayList, vtkTypeList::NullType>)
    {
      using ArrayType = typename TArrayList::Head;
      if (auto array = vtkArrayDownCast<ArrayType>(inArray))
      {
        worker(array, std::forward<Params>(params)...);
        return true;
      }
      else
      {
        return ExecuteUnknown<typename TArrayList::Tail>(
          inArray, std::forward<Worker>(worker), std::forward<Params>(params)...);
      }
    }
    else
    {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
      vtkGenericWarningMacro("Array dispatch failed.");
#endif
      return false;
    }
  }

  template <typename TArrayType, typename Worker, typename... Params>
  static constexpr bool ExecuteKnown(
    vtkAbstractArray* inArray, Worker&& worker, ParamsTuple<Params...> paramsTuple)
  {
    std::apply(
      [&worker, typedArray = static_cast<TArrayType*>(inArray)](Params&&... forwardedParams)
      { worker(typedArray, std::forward<Params>(forwardedParams)...); },
      std::move(paramsTuple));
    return true;
  }

  template <typename TArrayList, typename Worker, typename... Params>
  static constexpr void FillKnownArrayHandlers(
    std::array<DispatchFunction<Worker, Params...>, NumKnownArrays>& arr)
  {
    if constexpr (!std::is_same_v<TArrayList, vtkTypeList::NullType>)
    {
      using ArrayType = typename TArrayList::Head;
      using ArrayTypeTag = typename ArrayType::ArrayTypeTag;
      using DataTypeTag = typename ArrayType::DataTypeTag;
      constexpr int arrayTypeId = ArrayTypeTag::value;
      constexpr int dataTypeId = DataTypeTag::value;
      constexpr int arrayIndex = arrayTypeId * NumValueTypes + dataTypeId;
      constexpr int compactArrayIndex = GetCompactArrayIndex(arrayIndex);
      arr[compactArrayIndex] = &ExecuteKnown<ArrayType, Worker, Params...>;
      // Recurse for remaining types
      FillKnownArrayHandlers<typename TArrayList::Tail, Worker, Params...>(arr);
    }
  }

  template <typename TKnownArrayList, typename Worker, typename... Params>
  static constexpr std::array<DispatchFunction<Worker, Params...>, NumKnownArrays>
  GetArrayHandlers()
  {
    std::array<DispatchFunction<Worker, Params...>, NumKnownArrays> arr{};
    if constexpr (NumKnownArrays > 0)
    {
      FillKnownArrayHandlers<TKnownArrayList, Worker, Params...>(arr);
    }
    return arr;
  }

  template <typename Worker, typename... Params>
  static bool Execute(vtkAbstractArray* inArray, Worker&& worker, Params&&... params)
  {
    // Check for null arrays
    if (VTK_UNLIKELY(!inArray))
    {
      return false;
    }
    constexpr auto arrayHandlers = GetArrayHandlers<KnownArrayList, Worker, Params...>();
    const auto arrayIndex = inArray->GetArrayType() * NumValueTypes + inArray->GetDataType();
    const auto compactArrayIndex = GetCompactArrayIndex(arrayIndex);
    if (compactArrayIndex < NumKnownArrays)
    {
      ParamsTuple<Params...> paramsTuple{ std::forward<Params>(params)... };
      return arrayHandlers[compactArrayIndex](
        inArray, std::forward<Worker>(worker), std::move(paramsTuple));
    }
    // Fallback if unknown array types are present in ArrayList
    return ExecuteUnknown<ArrayList>(
      inArray, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};

//------------------------------------------------------------------------------
// Description:
// Implementation of the 2 array dispatch mechanism.
template <typename ArrayList1, typename ArrayList2>
struct Dispatch2;

// Recursive case:
template <typename Array1Head, typename Array1Tail, typename Array2Head, typename Array2Tail>
struct Dispatch2<vtkTypeList::TypeList<Array1Head, Array1Tail>,
  vtkTypeList::TypeList<Array2Head, Array2Tail>>
{
  using Array1List = vtkTypeList::TypeList<Array1Head, Array1Tail>;
  using Array2List = vtkTypeList::TypeList<Array2Head, Array2Tail>;

  using Known1List = typename FilterArraysByArrayTypeTag<Array1List, KnownArrayTypeTagList>::Result;
  using Known2List = typename FilterArraysByArrayTypeTag<Array2List, KnownArrayTypeTagList>::Result;

  static constexpr int NumKnownArrays1 = vtkTypeList::Size<Known1List>::Result;
  static constexpr int NumKnownArrays2 = vtkTypeList::Size<Known2List>::Result;
  static constexpr int NumKnownArrayPairs = NumKnownArrays1 * NumKnownArrays2;

  // Use two separate index maps instead of one N² map
  static constexpr auto CompactArray1Map = ArrayListIndexMap::GetCompactArrayMap<Known1List>();
  static constexpr auto CompactArray2Map = ArrayListIndexMap::GetCompactArrayMap<Known2List>();
  static constexpr int GetCompactArray1Index(int arrayIndex)
  {
    return CompactArray1Map[arrayIndex];
  }
  static constexpr int GetCompactArray2Index(int arrayIndex)
  {
    return CompactArray2Map[arrayIndex];
  }

  template <typename... Params>
  using ParamsTuple = std::tuple<Params...>;

  template <typename Worker, typename... Params>
  using Dispatch2Function = bool (*)(vtkAbstractArray* array1, vtkAbstractArray* array2,
    Worker&& worker, ParamsTuple<Params...> paramsTuple);

  template <typename Array1Type, typename Array2List, typename Worker, typename... Params>
  static constexpr bool ExecuteUnknown2(
    Array1Type* array1, vtkAbstractArray* array2, Worker&& worker, Params&&... params)
  {
    if constexpr (!std::is_same_v<Array2List, vtkTypeList::NullType>)
    {
      using Array2Type = typename Array2List::Head;
      if (auto typedArray2 = vtkArrayDownCast<Array2Type>(array2))
      {
        worker(array1, typedArray2, std::forward<Params>(params)...);
        return true;
      }
      else
      {
        return ExecuteUnknown2<Array1Type, typename Array2List::Tail>(
          array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
      }
    }
    else
    {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
      vtkGenericWarningMacro("Dual array dispatch failed.");
#endif
      return false;
    }
  }

  template <typename Array1List, typename Array2List, typename Worker, typename... Params>
  static constexpr bool ExecuteUnknown1(
    vtkAbstractArray* array1, vtkAbstractArray* array2, Worker&& worker, Params&&... params)
  {
    if constexpr (!std::is_same_v<Array1List, vtkTypeList::NullType>)
    {
      using Array1Type = typename Array1List::Head;
      if (auto typedArray1 = vtkArrayDownCast<Array1Type>(array1))
      {
        return ExecuteUnknown2<Array1Type, Array2List>(
          typedArray1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
      }
      else
      {
        return ExecuteUnknown1<typename Array1List::Tail, Array2List>(
          array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
      }
    }
    else
    {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
      vtkGenericWarningMacro("Dual array dispatch failed.");
#endif
      return false;
    }
  }

  template <typename Array1List, typename Array2List, typename Worker, typename... Params>
  static bool ExecuteUnknown(
    vtkAbstractArray* array1, vtkAbstractArray* array2, Worker&& worker, Params&&... params)
  {
    return ExecuteUnknown1<Array1List, Array2List>(
      array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }

  template <typename Array1Type, typename Array2Type, typename Worker, typename... Params>
  static constexpr bool ExecuteKnown(vtkAbstractArray* array1, vtkAbstractArray* array2,
    Worker&& worker, ParamsTuple<Params...> paramsTuple)
  {
    std::apply([&worker, typedArray1 = static_cast<Array1Type*>(array1),
                 typedArray2 = static_cast<Array2Type*>(array2)](Params&&... forwardedParams)
      { worker(typedArray1, typedArray2, std::forward<Params>(forwardedParams)...); },
      std::move(paramsTuple));
    return true;
  }

  template <typename Array1Type, typename Array2List, int CompactArray1Index, typename Worker,
    typename... Params>
  static constexpr void FillArray2(
    std::array<Dispatch2Function<Worker, Params...>, NumKnownArrayPairs>& arr)
  {
    if constexpr (!std::is_same_v<Array2List, vtkTypeList::NullType>)
    {
      using Array2Type = typename Array2List::Head;
      using Array2TypeTag = typename Array2Type::ArrayTypeTag;
      using Data2TypeTag = typename Array2Type::DataTypeTag;
      constexpr int array2TypeId = Array2TypeTag::value;
      constexpr int data2TypeId = Data2TypeTag::value;
      constexpr int array2Index = array2TypeId * NumValueTypes + data2TypeId;
      constexpr int compactArray2Index = GetCompactArray2Index(array2Index);
      constexpr int compactPairIndex = CompactArray1Index * NumKnownArrays2 + compactArray2Index;
      arr[compactPairIndex] = &ExecuteKnown<Array1Type, Array2Type, Worker, Params...>;
      // Recurse for remaining types
      FillArray2<Array1Type, typename Array2List::Tail, CompactArray1Index, Worker, Params...>(arr);
    }
  }

  template <typename Array1List, typename Array2List, typename Worker, typename... Params>
  static constexpr void FillArray1(
    std::array<Dispatch2Function<Worker, Params...>, NumKnownArrayPairs>& arr)
  {
    if constexpr (!std::is_same_v<Array1List, vtkTypeList::NullType>)
    {
      using Array1Type = typename Array1List::Head;
      using Array1TypeTag = typename Array1Type::ArrayTypeTag;
      using Data1TypeTag = typename Array1Type::DataTypeTag;
      constexpr int array1TypeId = Array1TypeTag::value;
      constexpr int data1TypeId = Data1TypeTag::value;
      constexpr int array1Index = array1TypeId * NumValueTypes + data1TypeId;
      constexpr int compactArray1Index = GetCompactArray1Index(array1Index);
      FillArray2<Array1Type, Array2List, compactArray1Index, Worker, Params...>(arr);
      // Recurse for remaining types
      FillArray1<typename Array1List::Tail, Array2List, Worker, Params...>(arr);
    }
  }

  template <typename Array1List, typename Array2List, typename Worker, typename... Params>
  static constexpr void FillKnownArrayPairHandlers(
    std::array<Dispatch2Function<Worker, Params...>, NumKnownArrayPairs>& arr)
  {
    FillArray1<Array1List, Array2List, Worker, Params...>(arr);
  }

  template <typename Known1List, typename Known2List, typename Worker, typename... Params>
  static constexpr std::array<Dispatch2Function<Worker, Params...>, NumKnownArrayPairs>
  GetArrayPairHandlers()
  {
    std::array<Dispatch2Function<Worker, Params...>, NumKnownArrayPairs> arr{};
    if constexpr (NumKnownArrayPairs > 0)
    {
      FillKnownArrayPairHandlers<Known1List, Known2List, Worker, Params...>(arr);
    }
    return arr;
  }

  template <typename Worker, typename... Params>
  static bool Execute(
    vtkAbstractArray* array1, vtkAbstractArray* array2, Worker&& worker, Params&&... params)
  {
    // Check for null arrays
    if (VTK_UNLIKELY(!array1 || !array2))
    {
      return false;
    }
    constexpr auto arrayPairHandlers =
      GetArrayPairHandlers<Known1List, Known2List, Worker, Params...>();
    const auto array1Index = array1->GetArrayType() * NumValueTypes + array1->GetDataType();
    const auto array2Index = array2->GetArrayType() * NumValueTypes + array2->GetDataType();
    const auto compactArray1Index = GetCompactArray1Index(array1Index);
    const auto compactArray2Index = GetCompactArray2Index(array2Index);
    if (compactArray1Index < NumKnownArrays1 && compactArray2Index < NumKnownArrays2)
    {
      ParamsTuple<Params...> paramsTuple{ std::forward<Params>(params)... };
      auto compactPairIndex = compactArray1Index * NumKnownArrays2 + compactArray2Index;
      return arrayPairHandlers[compactPairIndex](
        array1, array2, std::forward<Worker>(worker), std::move(paramsTuple));
    }
    // Fallback if unknown array types are present in Array1List or Array2List
    return ExecuteUnknown<Array1List, Array2List>(
      array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};

//------------------------------------------------------------------------------
// Description:
// Implementation of the 2 array same-type dispatch mechanism.
template <typename ArrayList1, typename ArrayList2>
struct Dispatch2Same;

// Recursive case:
template <typename Array1Head, typename Array1Tail, typename Array2Head, typename Array2Tail>
struct Dispatch2Same<vtkTypeList::TypeList<Array1Head, Array1Tail>,
  vtkTypeList::TypeList<Array2Head, Array2Tail>>
{
  using Array1List = vtkTypeList::TypeList<Array1Head, Array1Tail>;
  using Array2List = vtkTypeList::TypeList<Array2Head, Array2Tail>;

  using Known1List = typename FilterArraysByArrayTypeTag<Array1List, KnownArrayTypeTagList>::Result;
  using Known2List = typename FilterArraysByArrayTypeTag<Array2List, KnownArrayTypeTagList>::Result;

  static constexpr int NumKnownArrays1 = vtkTypeList::Size<Known1List>::Result;
  static constexpr int NumKnownArrays2 = vtkTypeList::Size<Known2List>::Result;
  static constexpr int NumKnownArrayPairs = NumKnownArrays1 * NumKnownArrays2;

  // Use two separate index maps instead of one N² map
  static constexpr auto CompactArray1Map = ArrayListIndexMap::GetCompactArrayMap<Known1List>();
  static constexpr auto CompactArray2Map = ArrayListIndexMap::GetCompactArrayMap<Known2List>();
  static constexpr int GetCompactArray1Index(int arrayIndex)
  {
    return CompactArray1Map[arrayIndex];
  }
  static constexpr int GetCompactArray2Index(int arrayIndex)
  {
    return CompactArray2Map[arrayIndex];
  }

  template <typename... Params>
  using ParamsTuple = std::tuple<Params...>;

  template <typename Worker, typename... Params>
  using Dispatch2Function = bool (*)(vtkAbstractArray* array1, vtkAbstractArray* array2,
    Worker&& worker, ParamsTuple<Params...> paramsTuple);

  template <typename Array1Type, typename Array2List, typename Worker, typename... Params>
  static constexpr bool ExecuteUnknown2(
    Array1Type* array1, vtkAbstractArray* array2, Worker&& worker, Params&&... params)
  {
    if constexpr (!std::is_same_v<Array2List, vtkTypeList::NullType>)
    {
      using Array2Type = typename Array2List::Head;
      if (auto typedArray2 = vtkArrayDownCast<Array2Type>(array2))
      {
        worker(array1, typedArray2, std::forward<Params>(params)...);
        return true;
      }
      else
      {
        return ExecuteUnknown2<Array1Type, typename Array2List::Tail>(
          array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
      }
    }
    else
    {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
      vtkGenericWarningMacro("Dual array dispatch failed.");
#endif
      return false;
    }
  }

  template <typename Array1List, typename Array2List, typename Worker, typename... Params>
  static constexpr bool ExecuteUnknown1(
    vtkAbstractArray* array1, vtkAbstractArray* array2, Worker&& worker, Params&&... params)
  {
    if constexpr (!std::is_same_v<Array1List, vtkTypeList::NullType>)
    {
      using Array1Type = typename Array1List::Head;
      if (auto typedArray1 = vtkArrayDownCast<Array1Type>(array1))
      {
        // Filter Array2List to only include arrays with the same DataTypeTag as Array1Type
        using DataTypeTagList = vtkTypeList::Create<typename Array1Type::DataTypeTag>;
        using FilteredArray2List =
          typename FilterArraysByDataTypeTag<Array2List, DataTypeTagList>::Result;
        return ExecuteUnknown2<Array1Type, FilteredArray2List>(
          typedArray1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
      }
      else
      {
        return ExecuteUnknown1<typename Array1List::Tail, Array2List>(
          array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
      }
    }
    else
    {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
      vtkGenericWarningMacro("Dual array dispatch failed.");
#endif
      return false;
    }
  }

  template <typename Array1List, typename Array2List, typename Worker, typename... Params>
  static bool ExecuteUnknown(
    vtkAbstractArray* array1, vtkAbstractArray* array2, Worker&& worker, Params&&... params)
  {
    return ExecuteUnknown1<Array1List, Array2List>(
      array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }

  template <typename Array1Type, typename Array2Type, typename Worker, typename... Params>
  static constexpr bool ExecuteKnown(vtkAbstractArray* array1, vtkAbstractArray* array2,
    Worker&& worker, ParamsTuple<Params...> paramsTuple)
  {
    static_assert(
      std::is_same_v<typename Array1Type::DataTypeTag, typename Array2Type::DataTypeTag>,
      "Array types must have the same DataTypeTag for Dispatch2Same");

    std::apply([&worker, typedArray1 = static_cast<Array1Type*>(array1),
                 typedArray2 = static_cast<Array2Type*>(array2)](Params&&... forwardedParams)
      { worker(typedArray1, typedArray2, std::forward<Params>(forwardedParams)...); },
      std::move(paramsTuple));
    return true;
  }

  template <typename Array1Type, typename Array2List, int CompactArray1Index, typename Worker,
    typename... Params>
  static constexpr void FillArray2(
    std::array<Dispatch2Function<Worker, Params...>, NumKnownArrayPairs>& arr)
  {
    if constexpr (!std::is_same_v<Array2List, vtkTypeList::NullType>)
    {
      using Array2Type = typename Array2List::Head;
      using Array2TypeTag = typename Array2Type::ArrayTypeTag;
      using Data2TypeTag = typename Array2Type::DataTypeTag;
      constexpr int array2TypeId = Array2TypeTag::value;
      constexpr int data2TypeId = Data2TypeTag::value;
      constexpr int array2Index = array2TypeId * NumValueTypes + data2TypeId;
      constexpr int compactArray2Index = GetCompactArray2Index(array2Index);
      constexpr int compactPairIndex = CompactArray1Index * NumKnownArrays2 + compactArray2Index;
      arr[compactPairIndex] = &ExecuteKnown<Array1Type, Array2Type, Worker, Params...>;
      // Recurse for remaining types
      FillArray2<Array1Type, typename Array2List::Tail, CompactArray1Index, Worker, Params...>(arr);
    }
  }

  template <typename Array1List, typename Array2List, typename Worker, typename... Params>
  static constexpr void FillArray1(
    std::array<Dispatch2Function<Worker, Params...>, NumKnownArrayPairs>& arr)
  {
    if constexpr (!std::is_same_v<Array1List, vtkTypeList::NullType>)
    {
      using Array1Type = typename Array1List::Head;
      using Array1TypeTag = typename Array1Type::ArrayTypeTag;
      using Data1TypeTag = typename Array1Type::DataTypeTag;
      constexpr int array1TypeId = Array1TypeTag::value;
      constexpr int data1TypeId = Data1TypeTag::value;
      constexpr int array1Index = array1TypeId * NumValueTypes + data1TypeId;
      constexpr int compactArray1Index = GetCompactArray1Index(array1Index);
      // Filter Array2List to only include arrays with the same DataTypeTag
      using DataTypeTagList = vtkTypeList::Create<typename Array1Type::DataTypeTag>;
      using FilteredArray2List =
        typename FilterArraysByDataTypeTag<Known2List, DataTypeTagList>::Result;
      FillArray2<Array1Type, FilteredArray2List, compactArray1Index, Worker, Params...>(arr);
      // Recurse for remaining types
      FillArray1<typename Array1List::Tail, Array2List, Worker, Params...>(arr);
    }
  }

  template <typename Array1List, typename Array2List, typename Worker, typename... Params>
  static constexpr void FillKnownArrayPairHandlers(
    std::array<Dispatch2Function<Worker, Params...>, NumKnownArrayPairs>& arr)
  {
    FillArray1<Array1List, Array2List, Worker, Params...>(arr);
  }

  template <typename Known1List, typename Known2List, typename Worker, typename... Params>
  static constexpr std::array<Dispatch2Function<Worker, Params...>, NumKnownArrayPairs>
  GetArrayPairHandlers()
  {
    std::array<Dispatch2Function<Worker, Params...>, NumKnownArrayPairs> arr{};
    if constexpr (NumKnownArrayPairs > 0)
    {
      FillKnownArrayPairHandlers<Known1List, Known2List, Worker, Params...>(arr);
    }
    return arr;
  }

  template <typename Worker, typename... Params>
  static bool Execute(
    vtkAbstractArray* array1, vtkAbstractArray* array2, Worker&& worker, Params&&... params)
  {
    // Check for null arrays and that both have same ValueType
    if (VTK_UNLIKELY(
          !array1 || !array2 || !vtkDataTypesCompare(array1->GetDataType(), array2->GetDataType())))
    {
      return false;
    }
    constexpr auto arrayPairHandlers =
      GetArrayPairHandlers<Known1List, Known2List, Worker, Params...>();
    const auto array1Index = array1->GetArrayType() * NumValueTypes + array1->GetDataType();
    const auto array2Index = array2->GetArrayType() * NumValueTypes + array2->GetDataType();
    const auto compactArray1Index = GetCompactArray1Index(array1Index);
    const auto compactArray2Index = GetCompactArray2Index(array2Index);
    if (compactArray1Index < NumKnownArrays1 && compactArray2Index < NumKnownArrays2)
    {
      ParamsTuple<Params...> paramsTuple{ std::forward<Params>(params)... };
      auto compactPairIndex = compactArray1Index * NumKnownArrays2 + compactArray2Index;
      return arrayPairHandlers[compactPairIndex](
        array1, array2, std::forward<Worker>(worker), std::move(paramsTuple));
    }
    // Fallback if unknown array types are present in Array1List or Array2List
    return ExecuteUnknown<Array1List, Array2List>(
      array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};

//------------------------------------------------------------------------------
// Description:
// Implementation of the 3 array dispatch mechanism.
template <typename ArrayList1, typename ArrayList2, typename ArrayList3>
struct Dispatch3;

// Recursive case:
template <typename Array1Head, typename Array1Tail, typename Array2Head, typename Array2Tail,
  typename Array3Head, typename Array3Tail>
struct Dispatch3<vtkTypeList::TypeList<Array1Head, Array1Tail>,
  vtkTypeList::TypeList<Array2Head, Array2Tail>, vtkTypeList::TypeList<Array3Head, Array3Tail>>
{
  using Array1List = vtkTypeList::TypeList<Array1Head, Array1Tail>;
  using Array2List = vtkTypeList::TypeList<Array2Head, Array2Tail>;
  using Array3List = vtkTypeList::TypeList<Array3Head, Array3Tail>;

  using Known1List = typename FilterArraysByArrayTypeTag<Array1List, KnownArrayTypeTagList>::Result;
  using Known2List = typename FilterArraysByArrayTypeTag<Array2List, KnownArrayTypeTagList>::Result;
  using Known3List = typename FilterArraysByArrayTypeTag<Array3List, KnownArrayTypeTagList>::Result;

  static constexpr int NumKnownArrays1 = vtkTypeList::Size<Known1List>::Result;
  static constexpr int NumKnownArrays2 = vtkTypeList::Size<Known2List>::Result;
  static constexpr int NumKnownArrays3 = vtkTypeList::Size<Known3List>::Result;
  static constexpr int NumKnownArrayTriples = NumKnownArrays1 * NumKnownArrays2 * NumKnownArrays3;

  // Use three separate index maps instead of one N^3 map
  static constexpr auto CompactArray1Map = ArrayListIndexMap::GetCompactArrayMap<Known1List>();
  static constexpr auto CompactArray2Map = ArrayListIndexMap::GetCompactArrayMap<Known2List>();
  static constexpr auto CompactArray3Map = ArrayListIndexMap::GetCompactArrayMap<Known3List>();

  static constexpr int GetCompactArray1Index(int arrayIndex)
  {
    return CompactArray1Map[arrayIndex];
  }
  static constexpr int GetCompactArray2Index(int arrayIndex)
  {
    return CompactArray2Map[arrayIndex];
  }
  static constexpr int GetCompactArray3Index(int arrayIndex)
  {
    return CompactArray3Map[arrayIndex];
  }

  template <typename... Params>
  using ParamsTuple = std::tuple<Params...>;

  template <typename Worker, typename... Params>
  using Dispatch3Function = bool (*)(vtkAbstractArray* array1, vtkAbstractArray* array2,
    vtkAbstractArray* array3, Worker&& worker, ParamsTuple<Params...> paramsTuple);

  // Unknown execution: try types from Array3List for a fixed Array1Type and Array2Type
  template <typename Array1Type, typename Array2Type, typename Array3ListT, typename Worker,
    typename... Params>
  static constexpr bool ExecuteUnknown3(Array1Type* array1, Array2Type* array2,
    vtkAbstractArray* array3, Worker&& worker, Params&&... params)
  {
    if constexpr (!std::is_same_v<Array3ListT, vtkTypeList::NullType>)
    {
      using Array3Type = typename Array3ListT::Head;
      if (auto typedArray3 = vtkArrayDownCast<Array3Type>(array3))
      {
        worker(array1, array2, typedArray3, std::forward<Params>(params)...);
        return true;
      }
      else
      {
        return ExecuteUnknown3<Array1Type, Array2Type, typename Array3ListT::Tail>(
          array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
      }
    }
    else
    {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
      vtkGenericWarningMacro("Triple array dispatch failed.");
#endif
      return false;
    }
  }

  // Unknown execution: try types from Array2List for a fixed Array1Type
  template <typename Array1Type, typename Array2ListT, typename Array3ListT, typename Worker,
    typename... Params>
  static constexpr bool ExecuteUnknown2(Array1Type* array1, vtkAbstractArray* array2,
    vtkAbstractArray* array3, Worker&& worker, Params&&... params)
  {
    if constexpr (!std::is_same_v<Array2ListT, vtkTypeList::NullType>)
    {
      using Array2Type = typename Array2ListT::Head;
      if (auto typedArray2 = vtkArrayDownCast<Array2Type>(array2))
      {
        return ExecuteUnknown3<Array1Type, Array2Type, Array3ListT>(array1, typedArray2, array3,
          std::forward<Worker>(worker), std::forward<Params>(params)...);
      }
      else
      {
        return ExecuteUnknown2<Array1Type, typename Array2ListT::Tail, Array3ListT>(
          array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
      }
    }
    else
    {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
      vtkGenericWarningMacro("Triple array dispatch failed.");
#endif
      return false;
    }
  }

  // Unknown execution: try types from Array1List
  template <typename Array1ListT, typename Array2ListT, typename Array3ListT, typename Worker,
    typename... Params>
  static constexpr bool ExecuteUnknown1(vtkAbstractArray* array1, vtkAbstractArray* array2,
    vtkAbstractArray* array3, Worker&& worker, Params&&... params)
  {
    if constexpr (!std::is_same_v<Array1ListT, vtkTypeList::NullType>)
    {
      using Array1Type = typename Array1ListT::Head;
      if (auto typedArray1 = vtkArrayDownCast<Array1Type>(array1))
      {
        return ExecuteUnknown2<Array1Type, Array2ListT, Array3ListT>(typedArray1, array2, array3,
          std::forward<Worker>(worker), std::forward<Params>(params)...);
      }
      else
      {
        return ExecuteUnknown1<typename Array1ListT::Tail, Array2ListT, Array3ListT>(
          array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
      }
    }
    else
    {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
      vtkGenericWarningMacro("Triple array dispatch failed.");
#endif
      return false;
    }
  }

  template <typename Array1ListT, typename Array2ListT, typename Array3ListT, typename Worker,
    typename... Params>
  static bool ExecuteUnknown(vtkAbstractArray* array1, vtkAbstractArray* array2,
    vtkAbstractArray* array3, Worker&& worker, Params&&... params)
  {
    return ExecuteUnknown1<Array1ListT, Array2ListT, Array3ListT>(
      array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }

  template <typename Array1Type, typename Array2Type, typename Array3Type, typename Worker,
    typename... Params>
  static constexpr bool ExecuteKnown(vtkAbstractArray* array1, vtkAbstractArray* array2,
    vtkAbstractArray* array3, Worker&& worker, ParamsTuple<Params...> paramsTuple)
  {
    std::apply([&worker, typedArray1 = static_cast<Array1Type*>(array1),
                 typedArray2 = static_cast<Array2Type*>(array2),
                 typedArray3 = static_cast<Array3Type*>(array3)](Params&&... forwardedParams)
      { worker(typedArray1, typedArray2, typedArray3, std::forward<Params>(forwardedParams)...); },
      std::move(paramsTuple));
    return true;
  }

  // Fill handlers: iterate Array3List for a fixed Array1Type and Array2Type
  template <typename Array1Type, typename Array2Type, typename Array3ListT, int CompactArray1Index,
    int CompactArray2Index, typename Worker, typename... Params>
  static constexpr void FillArray3(
    std::array<Dispatch3Function<Worker, Params...>, NumKnownArrayTriples>& arr)
  {
    if constexpr (!std::is_same_v<Array3ListT, vtkTypeList::NullType>)
    {
      using Array3Type = typename Array3ListT::Head;
      using Array3TypeTag = typename Array3Type::ArrayTypeTag;
      using Data3TypeTag = typename Array3Type::DataTypeTag;
      constexpr int array3TypeId = Array3TypeTag::value;
      constexpr int data3TypeId = Data3TypeTag::value;
      constexpr int array3Index = array3TypeId * NumValueTypes + data3TypeId;
      constexpr int compactArray3Index = GetCompactArray3Index(array3Index);
      constexpr int compactTripleIndex =
        (CompactArray1Index * NumKnownArrays2 + CompactArray2Index) * NumKnownArrays3 +
        compactArray3Index;
      arr[compactTripleIndex] =
        &ExecuteKnown<Array1Type, Array2Type, Array3Type, Worker, Params...>;
      // Recurse for remaining types
      FillArray3<Array1Type, Array2Type, typename Array3ListT::Tail, CompactArray1Index,
        CompactArray2Index, Worker, Params...>(arr);
    }
  }

  // Fill handlers: iterate Array2List for a fixed Array1Type
  template <typename Array1Type, typename Array2ListT, typename Array3ListT, int CompactArray1Index,
    typename Worker, typename... Params>
  static constexpr void FillArray2(
    std::array<Dispatch3Function<Worker, Params...>, NumKnownArrayTriples>& arr)
  {
    if constexpr (!std::is_same_v<Array2ListT, vtkTypeList::NullType>)
    {
      using Array2Type = typename Array2ListT::Head;
      using Array2TypeTag = typename Array2Type::ArrayTypeTag;
      using Data2TypeTag = typename Array2Type::DataTypeTag;
      constexpr int array2TypeId = Array2TypeTag::value;
      constexpr int data2TypeId = Data2TypeTag::value;
      constexpr int array2Index = array2TypeId * NumValueTypes + data2TypeId;
      constexpr int compactArray2Index = GetCompactArray2Index(array2Index);
      FillArray3<Array1Type, Array2Type, Array3ListT, CompactArray1Index, compactArray2Index,
        Worker, Params...>(arr);
      // Recurse for remaining types
      FillArray2<Array1Type, typename Array2ListT::Tail, Array3ListT, CompactArray1Index, Worker,
        Params...>(arr);
    }
  }

  // Fill handlers: iterate Array1List
  template <typename Array1ListT, typename Array2ListT, typename Array3ListT, typename Worker,
    typename... Params>
  static constexpr void FillArray1(
    std::array<Dispatch3Function<Worker, Params...>, NumKnownArrayTriples>& arr)
  {
    if constexpr (!std::is_same_v<Array1ListT, vtkTypeList::NullType>)
    {
      using Array1Type = typename Array1ListT::Head;
      using Array1TypeTag = typename Array1Type::ArrayTypeTag;
      using Data1TypeTag = typename Array1Type::DataTypeTag;
      constexpr int array1TypeId = Array1TypeTag::value;
      constexpr int data1TypeId = Data1TypeTag::value;
      constexpr int array1Index = array1TypeId * NumValueTypes + data1TypeId;
      constexpr int compactArray1Index = GetCompactArray1Index(array1Index);
      FillArray2<Array1Type, Array2ListT, Array3ListT, compactArray1Index, Worker, Params...>(arr);
      // Recurse for remaining types
      FillArray1<typename Array1ListT::Tail, Array2ListT, Array3ListT, Worker, Params...>(arr);
    }
  }

  template <typename Array1ListT, typename Array2ListT, typename Array3ListT, typename Worker,
    typename... Params>
  static constexpr void FillKnownArrayTripleHandlers(
    std::array<Dispatch3Function<Worker, Params...>, NumKnownArrayTriples>& arr)
  {
    FillArray1<Array1ListT, Array2ListT, Array3ListT, Worker, Params...>(arr);
  }

  template <typename Known1List, typename Known2List, typename Known3List, typename Worker,
    typename... Params>
  static constexpr std::array<Dispatch3Function<Worker, Params...>, NumKnownArrayTriples>
  GetArrayTripleHandlers()
  {
    std::array<Dispatch3Function<Worker, Params...>, NumKnownArrayTriples> arr{};
    if constexpr (NumKnownArrayTriples > 0)
    {
      FillKnownArrayTripleHandlers<Known1List, Known2List, Known3List, Worker, Params...>(arr);
    }
    return arr;
  }

  template <typename Worker, typename... Params>
  static bool Execute(vtkAbstractArray* array1, vtkAbstractArray* array2, vtkAbstractArray* array3,
    Worker&& worker, Params&&... params)
  {
    // Check for null arrays
    if (VTK_UNLIKELY(!array1 || !array2 || !array3))
    {
      return false;
    }
    constexpr auto arrayTripleHandlers =
      GetArrayTripleHandlers<Known1List, Known2List, Known3List, Worker, Params...>();
    const auto array1Index = array1->GetArrayType() * NumValueTypes + array1->GetDataType();
    const auto array2Index = array2->GetArrayType() * NumValueTypes + array2->GetDataType();
    const auto array3Index = array3->GetArrayType() * NumValueTypes + array3->GetDataType();
    const auto compactArray1Index = GetCompactArray1Index(array1Index);
    const auto compactArray2Index = GetCompactArray2Index(array2Index);
    const auto compactArray3Index = GetCompactArray3Index(array3Index);
    if (compactArray1Index < NumKnownArrays1 && compactArray2Index < NumKnownArrays2 &&
      compactArray3Index < NumKnownArrays3)
    {
      ParamsTuple<Params...> paramsTuple{ std::forward<Params>(params)... };
      auto compactTripleIndex =
        (compactArray1Index * NumKnownArrays2 + compactArray2Index) * NumKnownArrays3 +
        compactArray3Index;
      return arrayTripleHandlers[compactTripleIndex](
        array1, array2, array3, std::forward<Worker>(worker), std::move(paramsTuple));
    }
    // Fallback if unknown array types are present in Array1List, Array2List, or Array3List
    return ExecuteUnknown<Array1List, Array2List, Array3List>(
      array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};

//------------------------------------------------------------------------------
// Description:
// Implementation of the 3 array same-type dispatch mechanism.
template <typename ArrayList1, typename ArrayList2, typename ArrayList3>
struct Dispatch3Same;

// Recursive case:
template <typename Array1Head, typename Array1Tail, typename Array2Head, typename Array2Tail,
  typename Array3Head, typename Array3Tail>
struct Dispatch3Same<vtkTypeList::TypeList<Array1Head, Array1Tail>,
  vtkTypeList::TypeList<Array2Head, Array2Tail>, vtkTypeList::TypeList<Array3Head, Array3Tail>>
{
  using Array1List = vtkTypeList::TypeList<Array1Head, Array1Tail>;
  using Array2List = vtkTypeList::TypeList<Array2Head, Array2Tail>;
  using Array3List = vtkTypeList::TypeList<Array3Head, Array3Tail>;

  using Known1List = typename FilterArraysByArrayTypeTag<Array1List, KnownArrayTypeTagList>::Result;
  using Known2List = typename FilterArraysByArrayTypeTag<Array2List, KnownArrayTypeTagList>::Result;
  using Known3List = typename FilterArraysByArrayTypeTag<Array3List, KnownArrayTypeTagList>::Result;

  static constexpr int NumKnownArrays1 = vtkTypeList::Size<Known1List>::Result;
  static constexpr int NumKnownArrays2 = vtkTypeList::Size<Known2List>::Result;
  static constexpr int NumKnownArrays3 = vtkTypeList::Size<Known3List>::Result;
  static constexpr int NumKnownArrayTriples = NumKnownArrays1 * NumKnownArrays2 * NumKnownArrays3;

  // Use three separate index maps instead of one N^3 map
  static constexpr auto CompactArray1Map = ArrayListIndexMap::GetCompactArrayMap<Known1List>();
  static constexpr auto CompactArray2Map = ArrayListIndexMap::GetCompactArrayMap<Known2List>();
  static constexpr auto CompactArray3Map = ArrayListIndexMap::GetCompactArrayMap<Known3List>();

  static constexpr int GetCompactArray1Index(int arrayIndex)
  {
    return CompactArray1Map[arrayIndex];
  }
  static constexpr int GetCompactArray2Index(int arrayIndex)
  {
    return CompactArray2Map[arrayIndex];
  }
  static constexpr int GetCompactArray3Index(int arrayIndex)
  {
    return CompactArray3Map[arrayIndex];
  }

  template <typename... Params>
  using ParamsTuple = std::tuple<Params...>;

  template <typename Worker, typename... Params>
  using Dispatch3Function = bool (*)(vtkAbstractArray* array1, vtkAbstractArray* array2,
    vtkAbstractArray* array3, Worker&& worker, ParamsTuple<Params...> paramsTuple);

  // Unknown execution: try array3 types with fixed Array1Type and Array2Type,
  // but ensure same ValueType (filtering happens upstream).
  template <typename Array1Type, typename Array2Type, typename Array3ListT, typename Worker,
    typename... Params>
  static constexpr bool ExecuteUnknown3(Array1Type* array1, Array2Type* array2,
    vtkAbstractArray* array3, Worker&& worker, Params&&... params)
  {
    if constexpr (!std::is_same_v<Array3ListT, vtkTypeList::NullType>)
    {
      using Array3Type = typename Array3ListT::Head;
      if (auto typedArray3 = vtkArrayDownCast<Array3Type>(array3))
      {
        static_assert(
          std::is_same_v<typename Array1Type::DataTypeTag, typename Array2Type::DataTypeTag>,
          "Dispatch3Same requires same DataTypeTag across arrays");
        static_assert(
          std::is_same_v<typename Array1Type::DataTypeTag, typename Array3Type::DataTypeTag>,
          "Dispatch3Same requires same ValueType across arrays");
        worker(array1, array2, typedArray3, std::forward<Params>(params)...);
        return true;
      }
      else
      {
        return ExecuteUnknown3<Array1Type, Array2Type, typename Array3ListT::Tail>(
          array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
      }
    }
    else
    {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
      vtkGenericWarningMacro("Triple same-type array dispatch failed.");
#endif
      return false;
    }
  }

  // Unknown execution: try array2 types for a fixed Array1Type, but filter Array3List
  // to same ValueType as Array1Type.
  template <typename Array1Type, typename Array2ListT, typename Array3ListT, typename Worker,
    typename... Params>
  static constexpr bool ExecuteUnknown2(Array1Type* array1, vtkAbstractArray* array2,
    vtkAbstractArray* array3, Worker&& worker, Params&&... params)
  {
    if constexpr (!std::is_same_v<Array2ListT, vtkTypeList::NullType>)
    {
      using Array2Type = typename Array2ListT::Head;
      if (auto typedArray2 = vtkArrayDownCast<Array2Type>(array2))
      {
        // Filter Array3List to have the same ValueType as Array1Type
        using DataTypeTagList = vtkTypeList::Create<typename Array1Type::DataTypeTag>;
        using FilteredArray3List =
          typename FilterArraysByDataTypeTag<Array3ListT, DataTypeTagList>::Result;
        return ExecuteUnknown3<Array1Type, Array2Type, FilteredArray3List>(array1, typedArray2,
          array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
      }
      else
      {
        return ExecuteUnknown2<Array1Type, typename Array2ListT::Tail, Array3ListT>(
          array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
      }
    }
    else
    {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
      vtkGenericWarningMacro("Triple same-type array dispatch failed.");
#endif
      return false;
    }
  }

  // Unknown execution: try array1 types; when a typed array1 is found filter Array2List
  // and Array3List to matching ValueType.
  template <typename Array1ListT, typename Array2ListT, typename Array3ListT, typename Worker,
    typename... Params>
  static constexpr bool ExecuteUnknown1(vtkAbstractArray* array1, vtkAbstractArray* array2,
    vtkAbstractArray* array3, Worker&& worker, Params&&... params)
  {
    if constexpr (!std::is_same_v<Array1ListT, vtkTypeList::NullType>)
    {
      using Array1Type = typename Array1ListT::Head;
      if (auto typedArray1 = vtkArrayDownCast<Array1Type>(array1))
      {
        using DataTypeTagList = vtkTypeList::Create<typename Array1Type::DataTypeTag>;
        using FilteredArray2List =
          typename FilterArraysByDataTypeTag<Array2ListT, DataTypeTagList>::Result;
        using FilteredArray3List =
          typename FilterArraysByDataTypeTag<Array3ListT, DataTypeTagList>::Result;
        return ExecuteUnknown2<Array1Type, FilteredArray2List, FilteredArray3List>(typedArray1,
          array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
      }
      else
      {
        return ExecuteUnknown1<typename Array1ListT::Tail, Array2ListT, Array3ListT>(
          array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
      }
    }
    else
    {
#ifdef VTK_WARN_ON_DISPATCH_FAILURE
      vtkGenericWarningMacro("Triple same-type array dispatch failed.");
#endif
      return false;
    }
  }

  template <typename Array1ListT, typename Array2ListT, typename Array3ListT, typename Worker,
    typename... Params>
  static bool ExecuteUnknown(vtkAbstractArray* array1, vtkAbstractArray* array2,
    vtkAbstractArray* array3, Worker&& worker, Params&&... params)
  {
    return ExecuteUnknown1<Array1ListT, Array2ListT, Array3ListT>(
      array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }

  template <typename Array1Type, typename Array2Type, typename Array3Type, typename Worker,
    typename... Params>
  static constexpr bool ExecuteKnown(vtkAbstractArray* array1, vtkAbstractArray* array2,
    vtkAbstractArray* array3, Worker&& worker, ParamsTuple<Params...> paramsTuple)
  {
    static_assert(
      std::is_same_v<typename Array1Type::DataTypeTag, typename Array2Type::DataTypeTag>,
      "Array types must have the same DataTypeTag for Dispatch3Same");
    static_assert(
      std::is_same_v<typename Array1Type::DataTypeTag, typename Array3Type::DataTypeTag>,
      "Array types must have the same DataTypeTag for Dispatch3Same");

    std::apply([&worker, typedArray1 = static_cast<Array1Type*>(array1),
                 typedArray2 = static_cast<Array2Type*>(array2),
                 typedArray3 = static_cast<Array3Type*>(array3)](Params&&... forwardedParams)
      { worker(typedArray1, typedArray2, typedArray3, std::forward<Params>(forwardedParams)...); },
      std::move(paramsTuple));
    return true;
  }

  // Fill handlers: iterate Array3List for a fixed Array1Type and Array2Type,
  // but only include Array3 entries that match the ValueType of Array1Type.
  template <typename Array1Type, typename Array2Type, typename Array3ListT, int CompactArray1Index,
    int CompactArray2Index, typename Worker, typename... Params>
  static constexpr void FillArray3(
    std::array<Dispatch3Function<Worker, Params...>, NumKnownArrayTriples>& arr)
  {
    if constexpr (!std::is_same_v<Array3ListT, vtkTypeList::NullType>)
    {
      using Array3Type = typename Array3ListT::Head;
      using Array3TypeTag = typename Array3Type::ArrayTypeTag;
      using Data3TypeTag = typename Array3Type::DataTypeTag;
      constexpr int array3TypeId = Array3TypeTag::value;
      constexpr int data3TypeId = Data3TypeTag::value;
      constexpr int array3Index = array3TypeId * NumValueTypes + data3TypeId;
      constexpr int compactArray3Index = GetCompactArray3Index(array3Index);
      constexpr int compactTripleIndex =
        (CompactArray1Index * NumKnownArrays2 + CompactArray2Index) * NumKnownArrays3 +
        compactArray3Index;
      arr[compactTripleIndex] =
        &ExecuteKnown<Array1Type, Array2Type, Array3Type, Worker, Params...>;
      // Recurse for remaining types
      FillArray3<Array1Type, Array2Type, typename Array3ListT::Tail, CompactArray1Index,
        CompactArray2Index, Worker, Params...>(arr);
    }
  }

  // Fill handlers: iterate Array2List for a fixed Array1Type, but filter Array2List
  // to the same ValueType as Array1Type.
  template <typename Array1Type, typename Array2ListT, typename Array3ListT, int CompactArray1Index,
    typename Worker, typename... Params>
  static constexpr void FillArray2(
    std::array<Dispatch3Function<Worker, Params...>, NumKnownArrayTriples>& arr)
  {
    if constexpr (!std::is_same_v<Array2ListT, vtkTypeList::NullType>)
    {
      using Array2Type = typename Array2ListT::Head;
      using Array2TypeTag = typename Array2Type::ArrayTypeTag;
      using Data2TypeTag = typename Array2Type::DataTypeTag;
      constexpr int array2TypeId = Array2TypeTag::value;
      constexpr int data2TypeId = Data2TypeTag::value;
      constexpr int array2Index = array2TypeId * NumValueTypes + data2TypeId;
      constexpr int compactArray2Index = GetCompactArray2Index(array2Index);
      // Filter Array3List to only include arrays with same ValueType as Array1Type.
      using Data2TypeTagList = vtkTypeList::Create<typename Array1Type::DataTypeTag>;
      using FilteredArray3List =
        typename FilterArraysByDataTypeTag<Array3ListT, Data2TypeTagList>::Result;
      FillArray3<Array1Type, Array2Type, FilteredArray3List, CompactArray1Index, compactArray2Index,
        Worker, Params...>(arr);
      // Recurse for remaining types
      FillArray2<Array1Type, typename Array2ListT::Tail, Array3ListT, CompactArray1Index, Worker,
        Params...>(arr);
    }
  }

  // Fill handlers: iterate Array1List and filter Known2List / Known3List by ValueType
  template <typename Array1ListT, typename Array2ListT, typename Array3ListT, typename Worker,
    typename... Params>
  static constexpr void FillArray1(
    std::array<Dispatch3Function<Worker, Params...>, NumKnownArrayTriples>& arr)
  {
    if constexpr (!std::is_same_v<Array1ListT, vtkTypeList::NullType>)
    {
      using Array1Type = typename Array1ListT::Head;
      using Array1TypeTag = typename Array1Type::ArrayTypeTag;
      using Data1TypeTag = typename Array1Type::DataTypeTag;
      constexpr int array1TypeId = Array1TypeTag::value;
      constexpr int data1TypeId = Data1TypeTag::value;
      constexpr int array1Index = array1TypeId * NumValueTypes + data1TypeId;
      constexpr int compactArray1Index = GetCompactArray1Index(array1Index);
      // Filter Array2List and Array3List to only include arrays with same ValueType as Array1Type.
      using DataTypeTagList = vtkTypeList::Create<typename Array1Type::DataTypeTag>;
      using FilteredArray2List =
        typename FilterArraysByDataTypeTag<Array2ListT, DataTypeTagList>::Result;
      using FilteredArray3List =
        typename FilterArraysByDataTypeTag<Array3ListT, DataTypeTagList>::Result;
      FillArray2<Array1Type, FilteredArray2List, FilteredArray3List, compactArray1Index, Worker,
        Params...>(arr);
      // Recurse for remaining types
      FillArray1<typename Array1ListT::Tail, Array2ListT, Array3ListT, Worker, Params...>(arr);
    }
  }

  template <typename Array1ListT, typename Array2ListT, typename Array3ListT, typename Worker,
    typename... Params>
  static constexpr void FillKnownArrayTripleHandlers(
    std::array<Dispatch3Function<Worker, Params...>, NumKnownArrayTriples>& arr)
  {
    FillArray1<Array1ListT, Array2ListT, Array3ListT, Worker, Params...>(arr);
  }

  template <typename Known1List, typename Known2List, typename Known3List, typename Worker,
    typename... Params>
  static constexpr std::array<Dispatch3Function<Worker, Params...>, NumKnownArrayTriples>
  GetArrayTripleHandlers()
  {
    std::array<Dispatch3Function<Worker, Params...>, NumKnownArrayTriples> arr{};
    if constexpr (NumKnownArrayTriples > 0)
    {
      FillKnownArrayTripleHandlers<Known1List, Known2List, Known3List, Worker, Params...>(arr);
    }
    return arr;
  }

  template <typename Worker, typename... Params>
  static bool Execute(vtkAbstractArray* array1, vtkAbstractArray* array2, vtkAbstractArray* array3,
    Worker&& worker, Params&&... params)
  {
    // Check for null arrays and that all have the same ValueType
    if (VTK_UNLIKELY(!array1 || !array2 || !array3 ||
          !vtkDataTypesCompare(array1->GetDataType(), array2->GetDataType()) ||
          !vtkDataTypesCompare(array1->GetDataType(), array3->GetDataType())))
    {
      return false;
    }
    constexpr auto arrayTripleHandlers =
      GetArrayTripleHandlers<Known1List, Known2List, Known3List, Worker, Params...>();
    const auto array1Index = array1->GetArrayType() * NumValueTypes + array1->GetDataType();
    const auto array2Index = array2->GetArrayType() * NumValueTypes + array2->GetDataType();
    const auto array3Index = array3->GetArrayType() * NumValueTypes + array3->GetDataType();
    const auto compactArray1Index = GetCompactArray1Index(array1Index);
    const auto compactArray2Index = GetCompactArray2Index(array2Index);
    const auto compactArray3Index = GetCompactArray3Index(array3Index);
    if (compactArray1Index < NumKnownArrays1 && compactArray2Index < NumKnownArrays2 &&
      compactArray3Index < NumKnownArrays3)
    {
      ParamsTuple<Params...> paramsTuple{ std::forward<Params>(params)... };
      auto compactTripleIndex =
        (compactArray1Index * NumKnownArrays2 + compactArray2Index) * NumKnownArrays3 +
        compactArray3Index;
      return arrayTripleHandlers[compactTripleIndex](
        array1, array2, array3, std::forward<Worker>(worker), std::move(paramsTuple));
    }
    // Fallback if unknown array types are present in Array1List, Array2List, or Array3List
    return ExecuteUnknown<Array1List, Array2List, Array3List>(
      array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};

VTK_ABI_NAMESPACE_END
} // end namespace impl

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
// FilterArraysByArrayTypeTag implementation:
//------------------------------------------------------------------------------

// Terminal case:
template <typename ArrayTypeTagList>
struct FilterArraysByArrayTypeTag<vtkTypeList::NullType, ArrayTypeTagList>
{
  typedef vtkTypeList::NullType Result;
};

// Recursive case:
template <typename ArrayHead, typename ArrayTail, typename ArrayTypeTagList>
struct FilterArraysByArrayTypeTag<vtkTypeList::TypeList<ArrayHead, ArrayTail>, ArrayTypeTagList>
{
private:
  using ArrayTypeTag = typename ArrayHead::ArrayTypeTag;
  enum
  {
    ArrayTagIsAllowed = vtkTypeList::IndexOf<ArrayTypeTagList, ArrayTypeTag>::Result >= 0
  };
  using NewTail = typename FilterArraysByArrayTypeTag<ArrayTail, ArrayTypeTagList>::Result;

public:
  using Result = typename vtkTypeList::Select<ArrayTagIsAllowed,
    vtkTypeList::TypeList<ArrayHead, NewTail>, NewTail>::Result;
};

//------------------------------------------------------------------------------
// FilterArraysByValueTypeTag implementation:
//------------------------------------------------------------------------------

// Terminal case:
template <typename DataTypeTagList>
struct FilterArraysByDataTypeTag<vtkTypeList::NullType, DataTypeTagList>
{
  typedef vtkTypeList::NullType Result;
};

// Recursive case:
template <typename ArrayHead, typename ArrayTail, typename DataTypeTagList>
struct FilterArraysByDataTypeTag<vtkTypeList::TypeList<ArrayHead, ArrayTail>, DataTypeTagList>
{
private:
  using DataTypeTag = typename ArrayHead::DataTypeTag;
  enum
  {
    DataTagIsAllowed = vtkTypeList::IndexOf<DataTypeTagList, DataTypeTag>::Result >= 0
  };
  using NewTail = typename FilterArraysByDataTypeTag<ArrayTail, DataTypeTagList>::Result;

public:
  using Result = typename vtkTypeList::Select<DataTagIsAllowed,
    vtkTypeList::TypeList<ArrayHead, NewTail>, NewTail>::Result;
};

//------------------------------------------------------------------------------
// FilterArraysByValueType implementation:
//------------------------------------------------------------------------------

// Terminal case:
template <typename ValueList>
struct FilterArraysByValueType<vtkTypeList::NullType, ValueList>
{
  using Result = vtkTypeList::NullType;
};

// Recursive case:
template <typename ArrayHead, typename ArrayTail, typename ValueList>
struct FilterArraysByValueType<vtkTypeList::TypeList<ArrayHead, ArrayTail>, ValueList>
{
private:
  using ValueType = typename ArrayHead::ValueType;
  enum
  {
    ValueIsAllowed = vtkTypeList::IndexOf<ValueList, ValueType>::Result >= 0
  };
  using NewTail = typename FilterArraysByValueType<ArrayTail, ValueList>::Result;

public:
  using Result = typename vtkTypeList::Select<ValueIsAllowed,
    vtkTypeList::TypeList<ArrayHead, NewTail>, NewTail>::Result;
};

//------------------------------------------------------------------------------
// DispatchByArray implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch:
template <typename ArrayHead, typename ArrayTail>
struct DispatchByArray<vtkTypeList::TypeList<ArrayHead, ArrayTail>>
{
private:
  using ArrayList = vtkTypeList::TypeList<ArrayHead, ArrayTail>;
  using UniqueArrays = typename vtkTypeList::Unique<ArrayList>::Result;
  using SortedUniqueArrays = typename vtkTypeList::DerivedToFront<UniqueArrays>::Result;
  using ArrayDispatcher = impl::Dispatch<SortedUniqueArrays>;

public:
  template <typename Worker, typename... Params>
  static bool Execute(vtkAbstractArray* inArray, Worker&& worker, Params&&... params)
  {
    return ArrayDispatcher::Execute(
      inArray, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};

//------------------------------------------------------------------------------
// Dispatch implementation:
// (defined after DispatchByArray to prevent 'incomplete type' errors)
//------------------------------------------------------------------------------
struct Dispatch : public DispatchByArray<Arrays>
{
};

//------------------------------------------------------------------------------
// DispatchByArrayAndValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch
template <typename ArrayList, typename ValueTypeHead, typename ValueTypeTail>
struct DispatchByArrayAndValueType<ArrayList, vtkTypeList::TypeList<ValueTypeHead, ValueTypeTail>>
{
private:
  using ValueTypeList = vtkTypeList::TypeList<ValueTypeHead, ValueTypeTail>;
  using FilteredArrayList = typename FilterArraysByValueType<ArrayList, ValueTypeList>::Result;
  using UniqueArrays = typename vtkTypeList::Unique<FilteredArrayList>::Result;
  using SortedUniqueArrays = typename vtkTypeList::DerivedToFront<UniqueArrays>::Result;
  using ArrayDispatcher = impl::Dispatch<SortedUniqueArrays>;

public:
  template <typename Worker, typename... Params>
  static bool Execute(vtkAbstractArray* inArray, Worker&& worker, Params&&... params)
  {
    return ArrayDispatcher::Execute(
      inArray, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};
//------------------------------------------------------------------------------
// DispatchByValueType implementation:
//------------------------------------------------------------------------------
template <typename ValueTypeList>
struct DispatchByValueType : public DispatchByArrayAndValueType<Arrays, ValueTypeList>
{
};
//------------------------------------------------------------------------------
// DispatchByValueTypeUsingArrays implementation:
//------------------------------------------------------------------------------
template <typename ArrayList, typename ValueTypeList>
struct DispatchByValueTypeUsingArrays : public DispatchByArrayAndValueType<ArrayList, ValueTypeList>
{
};

//------------------------------------------------------------------------------
// Dispatch2ByArray implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch2:
template <typename ArrayList1, typename ArrayList2>
struct Dispatch2ByArray
{
private:
  using UniqueArray1 = typename vtkTypeList::Unique<ArrayList1>::Result;
  using UniqueArray2 = typename vtkTypeList::Unique<ArrayList2>::Result;
  using SortedUniqueArray1 = typename vtkTypeList::DerivedToFront<UniqueArray1>::Result;
  using SortedUniqueArray2 = typename vtkTypeList::DerivedToFront<UniqueArray2>::Result;
  using ArrayDispatcher = impl::Dispatch2<SortedUniqueArray1, SortedUniqueArray2>;

public:
  template <typename Worker, typename... Params>
  static bool Execute(
    vtkAbstractArray* array1, vtkAbstractArray* array2, Worker&& worker, Params&&... params)
  {
    return ArrayDispatcher::Execute(
      array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};

//------------------------------------------------------------------------------
// Dispatch2 implementation:
//------------------------------------------------------------------------------
struct Dispatch2 : public Dispatch2ByArray<Arrays, Arrays>
{
};

//------------------------------------------------------------------------------
// Dispatch2ByArrayAndValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch2
template <typename ArrayList1, typename ArrayList2, typename ValueTypeList1,
  typename ValueTypeList2>
struct Dispatch2ByArrayAndValueType
{
private:
  using FilteredArrayList1 = typename FilterArraysByValueType<ArrayList1, ValueTypeList1>::Result;
  using FilteredArrayList2 = typename FilterArraysByValueType<ArrayList2, ValueTypeList2>::Result;
  using UniqueArray1 = typename vtkTypeList::Unique<FilteredArrayList1>::Result;
  using UniqueArray2 = typename vtkTypeList::Unique<FilteredArrayList2>::Result;
  using SortedUniqueArray1 = typename vtkTypeList::DerivedToFront<UniqueArray1>::Result;
  using SortedUniqueArray2 = typename vtkTypeList::DerivedToFront<UniqueArray2>::Result;
  using ArrayDispatcher = impl::Dispatch2<SortedUniqueArray1, SortedUniqueArray2>;

public:
  template <typename Worker, typename... Params>
  static bool Execute(
    vtkAbstractArray* array1, vtkAbstractArray* array2, Worker&& worker, Params&&... params)
  {
    return ArrayDispatcher::Execute(
      array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};
//------------------------------------------------------------------------------
// Dispatch2ByValueType implementation:
//------------------------------------------------------------------------------
template <typename ValueTypeList1, typename ValueTypeList2>
struct Dispatch2ByValueType
  : Dispatch2ByArrayAndValueType<Arrays, Arrays, ValueTypeList1, ValueTypeList2>
{
};
//------------------------------------------------------------------------------
// Dispatch2ByValueTypeUsingArrays implementation:
//------------------------------------------------------------------------------
template <typename ArrayList, typename ValueTypeList1, typename ValueTypeList2>
struct Dispatch2ByValueTypeUsingArrays
  : Dispatch2ByArrayAndValueType<ArrayList, ArrayList, ValueTypeList1, ValueTypeList2>
{
};

//------------------------------------------------------------------------------
// Dispatch2ByArrayAndSameValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch2Same
template <typename ArrayList, typename ValueTypeList>
struct Dispatch2ByArrayAndSameValueType
{
private:
  using FilteredArrayList = typename FilterArraysByValueType<ArrayList, ValueTypeList>::Result;
  using UniqueArray = typename vtkTypeList::Unique<FilteredArrayList>::Result;
  using SortedUniqueArray = typename vtkTypeList::DerivedToFront<UniqueArray>::Result;
  using Dispatcher = impl::Dispatch2Same<SortedUniqueArray, SortedUniqueArray>;

public:
  template <typename Worker, typename... Params>
  static bool Execute(
    vtkAbstractArray* array1, vtkAbstractArray* array2, Worker&& worker, Params&&... params)
  {
    return Dispatcher::Execute(
      array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};
//------------------------------------------------------------------------------
// Dispatch2BySameValueType implementation:
//------------------------------------------------------------------------------
template <typename ValueTypeList>
struct Dispatch2BySameValueType : Dispatch2ByArrayAndSameValueType<Arrays, ValueTypeList>
{
};
//------------------------------------------------------------------------------
// Dispatch2BySameValueTypeUsingArrays implementation:
//------------------------------------------------------------------------------
template <typename ArrayList, typename ValueTypeList>
struct Dispatch2BySameValueTypeUsingArrays
  : Dispatch2ByArrayAndSameValueType<ArrayList, ValueTypeList>
{
};

//------------------------------------------------------------------------------
// Dispatch2ByArrayWithSameValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch2Same
template <typename ArrayList1, typename ArrayList2>
struct Dispatch2ByArrayWithSameValueType
{
private:
  using UniqueArray1 = typename vtkTypeList::Unique<ArrayList1>::Result;
  using UniqueArray2 = typename vtkTypeList::Unique<ArrayList2>::Result;
  using SortedUniqueArray1 = typename vtkTypeList::DerivedToFront<UniqueArray1>::Result;
  using SortedUniqueArray2 = typename vtkTypeList::DerivedToFront<UniqueArray2>::Result;
  using Dispatcher = impl::Dispatch2Same<SortedUniqueArray1, SortedUniqueArray2>;

public:
  template <typename Worker, typename... Params>
  static bool Execute(
    vtkAbstractArray* array1, vtkAbstractArray* array2, Worker&& worker, Params&&... params)
  {
    return Dispatcher::Execute(
      array1, array2, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};

//------------------------------------------------------------------------------
// Dispatch2SameValueType implementation:
//------------------------------------------------------------------------------
struct Dispatch2SameValueType : public Dispatch2ByArrayWithSameValueType<Arrays, Arrays>
{
};
//------------------------------------------------------------------------------
// Dispatch2SameValueTypeUsingArrays implementation:
//------------------------------------------------------------------------------
template <typename ArrayList>
struct Dispatch2SameValueTypeUsingArrays
  : public Dispatch2ByArrayWithSameValueType<ArrayList, ArrayList>
{
};

//------------------------------------------------------------------------------
// Dispatch3ByArray implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch3:
template <typename ArrayList1, typename ArrayList2, typename ArrayList3>
struct Dispatch3ByArray
{
private:
  using UniqueArray1 = typename vtkTypeList::Unique<ArrayList1>::Result;
  using UniqueArray2 = typename vtkTypeList::Unique<ArrayList2>::Result;
  using UniqueArray3 = typename vtkTypeList::Unique<ArrayList3>::Result;
  using SortedUniqueArray1 = typename vtkTypeList::DerivedToFront<UniqueArray1>::Result;
  using SortedUniqueArray2 = typename vtkTypeList::DerivedToFront<UniqueArray2>::Result;
  using SortedUniqueArray3 = typename vtkTypeList::DerivedToFront<UniqueArray3>::Result;
  using ArrayDispatcher =
    impl::Dispatch3<SortedUniqueArray1, SortedUniqueArray2, SortedUniqueArray3>;

public:
  template <typename Worker, typename... Params>
  static bool Execute(vtkAbstractArray* array1, vtkAbstractArray* array2, vtkAbstractArray* array3,
    Worker&& worker, Params&&... params)
  {
    return ArrayDispatcher::Execute(
      array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};

//------------------------------------------------------------------------------
// Dispatch3 implementation:
//------------------------------------------------------------------------------
struct Dispatch3 : public Dispatch3ByArray<Arrays, Arrays, Arrays>
{
};

//------------------------------------------------------------------------------
// Dispatch3ByArrayAndValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch3
template <typename ArrayList1, typename ArrayList2, typename ArrayList3, typename ValueTypeList1,
  typename ValueTypeList2, typename ValueTypeList3>
struct Dispatch3ByArrayAndValueType
{
private:
  using FilteredArrayList1 = typename FilterArraysByValueType<ArrayList1, ValueTypeList1>::Result;
  using FilteredArrayList2 = typename FilterArraysByValueType<ArrayList2, ValueTypeList2>::Result;
  using FilteredArrayList3 = typename FilterArraysByValueType<ArrayList3, ValueTypeList3>::Result;
  using UniqueArray1 = typename vtkTypeList::Unique<FilteredArrayList1>::Result;
  using UniqueArray2 = typename vtkTypeList::Unique<FilteredArrayList2>::Result;
  using UniqueArray3 = typename vtkTypeList::Unique<FilteredArrayList3>::Result;
  using SortedUniqueArray1 = typename vtkTypeList::DerivedToFront<UniqueArray1>::Result;
  using SortedUniqueArray2 = typename vtkTypeList::DerivedToFront<UniqueArray2>::Result;
  using SortedUniqueArray3 = typename vtkTypeList::DerivedToFront<UniqueArray3>::Result;
  using ArrayDispatcher =
    impl::Dispatch3<SortedUniqueArray1, SortedUniqueArray2, SortedUniqueArray3>;

public:
  template <typename Worker, typename... Params>
  static bool Execute(vtkAbstractArray* array1, vtkAbstractArray* array2, vtkAbstractArray* array3,
    Worker&& worker, Params&&... params)
  {
    return ArrayDispatcher::Execute(
      array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};
//------------------------------------------------------------------------------
// Dispatch3ByValueType implementation:
//------------------------------------------------------------------------------
template <typename ValueTypeList1, typename ValueTypeList2, typename ValueTypeList3>
struct Dispatch3ByValueType
  : public Dispatch3ByArrayAndValueType<Arrays, Arrays, Arrays, ValueTypeList1, ValueTypeList2,
      ValueTypeList3>
{
};
//------------------------------------------------------------------------------
// Dispatch3ByValueTypeUsingArrays implementation:
//------------------------------------------------------------------------------
template <typename ArrayList, typename ValueTypeList1, typename ValueTypeList2,
  typename ValueTypeList3>
struct Dispatch3ByValueTypeUsingArrays
  : public Dispatch3ByArrayAndValueType<ArrayList, ArrayList, ArrayList, ValueTypeList1,
      ValueTypeList2, ValueTypeList3>
{
};

//------------------------------------------------------------------------------
// Dispatch3ByArraySameValue implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch3Same
template <typename ArrayList, typename ValueTypeList>
struct Dispatch3ByArraySameValue
{
private:
  using FilteredArrayList = typename FilterArraysByValueType<ArrayList, ValueTypeList>::Result;
  using UniqueArray = typename vtkTypeList::Unique<FilteredArrayList>::Result;
  using SortedUniqueArray = typename vtkTypeList::DerivedToFront<UniqueArray>::Result;
  using Dispatcher = impl::Dispatch3Same<SortedUniqueArray, SortedUniqueArray, SortedUniqueArray>;

public:
  template <typename Worker, typename... Params>
  static bool Execute(vtkAbstractArray* array1, vtkAbstractArray* array2, vtkAbstractArray* array3,
    Worker&& worker, Params&&... params)
  {
    return Dispatcher::Execute(
      array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};
//------------------------------------------------------------------------------
// Dispatch3BySameValueType implementation:
//------------------------------------------------------------------------------
template <typename ValueTypeList>
struct Dispatch3BySameValueType : public Dispatch3ByArraySameValue<Arrays, ValueTypeList>
{
};
//------------------------------------------------------------------------------
// Dispatch3BySameValueTypeUsingArrays implementation:
//------------------------------------------------------------------------------
template <typename ArrayList, typename ValueTypeList>
struct Dispatch3BySameValueTypeUsingArrays : public Dispatch3ByArraySameValue<Arrays, ValueTypeList>
{
};

//------------------------------------------------------------------------------
// Dispatch3BySameValueType implementation:
//------------------------------------------------------------------------------
// Preprocess and pass off to impl::Dispatch3Same
template <typename ArrayList1, typename ArrayList2, typename ArrayList3>
struct Dispatch3ByArrayWithSameValueType
{
private:
  using UniqueArray1 = typename vtkTypeList::Unique<ArrayList1>::Result;
  using UniqueArray2 = typename vtkTypeList::Unique<ArrayList2>::Result;
  using UniqueArray3 = typename vtkTypeList::Unique<ArrayList3>::Result;
  using SortedUniqueArray1 = typename vtkTypeList::DerivedToFront<UniqueArray1>::Result;
  using SortedUniqueArray2 = typename vtkTypeList::DerivedToFront<UniqueArray2>::Result;
  using SortedUniqueArray3 = typename vtkTypeList::DerivedToFront<UniqueArray3>::Result;
  using Dispatcher =
    impl::Dispatch3Same<SortedUniqueArray1, SortedUniqueArray2, SortedUniqueArray3>;

public:
  template <typename Worker, typename... Params>
  static bool Execute(vtkAbstractArray* array1, vtkAbstractArray* array2, vtkAbstractArray* array3,
    Worker&& worker, Params&&... params)
  {
    return Dispatcher::Execute(
      array1, array2, array3, std::forward<Worker>(worker), std::forward<Params>(params)...);
  }
};

//------------------------------------------------------------------------------
// Dispatch3SameValueTypeUsingArrays implementation:
//------------------------------------------------------------------------------
template <typename ArrayList>
struct Dispatch3SameValueTypeUsingArrays
  : public Dispatch3ByArrayWithSameValueType<ArrayList, ArrayList, ArrayList>
{
};
//------------------------------------------------------------------------------
// Dispatch3SameValueType implementation:
//------------------------------------------------------------------------------
struct Dispatch3SameValueType : Dispatch3ByArrayWithSameValueType<Arrays, Arrays, Arrays>
{
};

VTK_ABI_NAMESPACE_END
} // end namespace vtkArrayDispatch

#endif // vtkArrayDispatch_txx
