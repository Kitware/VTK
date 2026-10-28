// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkKeys_txx
#define vtkKeys_txx

#include "vtkKeys.h"

#include "vtkLogger.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

namespace vtk
{
namespace keys
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// Count the elements of the records in chunks of a buffer on the stack, and sum them per thread.
template <typename InputMapper>
vtkIdType CountElements(const InputMapper& mapper)
{
  detail::ValidateCountElements<InputMapper>();
  using ElementIdType = detail::ElementIdTypeOfT<InputMapper>;
  using RecordId = detail::RecordIdOf<InputMapper>;
  static constexpr vtkIdType ChunkSize = 1024;
  vtkSMPThreadLocal<vtkIdType> threadTotal(0);
  vtkSMPTools::For(0, static_cast<vtkIdType>(mapper.GetNumberOfRecords()),
    [&mapper, &threadTotal](vtkIdType begin, vtkIdType end)
    {
      ElementIdType counts[ChunkSize];
      vtkIdType& total = threadTotal.Local();
      for (vtkIdType chunk = begin; chunk < end; chunk += ChunkSize)
      {
        const vtkIdType chunkEnd = std::min(chunk + ChunkSize, end);
        mapper.CountElements(static_cast<RecordId>(chunk), static_cast<RecordId>(chunkEnd), counts);
        for (vtkIdType i = 0; i < chunkEnd - chunk; ++i)
        {
          total += counts[i];
        }
      }
    });
  vtkIdType total = 0;
  for (vtkIdType threadCount : threadTotal)
  {
    total += threadCount;
  }
  return total;
}

VTK_ABI_NAMESPACE_END

namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// What ScatterDense() and ScatterSparse() share: the mapping of the records of an InputMapper,
// whose elements are counted into their offsets, scanned, and their keys written, unless their
// number does not fit ElementIdType (MapRecords()), and SetElement().
struct Scatterer
{
  // The elements of the records of an InputMapper: the first element of every record
  // (numberOfRecords + 1 values), and the key of every element.
  template <typename TKeys>
  struct MappedRecords
  {
    std::unique_ptr<typename TKeys::ElementIdType[]> Offsets;
    std::unique_ptr<typename TKeys::KeyType[]> Keys;
    typename TKeys::ElementIdType NumberOfElements = 0;
  };

  // Count the elements of every record, and sum them per thread while their counts are in cache.
  template <typename TKeys, typename InputMapper>
  static vtkIdType CountRecordElements(
    const InputMapper& mapper, typename TKeys::ElementIdType* counts)
  {
    using RecordId = RecordIdOf<InputMapper>;
    const auto numRecords = static_cast<vtkIdType>(mapper.GetNumberOfRecords());
    vtkSMPThreadLocal<vtkIdType> threadTotal(0);
    vtkSMPTools::For(0, numRecords,
      [&mapper, counts, &threadTotal](vtkIdType begin, vtkIdType end)
      {
        mapper.CountElements(
          static_cast<RecordId>(begin), static_cast<RecordId>(end), counts + begin);
        vtkIdType& total = threadTotal.Local();
        for (vtkIdType record = begin; record < end; ++record)
        {
          total += counts[record];
        }
      });
    counts[numRecords] = 0;
    vtkIdType total = 0;
    for (vtkIdType threadCount : threadTotal)
    {
      total += threadCount;
    }
    return total;
  }

  // Count the elements of the records into their offsets, scan them, and write their keys, or fail
  // if their number does not fit ElementIdType.
  template <typename TKeys, typename InputMapper>
  static bool MapRecords(const InputMapper& mapper, MappedRecords<TKeys>& mapped)
  {
    using ElementIdType = typename TKeys::ElementIdType;
    using KeyType = typename TKeys::KeyType;
    using RecordId = RecordIdOf<InputMapper>;
    const auto numRecords = static_cast<vtkIdType>(mapper.GetNumberOfRecords());
    if (numRecords <= 0)
    {
      return true;
    }
    mapped.Offsets.reset(new ElementIdType[numRecords + 1]);
    ElementIdType* offsets = mapped.Offsets.get();
    const vtkIdType numElements = CountRecordElements<TKeys>(mapper, offsets);
    if (numElements > static_cast<vtkIdType>(std::numeric_limits<ElementIdType>::max()))
    {
      vtkLogF(ERROR, "vtk::keys: %lld elements do not fit ElementIdType",
        static_cast<long long>(numElements));
      mapped.Offsets.reset();
      return false;
    }
    vtkSMPTools::ExclusiveScan(offsets, offsets + numRecords + 1, ElementIdType(0));
    mapped.NumberOfElements = static_cast<ElementIdType>(numElements);
    // Default-initialized, so that the keys are not zeroed before being written.
    mapped.Keys.reset(new KeyType[mapped.NumberOfElements]);
    KeyType* elementKeys = mapped.Keys.get();
    vtkSMPTools::For(0, numRecords,
      [&mapper, offsets, elementKeys](vtkIdType begin, vtkIdType end)
      {
        mapper.WriteKeys(
          static_cast<RecordId>(begin), static_cast<RecordId>(end), offsets, elementKeys);
      });
    return true;
  }

  // Empty the keys and the Output, e.g., of more elements than ElementIdType holds.
  template <typename TKeys, typename Output>
  static void Clear(TKeys& keys, Output& output)
  {
    keys.Reset();
    output.SetNumberOfElements(typename TKeys::ElementIdType(0));
  }

  // Set an element at its position. SetElement() is checked here, where its arguments are known.
  template <typename ElementIdType, typename Output, typename... Values>
  static void SetElement(Output& output, ElementIdType position, Values&&... values)
  {
    if constexpr (ValidateSetElement<Output, ElementIdType, Values&&...>())
    {
      output.SetElement(position, std::forward<Values>(values)...);
    }
  }
};

//------------------------------------------------------------------------------
// The engine of ScatterDense(), which writes the keys, and releases every array once it is no
// longer needed:
// - CountKeys(): the key range is set, the elements are counted per key, and the counts scanned
//   into the offsets of the keys;
// - ScatterMappedElements() and ScatterRecords(): every element is placed by decrementing the count
//   of its key, which fills the key from its front without another array, as
//   vtkStaticCellLinksTemplate: the elements of a key are in the order of their records when they
//   are scattered sequentially.
struct DenseScatterer : Scatterer
{
  template <typename TKeys>
  using KeyCounts = std::unique_ptr<std::atomic<typename TKeys::ElementIdType>[]>;

  // Set the key range and the number of keys, count the elements per key, and scan the counts into
  // the offsets of the keys. forEachKey(begin, end, f) calls f(key) for the keys [begin, end) of
  // the numberOfKeys keys. The counts are returned for the scatter.
  template <typename TKeys, typename ForEachKey>
  static KeyCounts<TKeys> CountKeys(TKeys& keys, vtkIdType numberOfKeys,
    const ForEachKey& forEachKey, const typename TKeys::KeyType* keyRange)
  {
    using ElementIdType = typename TKeys::ElementIdType;
    using KeyType = typename TKeys::KeyType;
    // 1. Parallel range reduction, unless the key range is given.
    KeyType minKey = 0;
    KeyType maxKey = 0;
    if (keyRange)
    {
      minKey = keyRange[0];
      maxKey = keyRange[1];
    }
    else
    {
      using RangeType = std::array<KeyType, 2>;
      const RangeType emptyRange{ std::numeric_limits<KeyType>::max(),
        std::numeric_limits<KeyType>::lowest() };
      vtkSMPThreadLocal<RangeType> threadRange(emptyRange);
      vtkSMPTools::For(0, numberOfKeys,
        [&](vtkIdType begin, vtkIdType end)
        {
          auto& localRange = threadRange.Local();
          forEachKey(begin, end,
            [&localRange](KeyType key)
            {
              localRange[0] = std::min(localRange[0], key);
              localRange[1] = std::max(localRange[1], key);
            });
        });
      RangeType globalRange = emptyRange;
      for (const auto& localRange : threadRange)
      {
        globalRange[0] = std::min(globalRange[0], localRange[0]);
        globalRange[1] = std::max(globalRange[1], localRange[1]);
      }
      minKey = globalRange[0];
      maxKey = globalRange[1];
    }
    if (minKey > maxKey) // no element
    {
      minKey = maxKey = 0;
    }
    keys.MinKey = minKey;
    keys.MaxKey = maxKey;
    keys.NumberOfKeys = TKeys::GetKeyId(maxKey, minKey) + 1;
    const typename TKeys::KeyIdType numKeys = keys.NumberOfKeys;

    // 2. Atomic histogram, then exclusive scan into the offsets. The arrays are read through
    // local pointers, which the compiler keeps in registers.
    auto numElementsPerKey = std::make_unique<std::atomic<ElementIdType>[]>(numKeys);
    std::atomic<ElementIdType>* counts = numElementsPerKey.get();
    vtkSMPTools::For(0, numberOfKeys,
      [&forEachKey, counts, minKey](vtkIdType begin, vtkIdType end)
      {
        forEachKey(begin, end, [counts, minKey](KeyType key)
          { counts[TKeys::GetKeyId(key, minKey)].fetch_add(1, std::memory_order_relaxed); });
      });
    keys.Offsets.reset(new ElementIdType[numKeys + 1]);
    ElementIdType* offsets = keys.Offsets.get();
    vtkSMPTools::For(0, static_cast<vtkIdType>(numKeys),
      [offsets, counts](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType k = begin; k < end; ++k)
        {
          offsets[k] = counts[k].load(std::memory_order_relaxed);
        }
      });
    offsets[numKeys] = 0;
    keys.NumberOfElements =
      vtkSMPTools::ExclusiveScan(offsets, offsets + numKeys + 1, ElementIdType(0));
    return numElementsPerKey;
  }

  // Scatter the elements of the records of an InputMapper, with their stored keys, as their record
  // and their index in their record.
  template <typename TKeys, typename InputMapper, typename Output>
  static void ScatterMappedElements(const TKeys& keys, const InputMapper& mapper,
    const MappedRecords<TKeys>& mapped, Output& output, KeyCounts<TKeys>& counts)
  {
    using ElementIdType = typename TKeys::ElementIdType;
    using KeyType = typename TKeys::KeyType;
    using RecordId = RecordIdOf<InputMapper>;
    output.SetNumberOfElements(static_cast<ElementIdType>(keys.NumberOfElements));
    const ElementIdType* offsets = keys.Offsets.get();
    std::atomic<ElementIdType>* keyCounts = counts.get();
    const ElementIdType* recordOffsets = mapped.Offsets.get();
    const KeyType* elementKeys = mapped.Keys.get();
    const KeyType minKey = keys.MinKey;
    const auto numRecords = static_cast<vtkIdType>(mapper.GetNumberOfRecords());
    vtkSMPTools::For(0, numRecords,
      [&output, offsets, keyCounts, recordOffsets, elementKeys, minKey](
        vtkIdType begin, vtkIdType end)
      {
        for (auto record = static_cast<RecordId>(begin); record < static_cast<RecordId>(end);
             ++record)
        {
          const ElementIdType first = recordOffsets[record];
          const ElementIdType last = recordOffsets[record + 1];
          for (ElementIdType element = first; element < last; ++element)
          {
            const auto keyId = TKeys::GetKeyId(elementKeys[element], minKey);
            const ElementIdType position =
              offsets[keyId + 1] - keyCounts[keyId].fetch_sub(1, std::memory_order_relaxed);
            SetElement(output, position, record, static_cast<ElementIdType>(element - first));
          }
        }
      });
    counts.reset();
  }

  // Scatter the elements of an InputMapper, whose offsets and keys are released once scattered.
  template <typename TKeys, typename InputMapper, typename Output>
  static void ScatterMapper(
    TKeys& keys, const InputMapper& mapper, const typename TKeys::KeyType* keyRange, Output& output)
  {
    MappedRecords<TKeys> mapped;
    const bool fits = MapRecords<TKeys>(mapper, mapped);
    Clear(keys, output);
    if (!fits)
    {
      return;
    }
    keys.NumberOfRecords = std::max<vtkIdType>(mapper.GetNumberOfRecords(), 0);
    if (mapped.NumberOfElements == 0)
    {
      return;
    }
    const auto* elementKeys = mapped.Keys.get();
    KeyCounts<TKeys> counts = CountKeys(
      keys, static_cast<vtkIdType>(mapped.NumberOfElements),
      [elementKeys](vtkIdType begin, vtkIdType end, auto&& f)
      {
        for (vtkIdType element = begin; element < end; ++element)
        {
          f(elementKeys[element]);
        }
      },
      keyRange);
    ScatterMappedElements(keys, mapper, mapped, output, counts);
  }

  // Scatter the records of an Input, as ScatterMappedElements().
  template <typename TKeys, typename Input, typename Output>
  static void ScatterRecords(
    const TKeys& keys, const Input& input, Output& output, KeyCounts<TKeys>& counts)
  {
    using ElementIdType = typename TKeys::ElementIdType;
    using KeyType = typename TKeys::KeyType;
    using RecordId = RecordIdOf<Input>;
    output.SetNumberOfElements(static_cast<ElementIdType>(keys.NumberOfElements));
    const ElementIdType* offsets = keys.Offsets.get();
    std::atomic<ElementIdType>* keyCounts = counts.get();
    const KeyType minKey = keys.MinKey;
    vtkSMPTools::For(0, static_cast<vtkIdType>(input.GetNumberOfRecords()),
      [&input, &output, offsets, keyCounts, minKey](vtkIdType begin, vtkIdType end)
      {
        for (auto record = static_cast<RecordId>(begin); record < static_cast<RecordId>(end);
             ++record)
        {
          const auto keyId = TKeys::GetKeyId(static_cast<KeyType>(input.GetKey(record)), minKey);
          const ElementIdType position =
            offsets[keyId + 1] - keyCounts[keyId].fetch_sub(1, std::memory_order_relaxed);
          SetElement(output, position, record);
        }
      });
    counts.reset();
  }

  // Scatter the records of an Input, whose keys are read in every pass, or fail if their number
  // does not fit ElementIdType.
  template <typename TKeys, typename Input, typename Output>
  static void ScatterInput(
    TKeys& keys, const Input& input, const typename TKeys::KeyType* keyRange, Output& output)
  {
    using ElementIdType = typename TKeys::ElementIdType;
    using KeyType = typename TKeys::KeyType;
    using RecordId = RecordIdOf<Input>;
    Clear(keys, output);
    const auto numRecords = static_cast<vtkIdType>(input.GetNumberOfRecords());
    if (numRecords <= 0)
    {
      return;
    }
    if (numRecords > static_cast<vtkIdType>(std::numeric_limits<ElementIdType>::max()))
    {
      vtkLogF(ERROR, "vtk::keys: %lld elements do not fit ElementIdType",
        static_cast<long long>(numRecords));
      return;
    }
    keys.NumberOfRecords = numRecords;
    KeyCounts<TKeys> counts = CountKeys(
      keys, numRecords,
      [&input](vtkIdType begin, vtkIdType end, auto&& f)
      {
        for (auto record = static_cast<RecordId>(begin); record < static_cast<RecordId>(end);
             ++record)
        {
          f(static_cast<KeyType>(input.GetKey(record)));
        }
      },
      keyRange);
    ScatterRecords(keys, input, output, counts);
  }

  // ScatterDense() of an Input or an InputMapper, with an optional key range, into keys of the
  // types of the input.
  template <typename Input, typename Output>
  static DenseKeysOf<Input> Run(
    const Input& input, const KeyTypeOfT<Input>* keyRange, Output& output)
  {
    // The key is checked once the input is valid, so that a failure is reported only once.
    DenseKeysOf<Input> keys;
    if constexpr (ValidateScatter<Input, Output>())
    {
      if constexpr (ValidateDenseKey<KeyTypeOfT<Input>>())
      {
        if constexpr (HasCountElements<Input, ElementIdTypeOfT<Input>>::value)
        {
          ScatterMapper(keys, input, keyRange, output);
        }
        else
        {
          ScatterInput(keys, input, keyRange, output);
        }
      }
    }
    return keys;
  }
};

VTK_ABI_NAMESPACE_END
} // namespace detail

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
template <typename Input, typename Output>
DenseKeysOf<Input> ScatterDense(const Input& input, Output& output)
{
  return detail::DenseScatterer::Run(input, nullptr, output);
}

//------------------------------------------------------------------------------
template <typename Input, typename Output>
DenseKeysOf<Input> ScatterDense(const Input& input, detail::KeyTypeOfT<Input> minKey,
  detail::KeyTypeOfT<Input> maxKey, Output& output)
{
  const detail::KeyTypeOfT<Input> keyRange[2] = { minKey, maxKey };
  return detail::DenseScatterer::Run(input, keyRange, output);
}

VTK_ABI_NAMESPACE_END

namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// The Input of an array of elements: every record is an element, keyed by its GetKey().
template <typename Element, typename ElementIdType>
class ArrayInput
{
public:
  ArrayInput(const Element* elements, ElementIdType numberOfElements)
    : Elements(elements)
    , NumberOfElements(numberOfElements)
  {
  }

  ElementIdType GetNumberOfRecords() const { return this->NumberOfElements; }
  ElementKeyOf<Element> GetKey(ElementIdType record) const
  {
    return this->Elements[record].GetKey();
  }

private:
  const Element* Elements;
  ElementIdType NumberOfElements;
};

//------------------------------------------------------------------------------
// The Output of an array of elements, sized by the caller: an element is copied from its record
// of the input array.
template <typename Element, typename ElementIdType>
class ArrayOutput
{
public:
  ArrayOutput(Element* elements, const Element* records)
    : Elements(elements)
    , Records(records)
  {
  }

  void SetNumberOfElements(ElementIdType) const {}
  void SetElement(ElementIdType position, ElementIdType record) const
  {
    this->Elements[position] = this->Records[record];
  }

private:
  Element* Elements;
  const Element* Records;
};

// A scatter of an array: run(input, output) with the array as an Input and an Output.
template <typename Element, typename ElementIdType, typename Run>
auto ScatterArray(
  const Element* elements, ElementIdType numberOfElements, Element* scattered, const Run& run)
{
  ValidateArray<Element, ElementIdType>();
  ArrayInput<Element, ElementIdType> input(elements, numberOfElements);
  ArrayOutput<Element, ElementIdType> output(scattered, elements);
  return run(input, output);
}

VTK_ABI_NAMESPACE_END
} // namespace detail

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
template <typename Element, typename ElementIdType>
DenseArrayKeys<Element, ElementIdType> ScatterDense(
  const Element* elements, ElementIdType numberOfElements, Element* scattered)
{
  return detail::ScatterArray(elements, numberOfElements, scattered,
    [](const auto& input, auto& output)
    { return detail::DenseScatterer::Run(input, nullptr, output); });
}

//------------------------------------------------------------------------------
template <typename Element, typename ElementIdType>
DenseArrayKeys<Element, ElementIdType> ScatterDense(const Element* elements,
  ElementIdType numberOfElements, detail::ElementKeyOf<Element> minKey,
  detail::ElementKeyOf<Element> maxKey, Element* scattered)
{
  const detail::ElementKeyOf<Element> keyRange[2] = { minKey, maxKey };
  return detail::ScatterArray(elements, numberOfElements, scattered,
    [&keyRange](const auto& input, auto& output)
    { return detail::DenseScatterer::Run(input, keyRange, output); });
}

VTK_ABI_NAMESPACE_END

namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// The engine of ScatterSparse(), data parallel: the elements are sorted by key, then by their
// index, with vtkSMPTools::Sort(), so that the elements of a key are in the order of their records,
// the starts of the distinct keys are counted per chunk of the sorted elements, scanned, and
// written with their keys, and every element is set at its sorted position. It shares the mapping
// of the records and SetElement() with ScatterDense(), see Scatterer.
struct SparseScatterer : Scatterer
{
  // The sorted elements are processed by chunks of this size.
  static constexpr vtkIdType ChunkSize = 4096;

  // Sort the elements 0, ..., numberOfElements - 1 by their keys, write the distinct keys and their
  // offsets, and return the sorted elements.
  template <typename TKeys>
  static std::unique_ptr<typename TKeys::ElementIdType[]> SortByKey(TKeys& keys,
    const typename TKeys::KeyType* elementKeys, typename TKeys::ElementIdType numberOfElements)
  {
    using ElementIdType = typename TKeys::ElementIdType;
    using KeyType = typename TKeys::KeyType;
    const auto numElements = static_cast<vtkIdType>(numberOfElements);
    std::unique_ptr<ElementIdType[]> sortedArray(new ElementIdType[numElements]);
    ElementIdType* sorted = sortedArray.get();
    vtkSMPTools::For(0, numElements,
      [sorted](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
        {
          sorted[i] = static_cast<ElementIdType>(i);
        }
      });
    // The elements of a key are ordered by their index, since vtkSMPTools::Sort() is not stable.
    vtkSMPTools::Sort(sorted, sorted + numElements, [elementKeys](ElementIdType a, ElementIdType b)
      { return elementKeys[a] < elementKeys[b] || (!(elementKeys[b] < elementKeys[a]) && a < b); });

    // Whether the sorted element i starts a key: its key is greater than the previous one.
    auto startsKey = [sorted, elementKeys](vtkIdType i)
    { return i == 0 || elementKeys[sorted[i - 1]] < elementKeys[sorted[i]]; };
    const vtkIdType numChunks = (numElements + ChunkSize - 1) / ChunkSize;
    std::unique_ptr<ElementIdType[]> chunkKeys(new ElementIdType[numChunks + 1]);
    ElementIdType* firstKeys = chunkKeys.get();
    vtkSMPTools::For(0, numChunks,
      [firstKeys, numElements, &startsKey](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType c = begin; c < end; ++c)
        {
          ElementIdType count = 0;
          for (vtkIdType i = c * ChunkSize; i < std::min((c + 1) * ChunkSize, numElements); ++i)
          {
            count += startsKey(i) ? 1 : 0;
          }
          firstKeys[c] = count;
        }
      });
    firstKeys[numChunks] = 0;
    const ElementIdType numKeys =
      vtkSMPTools::ExclusiveScan(firstKeys, firstKeys + numChunks + 1, ElementIdType(0));

    keys.NumberOfKeys = numKeys;
    keys.NumberOfElements = numElements;
    keys.Keys.reset(new KeyType[numKeys]);
    keys.Offsets.reset(new ElementIdType[numKeys + 1]);
    KeyType* distinctKeys = keys.Keys.get();
    ElementIdType* offsets = keys.Offsets.get();
    vtkSMPTools::For(0, numChunks,
      [firstKeys, numElements, &startsKey, sorted, elementKeys, distinctKeys, offsets](
        vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType c = begin; c < end; ++c)
        {
          ElementIdType keyId = firstKeys[c];
          for (vtkIdType i = c * ChunkSize; i < std::min((c + 1) * ChunkSize, numElements); ++i)
          {
            if (startsKey(i))
            {
              distinctKeys[keyId] = elementKeys[sorted[i]];
              offsets[keyId++] = static_cast<ElementIdType>(i);
            }
          }
        }
      });
    offsets[numKeys] = numberOfElements;
    return sortedArray;
  }

  // Scatter the records of an Input, sorted by their keys, or fail if their number does not fit
  // ElementIdType.
  template <typename TKeys, typename Input, typename Output>
  static void ScatterInput(TKeys& keys, const Input& input, Output& output)
  {
    using ElementIdType = typename TKeys::ElementIdType;
    using KeyType = typename TKeys::KeyType;
    using RecordId = RecordIdOf<Input>;
    Clear(keys, output);
    const auto numRecords = static_cast<vtkIdType>(input.GetNumberOfRecords());
    if (numRecords <= 0)
    {
      return;
    }
    if (numRecords > static_cast<vtkIdType>(std::numeric_limits<ElementIdType>::max()))
    {
      vtkLogF(ERROR, "vtk::keys: %lld elements do not fit ElementIdType",
        static_cast<long long>(numRecords));
      return;
    }
    keys.NumberOfRecords = numRecords;
    std::unique_ptr<KeyType[]> elementKeysArray(new KeyType[numRecords]);
    KeyType* elementKeys = elementKeysArray.get();
    vtkSMPTools::For(0, numRecords,
      [&input, elementKeys](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType record = begin; record < end; ++record)
        {
          elementKeys[record] = static_cast<KeyType>(input.GetKey(static_cast<RecordId>(record)));
        }
      });
    std::unique_ptr<ElementIdType[]> sortedArray =
      SortByKey(keys, elementKeys, static_cast<ElementIdType>(numRecords));
    elementKeysArray.reset();
    const ElementIdType* sorted = sortedArray.get();
    output.SetNumberOfElements(static_cast<ElementIdType>(numRecords));
    vtkSMPTools::For(0, numRecords,
      [&output, sorted](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
        {
          SetElement(output, static_cast<ElementIdType>(i), static_cast<RecordId>(sorted[i]));
        }
      });
  }

  // Scatter the elements of an InputMapper, sorted by their keys: the sorted position of every
  // element is the inverse of the sort, so that the records are visited in order.
  template <typename TKeys, typename InputMapper, typename Output>
  static void ScatterMapper(TKeys& keys, const InputMapper& mapper, Output& output)
  {
    using ElementIdType = typename TKeys::ElementIdType;
    using RecordId = RecordIdOf<InputMapper>;
    MappedRecords<TKeys> mapped;
    const bool fits = MapRecords<TKeys>(mapper, mapped);
    Clear(keys, output);
    if (!fits)
    {
      return;
    }
    keys.NumberOfRecords = std::max<vtkIdType>(mapper.GetNumberOfRecords(), 0);
    const ElementIdType numElements = mapped.NumberOfElements;
    if (numElements == 0)
    {
      return;
    }
    std::unique_ptr<ElementIdType[]> sortedArray = SortByKey(keys, mapped.Keys.get(), numElements);
    mapped.Keys.reset();
    std::unique_ptr<ElementIdType[]> positionsArray(new ElementIdType[numElements]);
    ElementIdType* positions = positionsArray.get();
    const ElementIdType* sorted = sortedArray.get();
    vtkSMPTools::For(0, static_cast<vtkIdType>(numElements),
      [positions, sorted](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
        {
          positions[sorted[i]] = static_cast<ElementIdType>(i);
        }
      });
    sortedArray.reset();

    output.SetNumberOfElements(numElements);
    const ElementIdType* recordOffsets = mapped.Offsets.get();
    const auto numRecords = static_cast<vtkIdType>(mapper.GetNumberOfRecords());
    vtkSMPTools::For(0, numRecords,
      [&output, recordOffsets, positions](vtkIdType begin, vtkIdType end)
      {
        for (auto record = static_cast<RecordId>(begin); record < static_cast<RecordId>(end);
             ++record)
        {
          const ElementIdType first = recordOffsets[record];
          for (ElementIdType element = first; element < recordOffsets[record + 1]; ++element)
          {
            SetElement(
              output, positions[element], record, static_cast<ElementIdType>(element - first));
          }
        }
      });
  }

  // ScatterSparse() of an Input or an InputMapper, into keys of the types of the input.
  template <typename Input, typename Output>
  static SparseKeysOf<Input> Run(const Input& input, Output& output)
  {
    // The key is checked once the input is valid, so that a failure is reported only once.
    SparseKeysOf<Input> keys;
    if constexpr (ValidateScatter<Input, Output>())
    {
      if constexpr (ValidateSparseKey<KeyTypeOfT<Input>>())
      {
        if constexpr (HasCountElements<Input, ElementIdTypeOfT<Input>>::value)
        {
          ScatterMapper(keys, input, output);
        }
        else
        {
          ScatterInput(keys, input, output);
        }
      }
    }
    return keys;
  }
};

VTK_ABI_NAMESPACE_END
} // namespace detail

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
template <typename Input, typename Output>
SparseKeysOf<Input> ScatterSparse(const Input& input, Output& output)
{
  return detail::SparseScatterer::Run(input, output);
}

//------------------------------------------------------------------------------
template <typename Element, typename ElementIdType>
SparseArrayKeys<Element, ElementIdType> ScatterSparse(
  const Element* elements, ElementIdType numberOfElements, Element* scattered)
{
  return detail::ScatterArray(elements, numberOfElements, scattered,
    [](const auto& input, auto& output) { return detail::SparseScatterer::Run(input, output); });
}

//------------------------------------------------------------------------------
template <typename TKey, typename TElementId>
typename SparseKeys<TKey, TElementId>::KeyIdType SparseKeys<TKey, TElementId>::FindKeyId(
  KeyType key) const
{
  const KeyType* first = this->Keys.get();
  const KeyType* last = first + this->NumberOfKeys;
  const KeyType* found = std::lower_bound(first, last, key);
  return found != last && !(key < *found) ? static_cast<KeyIdType>(found - first) : KeyIdType(-1);
}

VTK_ABI_NAMESPACE_END

namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// The vtkSMPTools wrapper of a functor on ranges of key ids: calls call(beginKeyId, endKeyId),
// and forwards the Initialize() and Reduce() of the functor, if any.
template <typename Functor, typename Call, bool = HasInitialize<Functor>::value>
class KeyRangeFunctor
{
public:
  KeyRangeFunctor(Functor& functor, Call call)
    : Target(functor)
    , Function(std::move(call))
  {
  }

  void operator()(vtkIdType beginKeyId, vtkIdType endKeyId)
  {
    this->Function(beginKeyId, endKeyId);
  }

protected:
  Functor& Target;
  Call Function;
};

template <typename Functor, typename Call>
class KeyRangeFunctor<Functor, Call, true> : public KeyRangeFunctor<Functor, Call, false>
{
public:
  using KeyRangeFunctor<Functor, Call, false>::KeyRangeFunctor;

  void Initialize() { this->Target.Initialize(); }

  // vtkSMPTools calls Reduce() after the ranges of a functor with an Initialize().
  void Reduce()
  {
    if constexpr (HasReduce<Functor>::value)
    {
      this->Target.Reduce();
    }
  }
};

template <typename Functor, typename Call>
KeyRangeFunctor(Functor&, Call) -> KeyRangeFunctor<Functor, Call>;

VTK_ABI_NAMESPACE_END
} // namespace detail

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
template <typename TKeys, typename Output, typename Functor, typename... Args>
void ForEachKeyRange(const TKeys& keys, Output&& output, Functor& functor, Args... args)
{
  detail::ValidateKeys<TKeys>();
  detail::ValidateKeyRangeFunctor<Functor, Output, TKeys, Args...>();
  using KeyIdType = typename TKeys::KeyIdType;
  using ElementIdType = typename TKeys::ElementIdType;
  const ElementIdType* offsets = keys.GetOffsets();
  detail::KeyRangeFunctor rangeFunctor(functor,
    [&output, &functor, offsets, args...](vtkIdType beginKeyId, vtkIdType endKeyId)
    {
      functor(output, static_cast<KeyIdType>(beginKeyId), static_cast<KeyIdType>(endKeyId), offsets,
        args...);
    });
  vtkSMPTools::For(0, static_cast<vtkIdType>(keys.GetNumberOfKeys()), rangeFunctor);
}

VTK_ABI_NAMESPACE_END

namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// The writer of the ReducedKeys of Reduce(): the reduce functor writes the survivors of every key,
// which are scanned into the offsets.
struct Reducer
{
  // Start the result with the keys of the scattered keys, and return its offsets to fill.
  template <typename TKeys>
  static typename TKeys::ElementIdType* StartLike(const TKeys& keys, TKeys& result)
  {
    result.CopyKeysFrom(keys);
    result.Offsets.reset(new typename TKeys::ElementIdType[keys.NumberOfKeys + 1]);
    result.Offsets[keys.NumberOfKeys] = 0;
    return result.Offsets.get();
  }

  // Scan the counts of the keys of a result into its offsets.
  template <typename TKeys>
  static void ScanCounts(TKeys& result)
  {
    using ElementIdType = typename TKeys::ElementIdType;
    ElementIdType* offsets = result.Offsets.get();
    result.NumberOfElements =
      vtkSMPTools::ExclusiveScan(offsets, offsets + result.NumberOfKeys + 1, ElementIdType(0));
  }

  template <typename TKeys, typename Output, typename Functor>
  static ReducedKeys<TKeys> Run(const TKeys& keys, Output& output, Functor& functor)
  {
    ValidateReduceFunctor<Functor, Output, TKeys>();
    using ElementIdType = typename TKeys::ElementIdType;
    ReducedKeys<TKeys> reduced;
    TKeys& result = reduced;
    ElementIdType* counts = StartLike(keys, result);
    ForEachKeyRange(keys, output, functor, counts);
    ScanCounts(result);
    return reduced;
  }
};

//------------------------------------------------------------------------------
// Call copy(destinationStart, count, sourceStart) for the survivors of every key, in parallel.
template <typename TKeys, typename Copy>
void CompactReducedKeys(const TKeys& keys, const ReducedKeys<TKeys>& reduced, const Copy& copy)
{
  using ElementIdType = typename TKeys::ElementIdType;
  using KeyIdType = typename TKeys::KeyIdType;
  vtkSMPTools::For(0, static_cast<vtkIdType>(reduced.GetNumberOfKeys()),
    [&keys, &reduced, &copy](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType k = begin; k < end; ++k)
      {
        const auto keyId = static_cast<KeyIdType>(k);
        const ElementIdType count = reduced.GetNumberOfElementsOfKey(keyId);
        if (count > 0)
        {
          copy(reduced.GetOffset(keyId), count, keys.GetOffset(keyId));
        }
      }
    });
}

VTK_ABI_NAMESPACE_END
} // namespace detail

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
template <typename TKeys, typename Output, typename Functor>
ReducedKeys<TKeys> Reduce(const TKeys& keys, Output&& output, Functor&& functor)
{
  detail::ValidateKeys<TKeys>();
  return detail::Reducer::Run(keys, output, functor);
}

//------------------------------------------------------------------------------
template <typename TKeys, typename Output, typename>
typename TKeys::ElementIdType Compact(
  const TKeys& keys, const ReducedKeys<TKeys>& reduced, const Output& input, Output& output)
{
  detail::ValidateKeys<TKeys>();
  using ElementIdType = typename TKeys::ElementIdType;
  detail::ValidateCompactOutput<Output, ElementIdType>();
  const auto numCompacted = static_cast<ElementIdType>(reduced.GetNumberOfElements());
  output.SetNumberOfElements(numCompacted);
  detail::CompactReducedKeys(keys, reduced,
    [&input, &output](ElementIdType destination, ElementIdType count, ElementIdType source)
    { output.CopyData(input, destination, count, source); });
  return numCompacted;
}

//------------------------------------------------------------------------------
template <typename TKeys, typename Element>
typename TKeys::ElementIdType Compact(
  const TKeys& keys, const ReducedKeys<TKeys>& reduced, const Element* input, Element* output)
{
  detail::ValidateKeys<TKeys>();
  using ElementIdType = typename TKeys::ElementIdType;
  detail::CompactReducedKeys(keys, reduced,
    [input, output](ElementIdType destination, ElementIdType count, ElementIdType source)
    { std::copy_n(input + source, count, output + destination); });
  return static_cast<ElementIdType>(reduced.GetNumberOfElements());
}

//------------------------------------------------------------------------------
template <typename Element, typename KeyIdType, typename ElementIdType>
void SortUnique::operator()(Element* elements, KeyIdType beginKeyId, KeyIdType endKeyId,
  const ElementIdType* offsets, ElementIdType* counts) const
{
  detail::ValidateSortUnique<Element>();
  for (KeyIdType k = beginKeyId; k < endKeyId; ++k)
  {
    Element* start = elements + offsets[k];
    Element* stop = elements + offsets[k + 1];
    std::sort(start, stop);
    counts[k] = static_cast<ElementIdType>(std::unique(start, stop) - start);
  }
}

VTK_ABI_NAMESPACE_END

namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// The writer of the Grouping of Group(): the group functor flags the groups of a range of keys,
// which are counted while in cache into the keys of the groups, scanned, and the first element of
// every group is written from the flags. The scan is over the keys, not the elements.
struct Grouper
{
  template <typename TKeys, typename Output, typename Functor>
  static Grouping<TKeys> Run(const TKeys& keys, Output& output, Functor& functor)
  {
    ValidateGroupFunctor<Functor, Output, TKeys>();
    using ElementIdType = typename TKeys::ElementIdType;
    using KeyIdType = typename TKeys::KeyIdType;
    Grouping<TKeys> grouping;
    TKeys& groupKeys = grouping.Keys;
    const auto numElements = static_cast<ElementIdType>(keys.GetNumberOfElements());
    const KeyIdType numKeys = keys.GetNumberOfKeys();
    // The arrays are read through local pointers, which the compiler keeps in registers.
    const ElementIdType* offsets = keys.GetOffsets();
    ElementIdType* keyGroups = Reducer::StartLike(keys, groupKeys);
    std::unique_ptr<uint8_t[]> groupStartsArray(new uint8_t[numElements]);
    uint8_t* groupStarts = groupStartsArray.get();

    KeyRangeFunctor flagAndCount(functor,
      [&output, &functor, offsets, groupStarts, keyGroups](vtkIdType begin, vtkIdType end)
      {
        functor(
          output, static_cast<KeyIdType>(begin), static_cast<KeyIdType>(end), offsets, groupStarts);
        for (vtkIdType k = begin; k < end; ++k)
        {
          const ElementIdType start = offsets[k];
          const ElementIdType stop = offsets[k + 1];
          if (stop > start)
          {
            groupStarts[start] = 1; // the first element of a key starts a group
            keyGroups[k] =
              static_cast<ElementIdType>(std::count(groupStarts + start, groupStarts + stop, 1));
          }
          else
          {
            keyGroups[k] = 0;
          }
        }
      });
    vtkSMPTools::For(0, static_cast<vtkIdType>(numKeys), flagAndCount);
    Reducer::ScanCounts(groupKeys);

    GroupedElements<TKeys>& groups = grouping.Groups;
    const auto numGroups = static_cast<ElementIdType>(groupKeys.NumberOfElements);
    groups.NumberOfGroups = numGroups;
    groups.GroupOffsets.reset(new ElementIdType[numGroups + 1]);
    ElementIdType* groupOffsets = groups.GroupOffsets.get();
    vtkSMPTools::For(0, static_cast<vtkIdType>(numKeys),
      [offsets, groupStarts, keyGroups, groupOffsets](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType k = begin; k < end; ++k)
        {
          ElementIdType group = keyGroups[k];
          for (ElementIdType i = offsets[k]; i < offsets[k + 1]; ++i)
          {
            if (groupStarts[i])
            {
              groupOffsets[group++] = i;
            }
          }
        }
      });
    groupOffsets[numGroups] = numElements;
    return grouping;
  }
};

//------------------------------------------------------------------------------
// Call copy(group, firstElement) for every group, in parallel.
template <typename TKeys, typename Copy>
void CompactGroups(const GroupedElements<TKeys>& groups, const Copy& copy)
{
  using ElementIdType = typename TKeys::ElementIdType;
  vtkSMPTools::For(0, static_cast<vtkIdType>(groups.GetNumberOfGroups()),
    [&groups, &copy](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType g = begin; g < end; ++g)
      {
        const auto group = static_cast<ElementIdType>(g);
        copy(group, groups.GetGroupOffset(group));
      }
    });
}

VTK_ABI_NAMESPACE_END
} // namespace detail

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
template <typename TKeys, typename Output, typename Functor>
Grouping<TKeys> Group(const TKeys& keys, Output&& output, Functor&& functor)
{
  detail::ValidateKeys<TKeys>();
  return detail::Grouper::Run(keys, output, functor);
}

//------------------------------------------------------------------------------
template <typename TKeys, typename Output, typename>
typename TKeys::ElementIdType Compact(
  const GroupedElements<TKeys>& groups, const Output& input, Output& output)
{
  using ElementIdType = typename TKeys::ElementIdType;
  detail::ValidateCompactOutput<Output, ElementIdType>();
  output.SetNumberOfElements(groups.GetNumberOfGroups());
  detail::CompactGroups(groups, [&input, &output](ElementIdType group, ElementIdType first)
    { output.CopyData(input, group, ElementIdType(1), first); });
  return groups.GetNumberOfGroups();
}

//------------------------------------------------------------------------------
template <typename TKeys, typename Element>
typename TKeys::ElementIdType Compact(
  const GroupedElements<TKeys>& groups, const Element* input, Element* output)
{
  using ElementIdType = typename TKeys::ElementIdType;
  detail::CompactGroups(groups,
    [input, output](ElementIdType group, ElementIdType first) { output[group] = input[first]; });
  return groups.GetNumberOfGroups();
}

//------------------------------------------------------------------------------
template <typename Element, typename KeyIdType, typename ElementIdType>
void SortGroup::operator()(Element* elements, KeyIdType beginKeyId, KeyIdType endKeyId,
  const ElementIdType* offsets, uint8_t* groupStarts) const
{
  detail::ValidateSortGroup<Element>();
  for (KeyIdType k = beginKeyId; k < endKeyId; ++k)
  {
    const ElementIdType start = offsets[k];
    const ElementIdType stop = offsets[k + 1];
    std::sort(elements + start, elements + stop);
    for (ElementIdType i = start; i < stop; ++i)
    {
      groupStarts[i] = (i == start || elements[i] != elements[i - 1]) ? 1 : 0;
    }
  }
}

VTK_ABI_NAMESPACE_END
} // namespace keys
} // namespace vtk

#endif // vtkKeys_txx
