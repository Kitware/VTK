// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests vtk::keys: ScatterDense() and ScatterSparse(), then Reduce() or Group() and Compact(), of
// an array of elements, of an Input (1-1) and of InputMappers (1-M), into one array and into a
// structure of arrays, with the sort functors and custom functors, keys of point ids and of pairs,
// and element ids of vtkIdType or of the key type.

#include "vtkKeys.h"
#include "vtkLogger.h"
#include "vtkSMPThreadLocal.h"
#include "vtkType.h" // For vtkIdType

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace
{
// An edge (V0, V1) with V0 <= V1, carrying the id of the record that generated it, keyed by V0.
template <typename TId>
struct Edge
{
  TId V0 = 0;
  TId V1 = 0;
  TId Record = 0;

  Edge() = default;
  Edge(TId v0, TId v1, TId record)
    : V0(std::min(v0, v1))
    , V1(std::max(v0, v1))
    , Record(record)
  {
  }

  TId GetKey() const { return this->V0; }
  bool operator==(const Edge& e) const { return this->V0 == e.V0 && this->V1 == e.V1; }
  bool operator!=(const Edge& e) const { return !(*this == e); }
  bool operator<(const Edge& e) const
  {
    return this->V0 < e.V0 || (this->V0 == e.V0 && this->V1 < e.V1);
  }
};

// The same edge, keyed by both its points, for ScatterSparse().
template <typename TId>
struct PairEdge : Edge<TId>
{
  using Edge<TId>::Edge;
  std::pair<TId, TId> GetKey() const { return { this->V0, this->V1 }; }
};

// Reduce functor keeping the elements that appear exactly once in their key, as when the faces
// shared by two cells cancel out, and counting the non-empty keys it reduces, per thread with
// Initialize(), and in total with Reduce(). It holds thread-local storage, so it cannot be copied.
struct KeepOnce
{
  vtkSMPThreadLocal<vtkIdType> LocalNumberOfKeys{ 0 };
  vtkIdType NumberOfKeys = 0;
  bool Initialized = false;

  KeepOnce() = default;
  KeepOnce(const KeepOnce&) = delete;

  void Initialize()
  {
    this->LocalNumberOfKeys.Local() = 0;
    this->Initialized = true;
  }

  template <typename ElementType, typename KeyIdType, typename ElementIdType>
  void operator()(ElementType* elements, KeyIdType beginKeyId, KeyIdType endKeyId,
    const ElementIdType* offsets, ElementIdType* counts)
  {
    vtkIdType& numberOfKeys = this->LocalNumberOfKeys.Local();
    for (KeyIdType k = beginKeyId; k < endKeyId; ++k)
    {
      ElementType* start = elements + offsets[k];
      ElementType* stop = elements + offsets[k + 1];
      if (start == stop)
      {
        counts[k] = 0;
        continue;
      }
      ++numberOfKeys;
      std::sort(start, stop);
      vtkIdType count = 0;
      for (ElementType* it = start; it < stop;)
      {
        ElementType* next = it + 1;
        while (next < stop && *next == *it)
        {
          ++next;
        }
        if (next - it == 1)
        {
          start[count++] = *it;
        }
        it = next;
      }
      counts[k] = static_cast<ElementIdType>(count);
    }
  }

  void Reduce()
  {
    for (vtkIdType numberOfKeys : this->LocalNumberOfKeys)
    {
      this->NumberOfKeys += numberOfKeys;
    }
  }
};

// Group functor grouping the edges by their first point only, and counting the comparisons.
struct GroupByFirstPoint
{
  vtkSMPThreadLocal<vtkIdType> NumberOfComparisons{ 0 };

  template <typename ElementType, typename KeyIdType, typename ElementIdType>
  void operator()(ElementType* elements, KeyIdType beginKeyId, KeyIdType endKeyId,
    const ElementIdType* offsets, uint8_t* groupStarts)
  {
    vtkIdType& numberOfComparisons = this->NumberOfComparisons.Local();
    for (KeyIdType k = beginKeyId; k < endKeyId; ++k)
    {
      std::sort(elements + offsets[k], elements + offsets[k + 1]);
      for (ElementIdType i = offsets[k]; i < offsets[k + 1]; ++i)
      {
        if (i == offsets[k])
        {
          groupStarts[i] = 1;
          continue;
        }
        ++numberOfComparisons;
        groupStarts[i] = elements[i].V0 != elements[i - 1].V0 ? 1 : 0;
      }
    }
  }
};

// Input of the edges (1-1), keyed by their first point.
template <typename EdgeType, typename TElementId>
struct EdgesInput
{
  using ElementIdType = TElementId;

  vtkIdType GetNumberOfRecords() const { return static_cast<vtkIdType>(this->Edges.size()); }
  auto GetKey(vtkIdType record) const { return this->Edges[record].GetKey(); }

  const std::vector<EdgeType>& Edges;
};

// InputMapper mapping every edge to two elements (1-M), as a cell to its faces, scattered as
// their record and their index in their record.
template <typename EdgeType, typename TElementId>
struct DuplicatedEdgesMapper
{
  using KeyType = decltype(std::declval<EdgeType>().GetKey());
  using ElementIdType = TElementId;

  vtkIdType GetNumberOfRecords() const { return static_cast<vtkIdType>(this->Edges.size()); }
  void CountElements(vtkIdType begin, vtkIdType end, ElementIdType* counts) const
  {
    std::fill(counts, counts + (end - begin), ElementIdType(2));
  }
  void WriteKeys(vtkIdType begin, vtkIdType end, const ElementIdType* offsets, KeyType* keys) const
  {
    for (vtkIdType i = begin; i < end; ++i)
    {
      keys[offsets[i]] = keys[offsets[i] + 1] = this->Edges[i].GetKey();
    }
  }

  const std::vector<EdgeType>& Edges;
};

// InputMapper of records mapped to no element.
template <typename TKey, typename TElementId>
struct EmptyRecordsMapper
{
  using KeyType = TKey;
  using ElementIdType = TElementId;

  vtkIdType GetNumberOfRecords() const { return this->NumberOfRecords; }
  void CountElements(vtkIdType begin, vtkIdType end, ElementIdType* counts) const
  {
    std::fill(counts, counts + (end - begin), ElementIdType(0));
  }
  void WriteKeys(vtkIdType, vtkIdType, const ElementIdType*, KeyType*) const {}

  vtkIdType NumberOfRecords;
};

// Output of the edges of an Input or of an InputMapper, copied from their record.
template <typename EdgeType>
struct EdgeOutput
{
  template <typename ElementIdType>
  void SetNumberOfElements(ElementIdType numberOfElements)
  {
    this->Edges.resize(numberOfElements);
  }
  template <typename ElementIdType>
  void SetElement(ElementIdType position, vtkIdType record)
  {
    this->Edges[position] = this->Records[record];
  }
  template <typename ElementIdType>
  void SetElement(ElementIdType position, vtkIdType record, ElementIdType)
  {
    this->Edges[position] = this->Records[record];
  }

  explicit EdgeOutput(const std::vector<EdgeType>& records)
    : Records(records)
  {
  }
  EdgeType* GetElements() { return this->Edges.data(); }

  const std::vector<EdgeType>& Records;
  std::vector<EdgeType> Edges;
};

// Functor of EdgeOutput applying a reduce or group functor of arrays of elements to its edges.
template <typename Functor>
struct OnEdges
{
  template <typename EdgeType, typename... Args>
  void operator()(EdgeOutput<EdgeType>& output, Args... args) const
  {
    this->ElementsFunctor(output.GetElements(), args...);
  }

  Functor& ElementsFunctor;
};

// Output of the key and the record of every edge in two arrays (a structure of arrays).
template <typename EdgeType, typename TId>
struct KeyRecordOutput
{
  template <typename ElementIdType>
  void SetNumberOfElements(ElementIdType numberOfElements)
  {
    this->Keys.resize(numberOfElements);
    this->Records.resize(numberOfElements);
  }
  template <typename ElementIdType>
  void SetElement(ElementIdType position, vtkIdType record)
  {
    this->Keys[position] = this->Edges[record].GetKey();
    this->Records[position] = static_cast<TId>(record);
  }
  template <typename ElementIdType>
  void CopyData(const KeyRecordOutput& source, ElementIdType destinationStart, ElementIdType count,
    ElementIdType sourceStart)
  {
    std::copy_n(source.Keys.begin() + sourceStart, count, this->Keys.begin() + destinationStart);
    std::copy_n(
      source.Records.begin() + sourceStart, count, this->Records.begin() + destinationStart);
  }

  explicit KeyRecordOutput(const std::vector<EdgeType>& edges)
    : Edges(edges)
  {
  }

  const std::vector<EdgeType>& Edges;
  std::vector<TId> Keys;
  std::vector<TId> Records;
};

// Reduce functor of KeyRecordOutput keeping all the elements, sorted by record.
struct SortRecords
{
  template <typename EdgeType, typename TId, typename KeyIdType, typename ElementIdType>
  void operator()(KeyRecordOutput<EdgeType, TId>& output, KeyIdType beginKeyId, KeyIdType endKeyId,
    const ElementIdType* offsets, ElementIdType* counts) const
  {
    for (KeyIdType k = beginKeyId; k < endKeyId; ++k)
    {
      std::sort(output.Records.begin() + offsets[k], output.Records.begin() + offsets[k + 1]);
      counts[k] = offsets[k + 1] - offsets[k];
    }
  }
};

#define CHECK(condition)                                                                           \
  do                                                                                               \
  {                                                                                                \
    if (!(condition))                                                                              \
    {                                                                                              \
      std::cerr << "Check failed at line " << __LINE__ << ": " #condition << std::endl;            \
      return false;                                                                                \
    }                                                                                              \
  } while (false)

// Check that the elements of every key have that key.
template <typename KeysT, typename EdgeT>
bool CheckScattered(const KeysT& keys, const EdgeT* output)
{
  CHECK(keys.GetNumberOfKeys() == 0 || keys.GetOffset(0) == 0);
  CHECK(keys.GetNumberOfKeys() == 0 ||
    keys.GetOffset(keys.GetNumberOfKeys()) == keys.GetNumberOfElements());
  for (typename KeysT::KeyIdType k = 0; k < keys.GetNumberOfKeys(); ++k)
  {
    for (auto i = keys.GetOffset(k); i < keys.GetOffset(k + 1); ++i)
    {
      CHECK(output[i].GetKey() == keys.GetKey(k));
    }
  }
  return true;
}

// Check that the surviving elements of every key are expected, and have that key.
template <typename TId, typename KeysT, typename ReducedT, typename EdgeT>
bool CheckReduced(const KeysT& keys, const ReducedT& reduced, const EdgeT* output,
  const std::set<std::pair<TId, TId>>& expected)
{
  CHECK(reduced.GetNumberOfElements() == static_cast<vtkIdType>(expected.size()));
  CHECK(reduced.GetNumberOfKeys() == keys.GetNumberOfKeys());
  std::set<std::pair<TId, TId>> survivors;
  for (typename KeysT::KeyIdType k = 0; k < keys.GetNumberOfKeys(); ++k)
  {
    for (vtkIdType i = 0; i < reduced.GetNumberOfElementsOfKey(k); ++i)
    {
      const EdgeT& e = output[keys.GetOffset(k) + i];
      CHECK(e.GetKey() == keys.GetKey(k));
      CHECK(survivors.insert({ e.V0, e.V1 }).second);
    }
  }
  CHECK(survivors == expected);
  return true;
}

// Check that groups hold equal edges, ordered by key, with the expected multiplicity, and that the
// keys of the groups give the groups of every key.
template <typename TId, typename KeysT, typename GroupingT, typename EdgeT>
bool CheckGrouped(const KeysT& keys, const GroupingT& grouping, const EdgeT* output,
  const std::map<std::pair<TId, TId>, TId>& expected, TId multiplicity)
{
  const auto& groups = grouping.Groups;
  CHECK(groups.GetNumberOfGroups() == static_cast<vtkIdType>(expected.size()));
  CHECK(groups.GetGroupOffset(0) == 0);
  CHECK(groups.GetGroupOffset(groups.GetNumberOfGroups()) == keys.GetNumberOfElements());
  CHECK(groups.GetGroupOffsets()[groups.GetNumberOfGroups()] == keys.GetNumberOfElements());
  for (vtkIdType g = 0; g < groups.GetNumberOfGroups(); ++g)
  {
    const EdgeT& first = output[groups.GetGroupOffset(g)];
    CHECK(g == 0 || !(first.GetKey() < output[groups.GetGroupOffset(g - 1)].GetKey()));
    for (auto i = groups.GetGroupOffset(g); i < groups.GetGroupOffset(g + 1); ++i)
    {
      CHECK(output[i] == first);
    }
    auto it = expected.find({ first.V0, first.V1 });
    CHECK(it != expected.end());
    CHECK(groups.GetGroupSize(g) == multiplicity * it->second);
  }
  const auto& groupKeys = grouping.Keys;
  CHECK(groupKeys.GetNumberOfKeys() == keys.GetNumberOfKeys());
  CHECK(groupKeys.GetNumberOfElements() == groups.GetNumberOfGroups());
  for (typename KeysT::KeyIdType k = 0; k < groupKeys.GetNumberOfKeys(); ++k)
  {
    for (auto g = groupKeys.GetOffset(k); g < groupKeys.GetOffset(k + 1); ++g)
    {
      CHECK(output[groups.GetGroupOffset(g)].GetKey() == keys.GetKey(k));
    }
  }
  return true;
}

template <typename TId, typename ElementIdType>
bool TestOperations()
{
  using EdgeType = Edge<TId>;
  using Output = EdgeOutput<EdgeType>;
  vtk::keys::SortUnique sortUnique;
  vtk::keys::SortGroup sortGroup;

  // Random edges over a few points, with duplicates, starting at point 3.
  const TId numPoints = 50;
  const auto numEdges = static_cast<ElementIdType>(2000);
  std::vector<EdgeType> edges;
  std::map<std::pair<TId, TId>, TId> counts;
  uint32_t seed = 12345;
  auto random = [&seed] { return (seed = seed * 1664525u + 1013904223u) >> 8; };
  for (TId i = 0; i < static_cast<TId>(numEdges); ++i)
  {
    const EdgeType e(
      static_cast<TId>(3 + random() % numPoints), static_cast<TId>(3 + random() % numPoints), i);
    edges.push_back(e);
    ++counts[{ e.V0, e.V1 }];
  }
  std::set<std::pair<TId, TId>> unique;
  std::set<std::pair<TId, TId>> once;
  std::set<TId> firstPoints;
  for (const auto& c : counts)
  {
    unique.insert(c.first);
    firstPoints.insert(c.first.first);
    if (c.second == 1)
    {
      once.insert(c.first);
    }
  }

  // ScatterDense() of an array: every key of the range, with the elements of its key.
  {
    std::vector<EdgeType> output(numEdges);
    const auto keys = vtk::keys::ScatterDense(edges.data(), numEdges, output.data());
    CHECK(keys.GetNumberOfElements() == numEdges && keys.GetNumberOfRecords() == numEdges);
    CHECK(keys.GetMinKey() == *firstPoints.begin() && keys.GetMaxKey() == *firstPoints.rbegin());
    CHECK(keys.GetNumberOfKeys() == keys.GetMaxKey() - keys.GetMinKey() + 1);
    CHECK(keys.GetKeyId(keys.GetMaxKey()) == keys.GetNumberOfKeys() - 1);
    CHECK((CheckScattered(keys, output.data())));
  }

  // ScatterSparse() of an array: the distinct keys only, in increasing order, found by FindKeyId().
  {
    std::vector<EdgeType> output(numEdges);
    const auto keys = vtk::keys::ScatterSparse(edges.data(), numEdges, output.data());
    CHECK(keys.GetNumberOfElements() == numEdges);
    CHECK(keys.GetNumberOfKeys() == static_cast<ElementIdType>(firstPoints.size()));
    CHECK((CheckScattered(keys, output.data())));
    ElementIdType keyId = 0;
    for (TId firstPoint : firstPoints)
    {
      CHECK(keys.GetKey(keyId) == firstPoint && keys.FindKeyId(firstPoint) == keyId);
      CHECK(keys.GetNumberOfElementsOfKey(keyId++) > 0);
    }
    CHECK(keys.FindKeyId(0) == -1 && keys.FindKeyId(numPoints + 3) == -1);
  }

  // ScatterSparse() of an array keyed by pairs: every key is a distinct edge, with its count.
  {
    std::vector<PairEdge<TId>> pairEdges;
    pairEdges.reserve(edges.size());
    for (const EdgeType& e : edges)
    {
      pairEdges.emplace_back(e.V0, e.V1, e.Record);
    }
    std::vector<PairEdge<TId>> output(numEdges);
    const auto keys = vtk::keys::ScatterSparse(pairEdges.data(), numEdges, output.data());
    CHECK(keys.GetNumberOfKeys() == static_cast<ElementIdType>(counts.size()));
    CHECK((CheckScattered(keys, output.data())));
    for (const auto& c : counts)
    {
      const ElementIdType keyId = keys.FindKeyId(c.first);
      CHECK(keyId >= 0 && keys.GetKey(keyId) == c.first);
      CHECK(keys.GetNumberOfElementsOfKey(keyId) == static_cast<ElementIdType>(c.second));
    }
    CHECK(keys.FindKeyId({ numPoints + 3, numPoints + 3 }) == -1);
  }

  // Reduce() and Compact() of an array, with dense and sparse keys.
  for (bool sparse : { false, true })
  {
    std::vector<EdgeType> output(numEdges);
    auto run = [&](auto keys)
    {
      const auto reduced = vtk::keys::Reduce(keys, output.data(), sortUnique);
      CHECK((CheckReduced<TId>(keys, reduced, output.data(), unique)));

      // Compact() copies the unique edges contiguously, in the order of the keys.
      std::vector<EdgeType> compacted(reduced.GetNumberOfElements());
      CHECK(vtk::keys::Compact(keys, reduced, output.data(), compacted.data()) ==
        reduced.GetNumberOfElements());
      for (typename decltype(keys)::KeyIdType k = 0; k < keys.GetNumberOfKeys(); ++k)
      {
        for (vtkIdType i = 0; i < reduced.GetNumberOfElementsOfKey(k); ++i)
        {
          CHECK(compacted[reduced.GetOffset(k) + i] == output[keys.GetOffset(k) + i]);
        }
      }

      // Reset() releases the memory.
      auto released = vtk::keys::Reduce(keys, output.data(), sortUnique);
      released.Reset();
      CHECK(released.GetNumberOfKeys() == 0 && released.GetNumberOfElements() == 0 &&
        released.GetOffsets() == nullptr);
      return true;
    };
    if (sparse)
    {
      CHECK(run(vtk::keys::ScatterSparse(edges.data(), numEdges, output.data())));
    }
    else
    {
      CHECK(run(vtk::keys::ScatterDense(edges.data(), numEdges, output.data())));
    }
  }

  // Reduce() with a custom, non-copyable functor, whose Initialize() and Reduce() are called.
  {
    std::vector<EdgeType> output(numEdges);
    KeepOnce functor;
    const auto keys = vtk::keys::ScatterDense(edges.data(), numEdges, output.data());
    const auto reduced = vtk::keys::Reduce(keys, output.data(), functor);
    CHECK((CheckReduced<TId>(keys, reduced, output.data(), once)));
    CHECK(functor.Initialized);
    // Every non-empty key is reduced once.
    vtkIdType numNonEmptyKeys = 0;
    for (TId k = 0; k < keys.GetNumberOfKeys(); ++k)
    {
      numNonEmptyKeys += keys.GetNumberOfElementsOfKey(k) > 0 ? 1 : 0;
    }
    CHECK(functor.NumberOfKeys == numNonEmptyKeys);
  }

  // An Input (1-1) and an InputMapper (1-M) mapping every edge to two elements, with dense keys
  // with and without a known key range, and with sparse keys.
  const EdgesInput<EdgeType, ElementIdType> input{ edges };
  const DuplicatedEdgesMapper<EdgeType, ElementIdType> mapper{ edges };
  for (int source = 0; source < 2; ++source)
  {
    for (int kind = 0; kind < 3; ++kind)
    {
      const TId multiplicity = source == 0 ? 1 : 2;
      // scatter(source, output): the keys of the elements of a source in an Output.
      auto test = [&](const auto& from, auto scatter)
      {
        Output output(edges);
        const auto keys = scatter(from, output);
        CHECK(keys.GetNumberOfElements() == multiplicity * numEdges);
        CHECK(keys.GetNumberOfRecords() == numEdges);
        CHECK((CheckScattered(keys, output.GetElements())));
        const auto reduced =
          vtk::keys::Reduce(keys, output, OnEdges<vtk::keys::SortUnique>{ sortUnique });
        CHECK((CheckReduced<TId>(keys, reduced, output.GetElements(), unique)));

        Output groupsOutput(edges);
        const auto groupKeys = scatter(from, groupsOutput);
        const auto grouping =
          vtk::keys::Group(groupKeys, groupsOutput, OnEdges<vtk::keys::SortGroup>{ sortGroup });
        CHECK((CheckGrouped<TId>(
          groupKeys, grouping, groupsOutput.GetElements(), counts, multiplicity)));

        // With every edge at least twice, none is kept by KeepOnce.
        if (multiplicity == 2)
        {
          KeepOnce keepOnce;
          Output onceOutput(edges);
          const auto onceKeys = scatter(from, onceOutput);
          CHECK(vtk::keys::Reduce(onceKeys, onceOutput, OnEdges<KeepOnce>{ keepOnce })
                  .GetNumberOfElements() == 0);
        }
        return true;
      };
      auto dense = [&](const auto& from, Output& output)
      {
        return kind == 0 ? vtk::keys::ScatterDense(from, output)
                         : vtk::keys::ScatterDense(from, TId(0), TId(numPoints + 3), output);
      };
      auto sparse = [](const auto& from, Output& output)
      { return vtk::keys::ScatterSparse(from, output); };
      if (source == 0)
      {
        CHECK(kind < 2 ? test(input, dense) : test(input, sparse));
      }
      else
      {
        CHECK(kind < 2 ? test(mapper, dense) : test(mapper, sparse));
      }
    }
  }

  // The elements of an InputMapper counted without being stored, e.g., to choose ElementIdType,
  // and the elements of an Input and of an InputMapper that do not fit ElementIdType.
  {
    CHECK(vtk::keys::CountElements(mapper) == 2 * numEdges);
    CHECK(vtk::keys::CountElements(EmptyRecordsMapper<TId, ElementIdType>{ 3000 }) == 0);

    const auto verbosity = vtkLogger::GetCurrentVerbosityCutoff();
    vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_OFF);
    Output output(edges);
    const EdgesInput<EdgeType, int8_t> smallInput{ edges };
    const DuplicatedEdgesMapper<EdgeType, int8_t> smallMapper{ edges };
    const auto denseInput = vtk::keys::ScatterDense(smallInput, output);
    const auto denseMapper = vtk::keys::ScatterDense(smallMapper, output);
    const auto sparseInput = vtk::keys::ScatterSparse(smallInput, output);
    const auto sparseMapper = vtk::keys::ScatterSparse(smallMapper, output);
    vtkLogger::SetStderrVerbosity(verbosity);
    CHECK(denseInput.GetNumberOfKeys() == 0 && denseMapper.GetNumberOfKeys() == 0);
    CHECK(sparseInput.GetNumberOfKeys() == 0 && sparseMapper.GetNumberOfKeys() == 0);
    CHECK(output.Edges.empty());
  }

  // An Input scattered into a structure of arrays, compacted through CopyData().
  {
    KeyRecordOutput<EdgeType, TId> output(edges);
    const auto keys = vtk::keys::ScatterDense(input, output);
    const auto reduced = vtk::keys::Reduce(keys, output, SortRecords{});
    CHECK(reduced.GetNumberOfElements() == numEdges);
    std::vector<bool> found(numEdges, false);
    for (TId k = 0; k < keys.GetNumberOfKeys(); ++k)
    {
      for (auto i = keys.GetOffset(k); i < keys.GetOffset(k + 1); ++i)
      {
        CHECK(output.Keys[i] == keys.GetKey(k));
        CHECK(i == keys.GetOffset(k) || output.Records[i - 1] < output.Records[i]);
        found[output.Records[i]] = true;
      }
    }
    CHECK(std::all_of(found.begin(), found.end(), [](bool f) { return f; }));

    KeyRecordOutput<EdgeType, TId> compacted(edges);
    CHECK(vtk::keys::Compact(keys, reduced, output, compacted) == numEdges);
    for (TId k = 0; k < keys.GetNumberOfKeys(); ++k)
    {
      for (vtkIdType i = 0; i < reduced.GetNumberOfElementsOfKey(k); ++i)
      {
        CHECK(compacted.Keys[reduced.GetOffset(k) + i] == output.Keys[keys.GetOffset(k) + i] &&
          compacted.Records[reduced.GetOffset(k) + i] == output.Records[keys.GetOffset(k) + i]);
      }
    }
  }

  // Group() and Compact() of an array, keeping both results or only the groups.
  {
    std::vector<EdgeType> output(numEdges);
    const auto keys = vtk::keys::ScatterDense(edges.data(), numEdges, output.data());
    const auto grouping = vtk::keys::Group(keys, output.data(), sortGroup);
    CHECK((CheckGrouped<TId>(keys, grouping, output.data(), counts, 1)));

    // Compact() copies the first edge of every group, so that group g is edge g.
    const vtkIdType numGroups = grouping.Groups.GetNumberOfGroups();
    std::vector<EdgeType> compacted(numGroups);
    CHECK(vtk::keys::Compact(grouping.Groups, output.data(), compacted.data()) == numGroups);
    for (vtkIdType g = 0; g < numGroups; ++g)
    {
      CHECK(compacted[g] == output[grouping.Groups.GetGroupOffset(g)]);
    }

    // Only the groups, the keys of the groups being released with the temporary.
    std::vector<EdgeType> groupsOutput(numEdges);
    const auto groupsKeys = vtk::keys::ScatterDense(edges.data(), numEdges, groupsOutput.data());
    const auto groups = vtk::keys::Group(groupsKeys, groupsOutput.data(), sortGroup).Groups;
    CHECK(groups.GetNumberOfGroups() == numGroups);
    for (vtkIdType g = 0; g <= numGroups; ++g)
    {
      CHECK(groups.GetGroupOffset(g) == grouping.Groups.GetGroupOffset(g));
    }
  }

  // Group() with a custom equivalence: edges with the same first point, compared once.
  {
    std::vector<EdgeType> output(numEdges);
    GroupByFirstPoint functor;
    const auto keys = vtk::keys::ScatterDense(edges.data(), numEdges, output.data());
    const auto groups = vtk::keys::Group(keys, output.data(), functor).Groups;
    CHECK(groups.GetNumberOfGroups() == static_cast<vtkIdType>(firstPoints.size()));
    for (vtkIdType g = 0; g < groups.GetNumberOfGroups(); ++g)
    {
      for (auto i = groups.GetGroupOffset(g); i < groups.GetGroupOffset(g + 1); ++i)
      {
        CHECK(output[i].V0 == output[groups.GetGroupOffset(g)].V0);
      }
    }
    vtkIdType numComparisons = 0;
    for (vtkIdType n : functor.NumberOfComparisons)
    {
      numComparisons += n;
    }
    // One comparison per element but the first of every non-empty key, at most.
    CHECK(numComparisons < numEdges);
  }

  // Empty arrays, and an InputMapper mapping its records to no element.
  {
    EdgeType* none = nullptr;
    const auto dense = vtk::keys::ScatterDense(edges.data(), ElementIdType(0), none);
    const auto sparse = vtk::keys::ScatterSparse(edges.data(), ElementIdType(0), none);
    CHECK(dense.GetNumberOfKeys() == 0 && dense.GetNumberOfElements() == 0);
    CHECK(sparse.GetNumberOfKeys() == 0 && sparse.GetNumberOfElements() == 0);
    CHECK(vtk::keys::Reduce(dense, none, sortUnique).GetNumberOfElements() == 0);
    CHECK(vtk::keys::Group(sparse, none, sortGroup).Groups.GetNumberOfGroups() == 0);

    Output output(edges);
    const EmptyRecordsMapper<TId, ElementIdType> nothing{ 10 };
    const auto mapped = vtk::keys::ScatterDense(nothing, output);
    CHECK(mapped.GetNumberOfElements() == 0 && mapped.GetNumberOfRecords() == 10);
    CHECK(vtk::keys::Reduce(mapped, output, OnEdges<vtk::keys::SortUnique>{ sortUnique })
            .GetNumberOfElements() == 0);
    CHECK(vtk::keys::Group(mapped, output, OnEdges<vtk::keys::SortGroup>{ sortGroup })
            .Groups.GetNumberOfGroups() == 0);
    CHECK(output.Edges.empty());
  }
  return true;
}

} // namespace

int TestKeys(int, char*[])
{
  if (!TestOperations<vtkIdType, vtkIdType>() || !TestOperations<int32_t, int32_t>() ||
    !TestOperations<int32_t, vtkIdType>())
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
