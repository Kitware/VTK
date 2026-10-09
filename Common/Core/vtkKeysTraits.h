// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @file   vtkKeysTraits.h
 * @brief  the traits and checks of the concepts of vtk::keys, see vtkKeys.h
 *
 * The type traits of the inputs and elements of vtkKeys.h (their KeyType and ElementIdType), the
 * detection traits of the methods of its concepts, and the checks that report what to implement,
 * in vtk::keys::detail.
 */

#ifndef vtkKeysTraits_h
#define vtkKeysTraits_h

#include "vtkABINamespace.h"

#include <cstdint>
#include <type_traits>
#include <utility>

namespace vtk
{
namespace keys
{
VTK_ABI_NAMESPACE_BEGIN
template <typename TKey, typename TElementId, typename TKeyId>
class KeysBase;
VTK_ABI_NAMESPACE_END

namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

// The type traits of the declarations of vtkKeys.h.

// Input and InputMapper: their number of records.
template <typename In, typename = void>
struct HasGetNumberOfRecords : std::false_type
{
};
template <typename In>
struct HasGetNumberOfRecords<In,
  std::enable_if_t<
    std::is_integral_v<std::decay_t<decltype(std::declval<const In&>().GetNumberOfRecords())>>>>
  : std::true_type
{
};

// The type of the records of an input, as it gives their number.
template <typename In>
using RecordIdOf = std::decay_t<decltype(std::declval<const In&>().GetNumberOfRecords())>;

// The key and element id types of an input: its KeyType and ElementIdType, if declared,
// otherwise the type of GetKey() and RecordIdType, or void.
template <typename In, typename = void>
struct GetKeyTypeOf
{
  using type = void;
};
template <typename In>
struct GetKeyTypeOf<In,
  std::void_t<decltype(std::declval<const In&>().GetKey(std::declval<RecordIdOf<In>>()))>>
{
  using type =
    std::decay_t<decltype(std::declval<const In&>().GetKey(std::declval<RecordIdOf<In>>()))>;
};
template <typename In, typename = void>
struct KeyTypeOf : GetKeyTypeOf<In>
{
};
template <typename In>
struct KeyTypeOf<In, std::void_t<typename In::KeyType>>
{
  using type = typename In::KeyType;
};
template <typename In>
using KeyTypeOfT = typename KeyTypeOf<In>::type;

template <typename In, typename = void>
struct RecordIdTypeOf
{
  using type = void;
};
template <typename In>
struct RecordIdTypeOf<In, std::enable_if_t<HasGetNumberOfRecords<In>::value>>
{
  using type = RecordIdOf<In>;
};
template <typename In, typename = void>
struct ElementIdTypeOf : RecordIdTypeOf<In>
{
};
template <typename In>
struct ElementIdTypeOf<In, std::void_t<typename In::ElementIdType>>
{
  using type = typename In::ElementIdType;
};
template <typename In>
using ElementIdTypeOfT = typename ElementIdTypeOf<In>::type;

// The key type of an element of an array.
template <typename T>
using ElementKeyOf = std::decay_t<decltype(std::declval<const T&>().GetKey())>;

//------------------------------------------------------------------------------
// The checks of the concepts, in the order of their first use by vtkKeys.txx, each detection trait
// right before its first check. Each check reports what to implement, and returns whether it
// passed, so that a caller can skip, with `if constexpr`, the code that would only add errors to
// the reported one.

// InputMapper: CountElements(), by which it is recognized.
template <typename In, typename ElementId, typename = void>
struct HasCountElements : std::false_type
{
};
template <typename In, typename ElementId>
struct HasCountElements<In, ElementId,
  std::void_t<decltype(std::declval<const In&>().CountElements(std::declval<RecordIdOf<In>>(),
    std::declval<RecordIdOf<In>>(), std::declval<ElementId*>()))>> : std::true_type
{
};

// The InputMapper of CountElements().
template <typename InputMapper>
constexpr bool ValidateCountElements()
{
  constexpr bool isMapper = HasGetNumberOfRecords<InputMapper>::value &&
    HasCountElements<InputMapper, ElementIdTypeOfT<InputMapper>>::value;
  static_assert(isMapper,
    "CountElements() requires an InputMapper: using ElementIdType, RecordIdType "
    "GetNumberOfRecords() const, void CountElements(RecordIdType beginRecord, RecordIdType "
    "endRecord, ElementIdType* counts) const");
  return isMapper;
}

// Output: SetElement() with the values of an element.
template <typename Out, typename ElementId, typename Void, typename... Values>
struct HasSetElementImpl : std::false_type
{
};
template <typename Out, typename ElementId, typename... Values>
struct HasSetElementImpl<Out, ElementId,
  std::void_t<decltype(std::declval<Out&>().SetElement(
    std::declval<ElementId>(), std::declval<Values>()...))>,
  Values...> : std::true_type
{
};
template <typename Out, typename ElementId, typename... Values>
using HasSetElement = HasSetElementImpl<Out, ElementId, void, Values...>;

// SetElement() of the Output, with the values of an element.
template <typename Output, typename ElementIdType, typename... Values>
constexpr bool ValidateSetElement()
{
  constexpr bool hasSetElement = HasSetElement<Output, ElementIdType, Values...>::value;
  static_assert(hasSetElement,
    "Output must implement: SetElement(ElementIdType position, RecordIdType record) for an "
    "Input, or SetElement(ElementIdType position, RecordIdType record, ElementIdType index) for "
    "an InputMapper");
  return hasSetElement;
}

// Input: GetKey().
template <typename In, typename Key, typename = void>
struct HasGetKey : std::false_type
{
};
template <typename In, typename Key>
struct HasGetKey<In, Key,
  std::enable_if_t<std::is_convertible_v<
    decltype(std::declval<const In&>().GetKey(std::declval<RecordIdOf<In>>())), Key>>>
  : std::true_type
{
};

// InputMapper: WriteKeys().
template <typename In, typename ElementId, typename Key, typename = void>
struct HasWriteKeys : std::false_type
{
};
template <typename In, typename ElementId, typename Key>
struct HasWriteKeys<In, ElementId, Key,
  std::void_t<decltype(std::declval<const In&>().WriteKeys(std::declval<RecordIdOf<In>>(),
    std::declval<RecordIdOf<In>>(), std::declval<const ElementId*>(), std::declval<Key*>()))>>
  : std::true_type
{
};

// Output: SetNumberOfElements().
template <typename Out, typename ElementId, typename = void>
struct HasSetNumberOfElements : std::false_type
{
};
template <typename Out, typename ElementId>
struct HasSetNumberOfElements<Out, ElementId,
  std::void_t<decltype(std::declval<Out&>().SetNumberOfElements(std::declval<ElementId>()))>>
  : std::true_type
{
};

// The Input or InputMapper, and the Output, of a scatter. Every check needs the previous ones.
template <typename Input, typename Output>
constexpr bool ValidateScatter()
{
  using KeyType = KeyTypeOfT<Input>;
  using ElementIdType = ElementIdTypeOfT<Input>;
  constexpr bool hasRecords = HasGetNumberOfRecords<Input>::value;
  static_assert(hasRecords, "Input must implement: RecordIdType GetNumberOfRecords() const");
  if constexpr (hasRecords)
  {
    constexpr bool hasTypes = !std::is_void_v<KeyType> && std::is_integral_v<ElementIdType>;
    static_assert(hasTypes,
      "Input must implement KeyType GetKey(RecordIdType record) const (Input), or declare a "
      "KeyType and an integer ElementIdType (InputMapper)");
    if constexpr (hasTypes)
    {
      constexpr bool isInput = HasGetKey<Input, KeyType>::value;
      constexpr bool isMapper = HasCountElements<Input, ElementIdType>::value;
      static_assert(isInput || isMapper,
        "Input must implement KeyType GetKey(RecordIdType record) const (Input), or void "
        "CountElements(RecordIdType beginRecord, RecordIdType endRecord, ElementIdType* counts) "
        "const (InputMapper)");
      static_assert(!(isInput && isMapper), "Input is both an Input and an InputMapper");
      constexpr bool hasWriteKeys = !isMapper || HasWriteKeys<Input, ElementIdType, KeyType>::value;
      static_assert(hasWriteKeys,
        "InputMapper must implement: void WriteKeys(RecordIdType beginRecord, RecordIdType "
        "endRecord, const ElementIdType* offsets, KeyType* keys) const");
      constexpr bool hasSize = HasSetNumberOfElements<Output, ElementIdType>::value;
      static_assert(
        hasSize, "Output must implement: void SetNumberOfElements(ElementIdType numberOfElements)");
      return isInput != isMapper && hasWriteKeys && hasSize;
    }
  }
  return false;
}

// The keys of ScatterDense(): integers, indexed by their difference with the minimum key.
template <typename KeyType>
constexpr bool ValidateDenseKey()
{
  constexpr bool isKey = std::is_integral_v<KeyType> && !std::is_same_v<KeyType, bool>;
  static_assert(
    isKey, "The keys of ScatterDense() must be integers: use ScatterSparse() otherwise");
  return isKey;
}

// An array of elements to scatter.
template <typename Element, typename ElementIdType>
constexpr bool ValidateArray()
{
  static_assert(
    std::is_integral_v<ElementIdType>, "The number of elements must be of an integer type");
  static_assert(std::is_copy_assignable_v<Element>, "The elements must be copy assignable");
  return std::is_integral_v<ElementIdType> && std::is_copy_assignable_v<Element>;
}

// Element and sparse KeyType: operator<.
template <typename T, typename = void>
struct HasLess : std::false_type
{
};
template <typename T>
struct HasLess<T,
  std::enable_if_t<
    std::is_convertible_v<decltype(std::declval<const T&>() < std::declval<const T&>()), bool>>>
  : std::true_type
{
};

// The keys of ScatterSparse(): sorted, and stored, by value.
template <typename KeyType>
constexpr bool ValidateSparseKey()
{
  constexpr bool isKey = HasLess<KeyType>::value && std::is_default_constructible_v<KeyType> &&
    std::is_copy_assignable_v<KeyType>;
  static_assert(isKey,
    "The keys of ScatterSparse() must be default constructible, copy assignable, and implement "
    "operator<");
  return isKey;
}

// Functors: the optional Initialize() and Reduce() of a vtkSMPTools functor.
template <typename P, typename = void>
struct HasInitialize : std::false_type
{
};
template <typename P>
struct HasInitialize<P, std::void_t<decltype(std::declval<P&>().Initialize())>> : std::true_type
{
};

template <typename P, typename = void>
struct HasReduce : std::false_type
{
};
template <typename P>
struct HasReduce<P, std::void_t<decltype(std::declval<P&>().Reduce())>> : std::true_type
{
};

// Keys: whether a type derives from KeysBase, e.g., DenseKeys, SparseKeys and their reductions.
template <typename TKey, typename TElementId, typename TKeyId>
std::true_type IsKeysImpl(const KeysBase<TKey, TElementId, TKeyId>*);
std::false_type IsKeysImpl(...);
template <typename T>
using IsKeys = decltype(IsKeysImpl(std::declval<const T*>()));

// Keys: DenseKeys, SparseKeys or their reductions.
template <typename Keys>
constexpr bool ValidateKeys()
{
  static_assert(IsKeys<Keys>::value,
    "The keys must be DenseKeys or SparseKeys, as returned by ScatterDense() or ScatterSparse()");
  return IsKeys<Keys>::value;
}

// The functor of ForEachKeyRange().
template <typename Functor, typename Output, typename Keys, typename... Args>
constexpr bool ValidateKeyRangeFunctor()
{
  constexpr bool isFunctor = std::is_invocable_v<Functor&, Output&, typename Keys::KeyIdType,
    typename Keys::KeyIdType, const typename Keys::ElementIdType*, Args...>;
  static_assert(isFunctor,
    "The functor of ForEachKeyRange() must implement: operator()(Output& output, KeyIdType "
    "beginKeyId, KeyIdType "
    "endKeyId, const ElementIdType* offsets, args...)");
  return isFunctor;
}

// The reduce functor of Reduce().
template <typename Functor, typename Output, typename Keys>
constexpr bool ValidateReduceFunctor()
{
  constexpr bool isFunctor = std::is_invocable_v<Functor&, Output&, typename Keys::KeyIdType,
    typename Keys::KeyIdType, const typename Keys::ElementIdType*, typename Keys::ElementIdType*>;
  static_assert(isFunctor,
    "The reduce functor of Reduce() must implement: void operator()(Output& output, KeyIdType "
    "beginKeyId, KeyIdType endKeyId, const ElementIdType* offsets, ElementIdType* counts)");
  return isFunctor;
}

// Output: CopyData(), for Compact().
template <typename Out, typename ElementId, typename = void>
struct HasCopyData : std::false_type
{
};
template <typename Out, typename ElementId>
struct HasCopyData<Out, ElementId,
  std::void_t<decltype(std::declval<Out&>().CopyData(std::declval<const Out&>(),
    std::declval<ElementId>(), std::declval<ElementId>(), std::declval<ElementId>()))>>
  : std::true_type
{
};

// The Output of Compact().
template <typename Output, typename ElementIdType>
constexpr bool ValidateCompactOutput()
{
  constexpr bool isOutput = HasSetNumberOfElements<Output, ElementIdType>::value &&
    HasCopyData<Output, ElementIdType>::value;
  static_assert(isOutput,
    "The Output of Compact() must implement: void SetNumberOfElements(ElementIdType), and void "
    "CopyData(const Output& source, ElementIdType destinationStart, ElementIdType count, "
    "ElementIdType sourceStart)");
  return isOutput;
}

// Element: operator==.
template <typename T, typename = void>
struct HasEqual : std::false_type
{
};
template <typename T>
struct HasEqual<T,
  std::enable_if_t<
    std::is_convertible_v<decltype(std::declval<const T&>() == std::declval<const T&>()), bool>>>
  : std::true_type
{
};

// The elements of SortUnique.
template <typename Element>
constexpr bool ValidateSortUnique()
{
  constexpr bool isElement = HasLess<Element>::value && HasEqual<Element>::value;
  static_assert(isElement, "The elements of SortUnique must implement: operator< and operator==");
  return isElement;
}

// The group functor of Group().
template <typename Functor, typename Output, typename Keys>
constexpr bool ValidateGroupFunctor()
{
  constexpr bool isFunctor = std::is_invocable_v<Functor&, Output&, typename Keys::KeyIdType,
    typename Keys::KeyIdType, const typename Keys::ElementIdType*, uint8_t*>;
  static_assert(isFunctor,
    "The group functor of Group() must implement: void operator()(Output& output, KeyIdType "
    "beginKeyId, KeyIdType endKeyId, const ElementIdType* offsets, uint8_t* groupStarts)");
  return isFunctor;
}

// Element: operator!=.
template <typename T, typename = void>
struct HasNotEqual : std::false_type
{
};
template <typename T>
struct HasNotEqual<T,
  std::enable_if_t<
    std::is_convertible_v<decltype(std::declval<const T&>() != std::declval<const T&>()), bool>>>
  : std::true_type
{
};

// The elements of SortGroup.
template <typename Element>
constexpr bool ValidateSortGroup()
{
  constexpr bool isElement = HasLess<Element>::value && HasNotEqual<Element>::value;
  static_assert(isElement, "The elements of SortGroup must implement: operator< and operator!=");
  return isElement;
}

VTK_ABI_NAMESPACE_END
} // namespace detail
} // namespace keys
} // namespace vtk

#endif
// VTK-HeaderTest-Exclude: vtkKeysTraits.h
