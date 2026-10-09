// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @file   vtkKeys.h
 * @brief  parallel scatter of elements by key, and operations on the elements of every key
 *
 * vtk::keys moves elements, e.g., the faces of the cells of a mesh, into the order of their keys,
 * e.g., their smallest point id, in parallel, so that the elements of a key are contiguous and
 * processed together. As in Viskores, the data is separate from the operations:
 * - ScatterDense() moves the elements to an Output in key order with a counting sort over the key
 *   range, for dense keys, e.g., point ids, and returns their DenseKeys: every key of the range,
 *   with the first element of every key;
 * - ScatterSparse() does it with a sort, for sparse keys, e.g., hashes or pairs of point ids, and
 *   returns their SparseKeys: the distinct keys, with the first element of every key;
 * - Reduce() keeps the surviving elements of every key at its front, e.g., the faces used once (a
 *   segmented compaction), and returns their ReducedKeys;
 * - Group() makes the equal elements of every key contiguous, e.g., the faces shared by cells (a
 *   segmented run-length encoding), and returns their GroupedElements and the ReducedKeys of the
 *   groups;
 * - Compact() copies the surviving elements, or the first element of every group, contiguously.
 * Reduce() and Group() work on both kinds of keys, and only reorder the elements of every key,
 * with a reduce or group functor called on ranges of key ids, see ForEachKeyRange(). A key id is
 * the position of a key: in the key range for DenseKeys, among the distinct keys for SparseKeys.
 * Every result holds its own arrays, and can be released alone.
 *
 * A *record* is what the input is iterated over, e.g., a cell, and an *element* what is scattered:
 * the record (1-1), or one of its elements (1-M), e.g., a face of a cell. The operations take
 * objects of the concepts below, or arrays of elements that provide their key (Element concept).
 *
 * @code{.cpp}
 * // The unique edges, by their smallest point id.
 * std::vector<Edge> scattered(edges.size());
 * auto keys = vtk::keys::ScatterDense(
 *   edges.data(), numberOfEdges, 0, numberOfPoints - 1, scattered.data());
 * auto unique = vtk::keys::Reduce(keys, scattered.data(), vtk::keys::SortUnique{});
 * std::vector<Edge> uniqueEdges(unique.GetNumberOfElements());
 * vtk::keys::Compact(keys, unique, scattered.data(), uniqueEdges.data());
 * // The unique edges of key id k are at [unique.GetOffset(k), unique.GetOffset(k + 1)).
 * @endcode
 *
 * The dense scatter is the atomic histogram, prefix scan and atomic scatter of DP-Hash-Count, as
 * Kokkos::BinSort, and the sparse one is the sort by key of Viskores's Keys. The operations process
 * every key as a segment, as reduce_by_key in Thrust or WorkletReduceByKey in Viskores.
 *
 * @warning
 * DenseKeys hold (maxKey - minKey) + 1 keys: use them for dense keys only. ScatterSparse() keeps
 * the elements of every key in the order of their records. ScatterDense() fills every key from its
 * front, as vtkStaticCellLinksTemplate: in the order of their records with the sequential backend,
 * but in an order that depends on thread scheduling with the others, so that equal elements that
 * carry different data can be ordered differently from one run to the next, unless the elements of
 * every key are sorted, as vtkStaticCellLinksTemplate does.
 *
 * @sa
 * vtkStaticElementLocator vtkUnstructuredGridSides
 */

#ifndef vtkKeys_h
#define vtkKeys_h

#include "vtkABINamespace.h"
#include "vtkKeysTraits.h" // For the checks of the concepts
#include "vtkType.h"       // For vtkIdType

#include <cstdint>
#include <memory>
#include <type_traits>

//------------------------------------------------------------------------------
// Concepts, checked at compile time, see vtkKeysTraits.h.
//
// Input: records that are the elements (1-1).
//   RecordIdType GetNumberOfRecords() const;
//   KeyType GetKey(RecordIdType record) const; // called concurrently
//   // Optional, otherwise the type of GetKey() and RecordIdType.
//   using KeyType = ...;
//   using ElementIdType = ...;
//
// InputMapper: records mapped to several elements (1-M), e.g., cells to their faces.
//   using KeyType = ...;
//   using ElementIdType = ...; // must hold the number of elements
//   RecordIdType GetNumberOfRecords() const;
//   // The number of elements of record beginRecord + i, to counts[i].
//   void CountElements(RecordIdType beginRecord, RecordIdType endRecord,
//     ElementIdType* counts) const;
//   // The keys of the elements of every record, to keys[offsets[record]], ..., in order.
//   void WriteKeys(RecordIdType beginRecord, RecordIdType endRecord,
//     const ElementIdType* offsets, KeyType* keys) const;
//   The methods are called concurrently on disjoint ranges of records, so that they can set up
//   their scratch data once per range.
//
// Output: the scattered elements, in one array or in several (a structure of arrays).
//   void SetNumberOfElements(ElementIdType numberOfElements); // 0 first, to release
//   // Called concurrently, at distinct positions, with the record of an Input, or the record
//   // and the index of the element in its record of an InputMapper, from which the Output reads
//   // what it needs, as Viskores's ExternalFaces worklet does.
//   void SetElement(ElementIdType position, RecordIdType record);
//   void SetElement(ElementIdType position, RecordIdType record, ElementIdType index);
//   // For Compact() only, as vtkDataSetAttributes::CopyData().
//   void CopyData(const Output& source, ElementIdType destinationStart, ElementIdType count,
//     ElementIdType sourceStart);
//
// KeyType: an integer type for ScatterDense(); for ScatterSparse(), any type that is default
//   constructible, copy assignable and ordered by operator<, e.g., std::pair<vtkIdType, vtkIdType>.
//
// Element: an element of an array, copy assignable, with
//   KeyType GetKey() const; // equal elements have the same key
//   and, for SortUnique and SortGroup, operator<, operator== and operator!=. The ElementIdType of
//   an array is the type of its number of elements.
//
// Keys: the result of ScatterDense() or ScatterSparse(), a DenseKeys or a SparseKeys, or their
//   reductions, which derive from KeysBase, all that the operations use.
//
// Functors of ForEachKeyRange(), Reduce() and Group(): callables on a range of key ids of the
// Output, called concurrently on disjoint ranges, see ForEachKeyRange():
//   // Reduce functor: keep the survivors of key id k at its front, their number to counts[k].
//   void operator()(Output& output, KeyIdType beginKeyId, KeyIdType endKeyId,
//     const ElementIdType* offsets, ElementIdType* counts);
//   // Group functor: make the equal elements of every key contiguous, and set groupStarts[i] to 1
//   // for every element i that differs from the previous one.
//   void operator()(Output& output, KeyIdType beginKeyId, KeyIdType endKeyId,
//     const ElementIdType* offsets, uint8_t* groupStarts);
//   As vtkSMPTools functors, they can have an Initialize() and a Reduce(), to keep their scratch
//   data or their results per thread.
namespace vtk
{
namespace keys
{
namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

// The writers of the results, defined in vtkKeys.txx.
struct DenseScatterer;
struct SparseScatterer;
struct Reducer;
struct Grouper;

VTK_ABI_NAMESPACE_END
} // namespace detail

VTK_ABI_NAMESPACE_BEGIN

/**
 * What DenseKeys and SparseKeys share, and all that Reduce(), Group(), Compact() and
 * ForEachKeyRange() use: the first element of every key id, the elements of key id k being at
 * [GetOffset(k), GetOffset(k + 1)) of the Output. KeyIdType is the type of the key ids.
 */
template <typename TKey, typename TElementId, typename TKeyId>
class KeysBase
{
public:
  using KeyType = TKey;
  using ElementIdType = TElementId;
  using KeyIdType = TKeyId;

  static_assert(std::is_integral_v<ElementIdType>, "ElementIdType must be an integer type");

  vtkIdType GetNumberOfRecords() const { return this->NumberOfRecords; }
  vtkIdType GetNumberOfElements() const { return this->NumberOfElements; }
  KeyIdType GetNumberOfKeys() const { return this->NumberOfKeys; }

  /**
   * The first element of a key id, 0 <= keyId <= GetNumberOfKeys().
   */
  ElementIdType GetOffset(KeyIdType keyId) const { return this->Offsets[keyId]; }

  /**
   * The first element of every key id, GetNumberOfKeys() + 1 values, nullptr without element.
   */
  const ElementIdType* GetOffsets() const { return this->Offsets.get(); }

  ElementIdType GetNumberOfElementsOfKey(KeyIdType keyId) const
  {
    return this->Offsets[keyId + 1] - this->Offsets[keyId];
  }

protected:
  void ResetBase()
  {
    this->NumberOfRecords = this->NumberOfElements = 0;
    this->NumberOfKeys = 0;
    this->Offsets.reset();
  }

  // The records and the keys of other keys, e.g., of a reduction, without their offsets.
  void CopyBaseFrom(const KeysBase& other)
  {
    this->NumberOfRecords = other.NumberOfRecords;
    this->NumberOfKeys = other.NumberOfKeys;
  }

  vtkIdType NumberOfRecords = 0;
  vtkIdType NumberOfElements = 0;
  KeyIdType NumberOfKeys = 0;
  std::unique_ptr<ElementIdType[]> Offsets; // NumberOfKeys + 1
};

/**
 * The dense keys of the elements, returned by ScatterDense(): every key of the key range
 * [GetMinKey(), GetMaxKey()], present or not, whose key id is its position in the range. The
 * number of keys, plus one, must fit KeyType, the type of the key ids.
 */
template <typename TKey = vtkIdType, typename TElementId = vtkIdType>
class DenseKeys : public KeysBase<TKey, TElementId, TKey>
{
public:
  using KeyType = TKey;
  using KeyIdType = TKey;

  static_assert(std::is_integral_v<KeyType> && !std::is_same_v<KeyType, bool>,
    "The KeyType of DenseKeys must be an integer type: a key id is its difference with the "
    "minimum key");

  KeyType GetMinKey() const { return this->MinKey; }
  KeyType GetMaxKey() const { return this->MaxKey; }

  /**
   * The key of a key id, and the key id of a key of [GetMinKey(), GetMaxKey()].
   */
  KeyType GetKey(KeyIdType keyId) const { return this->MinKey + keyId; }
  KeyIdType GetKeyId(KeyType key) const { return DenseKeys::GetKeyId(key, this->MinKey); }

  /**
   * Release the memory.
   */
  void Reset()
  {
    this->ResetBase();
    this->MinKey = this->MaxKey = 0;
  }

private:
  friend struct detail::DenseScatterer;
  friend struct detail::Reducer;
  friend struct detail::Grouper;

  // The key id of a key given the minimum key, kept in a register by the loops of ScatterDense().
  static KeyIdType GetKeyId(KeyType key, KeyType minKey) { return key - minKey; }

  // The keys of other keys of this type, e.g., of a reduction.
  void CopyKeysFrom(const DenseKeys& other)
  {
    this->CopyBaseFrom(other);
    this->MinKey = other.MinKey;
    this->MaxKey = other.MaxKey;
  }

  KeyType MinKey = 0;
  KeyType MaxKey = 0;
};

/**
 * The sparse keys of the elements, returned by ScatterSparse(): the distinct keys, in increasing
 * order of their operator<, whose key id is their position. The keys can be of any type ordered by
 * operator<, e.g., a std::pair of point ids for edges. The distinct keys are shared with the
 * reductions of these keys.
 */
template <typename TKey = vtkIdType, typename TElementId = vtkIdType>
class SparseKeys : public KeysBase<TKey, TElementId, TElementId>
{
public:
  using KeyType = TKey;
  using KeyIdType = TElementId;

  static_assert(std::is_signed_v<TElementId>, "ElementIdType must be signed, see FindKeyId()");

  /**
   * The key of a key id, and the key id of a key, -1 if absent, by a binary search.
   */
  KeyType GetKey(KeyIdType keyId) const { return this->Keys[keyId]; }
  KeyIdType FindKeyId(KeyType key) const;

  /**
   * Release the memory, the distinct keys once no reduction shares them.
   */
  void Reset()
  {
    this->ResetBase();
    this->Keys.reset();
  }

private:
  friend struct detail::SparseScatterer;
  friend struct detail::Reducer;
  friend struct detail::Grouper;

  // The keys of other keys of this type, e.g., of a reduction, which share the distinct keys.
  void CopyKeysFrom(const SparseKeys& other)
  {
    this->CopyBaseFrom(other);
    this->Keys = other.Keys;
  }

  std::shared_ptr<KeyType[]> Keys; // NumberOfKeys
};

/**
 * The keys of the scatters of an input, with its KeyType and ElementIdType, and of an array.
 */
template <typename Input>
using DenseKeysOf = DenseKeys<detail::KeyTypeOfT<Input>, detail::ElementIdTypeOfT<Input>>;
template <typename Input>
using SparseKeysOf = SparseKeys<detail::KeyTypeOfT<Input>, detail::ElementIdTypeOfT<Input>>;
template <typename Element, typename ElementIdType>
using DenseArrayKeys = DenseKeys<detail::ElementKeyOf<Element>, ElementIdType>;
template <typename Element, typename ElementIdType>
using SparseArrayKeys = SparseKeys<detail::ElementKeyOf<Element>, ElementIdType>;

//------------------------------------------------------------------------------
// ScatterDense() and ScatterSparse(): the elements are moved to the Output in key order.

/**
 * The number of elements of an InputMapper, without storing them, e.g., to choose its
 * ElementIdType. The elements of a record must fit it, not their total.
 */
template <typename InputMapper>
vtkIdType CountElements(const InputMapper& mapper);

/**
 * Scatter the elements of an Input or an InputMapper to an Output by a counting sort over the key
 * range, with the KeyType and ElementIdType of the input. The key range, [minKey, maxKey], saves a
 * pass if given; every key must lie in it.
 *
 * @return The keys, empty if the elements do not fit their ElementIdType.
 */
template <typename Input, typename Output>
DenseKeysOf<Input> ScatterDense(const Input& input, Output& output);
template <typename Input, typename Output>
DenseKeysOf<Input> ScatterDense(const Input& input, detail::KeyTypeOfT<Input> minKey,
  detail::KeyTypeOfT<Input> maxKey, Output& output);

/**
 * Scatter an array of elements, keyed by their GetKey() (Element concept), to `scattered`, which
 * holds as many elements and does not overlap `elements`. The ElementIdType of the keys is the type
 * of numberOfElements.
 */
template <typename Element, typename ElementIdType>
DenseArrayKeys<Element, ElementIdType> ScatterDense(
  const Element* elements, ElementIdType numberOfElements, Element* scattered);
template <typename Element, typename ElementIdType>
DenseArrayKeys<Element, ElementIdType> ScatterDense(const Element* elements,
  ElementIdType numberOfElements, detail::ElementKeyOf<Element> minKey,
  detail::ElementKeyOf<Element> maxKey, Element* scattered);

/**
 * Scatter the elements of an Input or an InputMapper, or an array of elements, to an Output by a
 * parallel sort of their keys, for keys too sparse for ScatterDense(). One key and two ids per
 * element are allocated, and released before returning.
 *
 * @return The keys, empty if the elements do not fit their ElementIdType.
 */
template <typename Input, typename Output>
SparseKeysOf<Input> ScatterSparse(const Input& input, Output& output);
template <typename Element, typename ElementIdType>
SparseArrayKeys<Element, ElementIdType> ScatterSparse(
  const Element* elements, ElementIdType numberOfElements, Element* scattered);

//------------------------------------------------------------------------------
// ForEachKeyRange(), Reduce() and Group(): the elements of every key are processed.

/**
 * Call functor(output, beginKeyId, endKeyId, offsets, args...) on disjoint ranges of key ids,
 * concurrently, on the same functor, where the elements of key id k are
 * [offsets[k], offsets[k + 1]) of the Output, for DenseKeys or SparseKeys. The functor's
 * Initialize() and Reduce(), if any, are called as the ones of a vtkSMPTools functor.
 */
template <typename TKeys, typename Output, typename Functor, typename... Args>
void ForEachKeyRange(const TKeys& keys, Output&& output, Functor& functor, Args... args);

//------------------------------------------------------------------------------
// Reduce(): the surviving elements of every key, e.g., the faces used once.

/**
 * The keys of the surviving elements of Reduce(), or of the groups of Group(): the keys of the
 * scattered keys, with the scanned survivors of every key id as offsets, i.e., the keys of the
 * Output once compacted. Before Compact(), the survivors of key id k are at [keys.GetOffset(k),
 * keys.GetOffset(k) + reduced.GetNumberOfElementsOfKey(k)) of the scattered Output. It is a
 * distinct type, so that the arguments of Compact() cannot be swapped.
 */
template <typename TKeys>
class ReducedKeys : public TKeys
{
};

/**
 * Keep the surviving elements of every key of the Output at its front with a reduce functor called
 * as functor(output, beginKeyId, endKeyId, offsets, counts), which writes the survivors of key id k
 * to counts[k], see ForEachKeyRange(). The Output is any Output, or an array of elements. Only one
 * Reduce() or Group() can be applied to an Output.
 */
template <typename TKeys, typename Output, typename Functor>
ReducedKeys<TKeys> Reduce(const TKeys& keys, Output&& output, Functor&& functor);

/**
 * Copy the survivors of the reduced Output `input` contiguously into `output`, another Output that
 * implements CopyData(), or arrays of elements.
 *
 * @return The number of elements copied.
 */
template <typename TKeys, typename Output, typename = std::enable_if_t<!std::is_pointer_v<Output>>>
typename TKeys::ElementIdType Compact(
  const TKeys& keys, const ReducedKeys<TKeys>& reduced, const Output& input, Output& output);
template <typename TKeys, typename Element>
typename TKeys::ElementIdType Compact(
  const TKeys& keys, const ReducedKeys<TKeys>& reduced, const Element* input, Element* output);

/**
 * The reduce functor of an array of elements that keeps one element of every set of equal ones,
 * with operator< and operator==: it sorts the elements of every key, then applies std::unique().
 */
struct SortUnique
{
  template <typename Element, typename KeyIdType, typename ElementIdType>
  void operator()(Element* elements, KeyIdType beginKeyId, KeyIdType endKeyId,
    const ElementIdType* offsets, ElementIdType* counts) const;
};

//------------------------------------------------------------------------------
// Group(): the groups of equal elements of every key, e.g., the faces shared by cells.

/**
 * The groups of equal elements of Group(), ordered by key: the elements of group g are at
 * [GetGroupOffset(g), GetGroupOffset(g + 1)) of the grouped Output.
 */
template <typename TKeys>
class GroupedElements
{
public:
  using ElementIdType = typename TKeys::ElementIdType;

  ElementIdType GetNumberOfGroups() const { return this->NumberOfGroups; }

  /**
   * The first element of a group, 0 <= group <= GetNumberOfGroups().
   */
  ElementIdType GetGroupOffset(ElementIdType group) const { return this->GroupOffsets[group]; }

  /**
   * The first element of every group, GetNumberOfGroups() + 1 values, nullptr without grouping.
   */
  const ElementIdType* GetGroupOffsets() const { return this->GroupOffsets.get(); }

  ElementIdType GetGroupSize(ElementIdType group) const
  {
    return this->GroupOffsets[group + 1] - this->GroupOffsets[group];
  }

  /**
   * Release the memory.
   */
  void Reset()
  {
    this->NumberOfGroups = 0;
    this->GroupOffsets.reset();
  }

private:
  friend struct detail::Grouper;

  ElementIdType NumberOfGroups = 0;
  std::unique_ptr<ElementIdType[]> GroupOffsets; // NumberOfGroups + 1
};

/**
 * The result of Group(): the groups, and the keys of the groups, i.e., the groups of key id k are
 * [Keys.GetOffset(k), Keys.GetOffset(k + 1)), which are also the keys of the first elements of the
 * groups once compacted. Keep only what is needed, e.g.:
 * @code{.cpp}
 * auto groups = vtk::keys::Group(keys, output, functor).Groups; // the keys of the groups released
 * auto groupKeys = vtk::keys::Group(keys, output, functor).Keys; // the groups released
 * auto grouping = vtk::keys::Group(keys, output, functor); // both
 * @endcode
 * A structured binding, e.g., `auto [groups, groupKeys]`, keeps both until the end of its scope.
 */
template <typename TKeys>
struct Grouping
{
  GroupedElements<TKeys> Groups;
  ReducedKeys<TKeys> Keys;
};

/**
 * Group the equal elements of every key of the Output with a group functor called as
 * functor(output, beginKeyId, endKeyId, offsets, groupStarts), which makes them contiguous and sets
 * groupStarts[i] to 1 for every element i that differs from the previous one, see
 * ForEachKeyRange(). The Output is any Output, or an array of elements. One byte per element is
 * allocated, and released before returning. Only one Reduce() or Group() can be applied to an
 * Output.
 */
template <typename TKeys, typename Output, typename Functor>
Grouping<TKeys> Group(const TKeys& keys, Output&& output, Functor&& functor);

/**
 * Copy the first element of every group of the grouped Output `input` to `output`, another Output
 * that implements CopyData(), or arrays of elements, so that group g is element g of `output`.
 *
 * @return The number of elements copied.
 */
template <typename TKeys, typename Output, typename = std::enable_if_t<!std::is_pointer_v<Output>>>
typename TKeys::ElementIdType Compact(
  const GroupedElements<TKeys>& groups, const Output& input, Output& output);
template <typename TKeys, typename Element>
typename TKeys::ElementIdType Compact(
  const GroupedElements<TKeys>& groups, const Element* input, Element* output);

/**
 * The group functor of an array of elements, with operator< and operator!=: it sorts the elements
 * of every key, then flags the elements that differ from the previous one.
 */
struct SortGroup
{
  template <typename Element, typename KeyIdType, typename ElementIdType>
  void operator()(Element* elements, KeyIdType beginKeyId, KeyIdType endKeyId,
    const ElementIdType* offsets, uint8_t* groupStarts) const;
};

VTK_ABI_NAMESPACE_END
} // namespace keys
} // namespace vtk

#include "vtkKeys.txx"

#endif
// VTK-HeaderTest-Exclude: vtkKeys.h
