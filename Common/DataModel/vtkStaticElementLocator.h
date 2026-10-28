// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStaticElementLocator
 * @brief   templated locator of topological elements, with data on the elements
 *
 * vtkStaticElementLocator locates topological elements, i.e., tuples of point ids, e.g., an edge
 * (v0, v1) with v0 < v1 (see EdgeTuple, TriangleTuple and QuadTuple), with data associated with
 * them, e.g., to find the duplicate edges of an isocontour. It is templated on the type of the ids
 * (IDType) and the element tuple (ElementType), which carries the data.
 *
 * The locator is static, as vtkStaticPointLocator and vtkStaticCellLocator: it is built from an
 * array of elements, to which no element can be added afterwards. It is used in one of two ways:
 * - BuildLocator(), then IsInsertedElement() queries, to search the unique elements;
 * - GroupElements(), which sorts an array of elements into groups of equal elements, whose offsets
 *   give the number of unique elements and O(1) access to every group, e.g., to renumber the
 *   duplicate elements or their data.
 * Both scatter the elements in parallel by their key (GetKey(), e.g., their smallest point id) with
 * vtk::keys::ScatterDense(), see vtkKeys.h, so that only the elements of a key are sorted and
 * compared.
 *
 * @warning
 * The number of unique elements of BuildLocator() must fit IDType, which can be smaller than
 * vtkIdType to reduce memory and speed up processing. The number of elements of GroupElements()
 * can exceed it, since the offsets of its groups are vtkIdType.
 *
 * @warning
 * The data of the elements is not used to compare and sort them: since the elements are binned in
 * parallel, equal elements with different data can be ordered differently within their group from
 * one run to the next.
 *
 * @warning
 * ElementType must provide `GetKey() const -> IDType`, operator==, operator!= and operator<: the
 * Element concept of vtkKeys.h, checked at compile time. The keys must be dense, as point ids are,
 * see vtk::keys::DenseKeys.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkStaticPointLocator vtkStaticCellLocator
 */

#ifndef vtkStaticElementLocator_h
#define vtkStaticElementLocator_h

#include "vtkABINamespace.h"
#include "vtkKeys.h" // For the scatter of the elements by key
#include "vtkType.h" // For vtkIdType

#include <type_traits> // For std::is_void_v
#include <utility>     // For std::forward, std::swap

VTK_ABI_NAMESPACE_BEGIN

template <typename TID, typename TElement>
class vtkStaticElementLocator
{
public:
  using IDType = TID;
  using ElementType = TElement;
  // The keys of the elements, whose number is not bounded by IDType.
  using KeysType = vtk::keys::DenseArrayKeys<ElementType, vtkIdType>;

  /**
   * The groups of equal elements of GroupElements(), see vtk::keys::GroupedElements.
   */
  using GroupedElements = vtk::keys::GroupedElements<KeysType>;

  /**
   * Construct an empty element locator.
   */
  vtkStaticElementLocator() = default;
  ~vtkStaticElementLocator() = default;

  /**
   * Move a locator. The locator does not own the elements, so their array must still outlive it. A
   * moved-from locator must be built again before it is used.
   */
  vtkStaticElementLocator(vtkStaticElementLocator&&) = default;
  vtkStaticElementLocator& operator=(vtkStaticElementLocator&&) = default;

  /**
   * Build the locator from the numElements elements of elementArray, which may contain
   * duplicates: the unique elements are moved to its front, sorted by key, see vtkKeys.h.
   * elementArray is modified in place, and must outlive the locator. Return the number of unique
   * elements.
   */
  vtkIdType BuildLocator(vtkIdType numElements, ElementType* elementArray);

  /**
   * Return the id of an element, or a negative value if it was not inserted. The point ids of the
   * query can be in any order, since the element tuple sorts them. BuildLocator() must be called
   * first.
   */
  IDType IsInsertedElement(const ElementType& query) const;

  /**
   * Same as above, with the query constructed in place from its arguments, as emplace_back(),
   * e.g., IsInsertedElement(v0, v1) for an element.
   */
  template <typename Arg0, typename Arg1, typename... Args>
  IDType IsInsertedElement(Arg0&& arg0, Arg1&& arg1, Args&&... args) const
  {
    return this->IsInsertedElement(
      ElementType(std::forward<Arg0>(arg0), std::forward<Arg1>(arg1), std::forward<Args>(args)...));
  }

  /**
   * Return the number of unique elements, after BuildLocator().
   */
  IDType GetNumberOfElements() const { return this->NumElements; }

  /**
   * Return the element of id i, i.e., the ith unique element, 0 <= i < GetNumberOfElements().
   */
  const ElementType& GetElement(IDType i) const { return this->ElementArray[i]; }

  /**
   * Sort the numElements elements of elementArray in place into groups of equal elements, and
   * return them: group g is [GetGroupOffset(g), GetGroupOffset(g + 1)) of elementArray, the
   * number of unique elements is GetNumberOfGroups(), and GetGroupOffset(GetNumberOfGroups()) is
   * numElements. The data of the elements can then, e.g., renumber the duplicate elements to unique
   * ids. The groups are owned by the locator until the next GroupElements(), independently of
   * BuildLocator(). The equal elements are grouped with vtk::keys::SortGroup: another rule needs
   * vtk::keys::Group() with its group functor.
   */
  const GroupedElements& GroupElements(vtkIdType numElements, ElementType* elementArray);

protected:
  IDType NumElements = 0;
  ElementType* ElementArray = nullptr; // Non-owning pointer to external memory
  // The search index of BuildLocator(): the unique elements of key id k are
  // [GetOffset(k), GetOffset(k + 1)) of ElementArray.
  vtk::keys::ReducedKeys<KeysType> Index;
  // The groups of equal elements of GroupElements().
  GroupedElements Groups;

private:
  vtkStaticElementLocator(const vtkStaticElementLocator&) = delete;
  void operator=(const vtkStaticElementLocator&) = delete;
};

// ============================================================================
// Common Elements
// ============================================================================

/**
 * The data associated with an element tuple, TData, as its member Data. With TData void, the
 * tuple has no data, and this empty base takes no space.
 */
template <typename TData>
struct vtkTupleData
{
  TData Data;
};
template <>
struct vtkTupleData<void>
{
};

/**
 * Definition of an edge tuple: its 2 point ids, ordered (V0 <= V1) by the constructors. TId can be
 * smaller than vtkIdType to save memory and speed up sorting. The data TED, if not void, is
 * associated with the edge, but not used when comparing and sorting the edges.
 */
template <typename TId, typename TED = void>
struct EdgeTuple : vtkTupleData<TED>
{
  TId V0;
  TId V1;

  EdgeTuple() = default;

  // Construct an edge with its data.
  template <typename D = TED, typename = std::enable_if_t<!std::is_void_v<D>>>
  EdgeTuple(TId v0, TId v1, const std::common_type_t<D>& data)
    : vtkTupleData<TED>{ data }
    , V0(v0)
    , V1(v1)
  {
    if (this->V0 > this->V1)
    {
      std::swap(this->V0, this->V1);
    }
  }

  // Construct an edge without data, e.g., a query.
  EdgeTuple(TId v0, TId v1)
    : vtkTupleData<TED>()
    , V0(v0)
    , V1(v1)
  {
    if (this->V0 > this->V1)
    {
      std::swap(this->V0, this->V1);
    }
  }

  // The key of the element: required by vtkStaticElementLocator, see the Element concept of
  // vtkKeys.h.
  TId GetKey() const { return this->V0; }

  // Set the points of the edge, ordered.
  void Define(TId v0, TId v1)
  {
    if (v0 < v1)
    {
      this->V0 = v0;
      this->V1 = v1;
    }
    else
    {
      this->V0 = v1;
      this->V1 = v0;
    }
  }

  bool operator==(const EdgeTuple& et) const { return this->V0 == et.V0 && this->V1 == et.V1; }
  bool operator!=(const EdgeTuple& et) const { return !(*this == et); }

  // Whether the points of the edge are v0 and v1, in any order.
  bool IsEdge(TId v0, TId v1) const
  {
    return v0 < v1 ? this->V0 == v0 && this->V1 == v1 : this->V0 == v1 && this->V1 == v0;
  }

  // Sort on V0 first, then V1.
  bool operator<(const EdgeTuple& et) const
  {
    return this->V0 < et.V0 || (this->V0 == et.V0 && this->V1 < et.V1);
  }
};

/**
 * Definition of a triangle tuple: its 3 point ids, sorted (V0 <= V1 <= V2) by the constructor, so
 * that two triangles are equal if they have the same points, in any order. The data TData, if not
 * void, is associated with the triangle, but not used when comparing and sorting the triangles.
 */
template <typename TId, typename TData = void>
struct TriangleTuple : vtkTupleData<TData>
{
  TId V0;
  TId V1;
  TId V2;

  TriangleTuple() = default;

  // Construct a triangle with its data.
  template <typename D = TData, typename = std::enable_if_t<!std::is_void_v<D>>>
  TriangleTuple(TId v0, TId v1, TId v2, const std::common_type_t<D>& data)
    : vtkTupleData<TData>{ data }
    , V0(v0)
    , V1(v1)
    , V2(v2)
  {
    this->Sort();
  }

  // Construct a triangle without data, e.g., a query.
  TriangleTuple(TId v0, TId v1, TId v2)
    : vtkTupleData<TData>()
    , V0(v0)
    , V1(v1)
    , V2(v2)
  {
    this->Sort();
  }

  // The key of the element, see EdgeTuple::GetKey().
  TId GetKey() const { return this->V0; }

  bool operator==(const TriangleTuple& t) const
  {
    return this->V0 == t.V0 && this->V1 == t.V1 && this->V2 == t.V2;
  }
  bool operator!=(const TriangleTuple& t) const { return !(*this == t); }

  // Sort on V0 first, then V1, then V2.
  bool operator<(const TriangleTuple& t) const
  {
    if (this->V0 != t.V0)
    {
      return this->V0 < t.V0;
    }
    return this->V1 != t.V1 ? this->V1 < t.V1 : this->V2 < t.V2;
  }

private:
  // A 3-element sorting network.
  void Sort()
  {
    if (this->V0 > this->V1)
      std::swap(this->V0, this->V1);
    if (this->V1 > this->V2)
      std::swap(this->V1, this->V2);
    if (this->V0 > this->V1)
      std::swap(this->V0, this->V1);
  }
};

/**
 * Definition of a quad tuple: its 4 point ids, sorted (V0 <= V1 <= V2 <= V3) by the constructor,
 * so that two quads are equal if they have the same points, in any order. The data TData, if not
 * void, is associated with the quad, but not used when comparing and sorting the quads.
 */
template <typename TId, typename TData = void>
struct QuadTuple : vtkTupleData<TData>
{
  TId V0;
  TId V1;
  TId V2;
  TId V3;

  QuadTuple() = default;

  // Construct a quad with its data.
  template <typename D = TData, typename = std::enable_if_t<!std::is_void_v<D>>>
  QuadTuple(TId v0, TId v1, TId v2, TId v3, const std::common_type_t<D>& data)
    : vtkTupleData<TData>{ data }
    , V0(v0)
    , V1(v1)
    , V2(v2)
    , V3(v3)
  {
    this->Sort();
  }

  // Construct a quad without data, e.g., a query.
  QuadTuple(TId v0, TId v1, TId v2, TId v3)
    : vtkTupleData<TData>()
    , V0(v0)
    , V1(v1)
    , V2(v2)
    , V3(v3)
  {
    this->Sort();
  }

  // The key of the element, see EdgeTuple::GetKey().
  TId GetKey() const { return this->V0; }

  bool operator==(const QuadTuple& q) const
  {
    return this->V0 == q.V0 && this->V1 == q.V1 && this->V2 == q.V2 && this->V3 == q.V3;
  }
  bool operator!=(const QuadTuple& q) const { return !(*this == q); }

  // Sort on V0 first, then V1, then V2, then V3.
  bool operator<(const QuadTuple& q) const
  {
    if (this->V0 != q.V0)
    {
      return this->V0 < q.V0;
    }
    if (this->V1 != q.V1)
    {
      return this->V1 < q.V1;
    }
    return this->V2 != q.V2 ? this->V2 < q.V2 : this->V3 < q.V3;
  }

private:
  // A 4-element sorting network.
  void Sort()
  {
    if (this->V0 > this->V1)
      std::swap(this->V0, this->V1);
    if (this->V2 > this->V3)
      std::swap(this->V2, this->V3);
    if (this->V0 > this->V2)
      std::swap(this->V0, this->V2);
    if (this->V1 > this->V3)
      std::swap(this->V1, this->V3);
    if (this->V1 > this->V2)
      std::swap(this->V1, this->V2);
  }
};

// ============================================================================
// Common Locators
// ============================================================================

/**
 * Static locator specialized for EdgeTuple (2 vertices).
 * Defaults to vtkIdType IDs with no associated data.
 */
template <typename TId = vtkIdType, typename TData = void>
using vtkStaticEdgeLocator = vtkStaticElementLocator<TId, EdgeTuple<TId, TData>>;

/**
 * Static locator specialized for TriangleTuple (3 vertices).
 * Defaults to vtkIdType IDs with no associated data.
 */
template <typename TId = vtkIdType, typename TData = void>
using vtkStaticTriangleLocator = vtkStaticElementLocator<TId, TriangleTuple<TId, TData>>;

/**
 * Static locator specialized for QuadTuple (4 vertices).
 * Defaults to vtkIdType IDs with no associated data.
 */
template <typename TId = vtkIdType, typename TData = void>
using vtkStaticQuadLocator = vtkStaticElementLocator<TId, QuadTuple<TId, TData>>;

VTK_ABI_NAMESPACE_END
#include "vtkStaticElementLocator.txx"

#endif
// VTK-HeaderTest-Exclude: vtkStaticElementLocator.h
