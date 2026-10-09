// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStaticElementLocator
 * @brief   templated locator of topological elements, with data on the elements
 *
 * vtkStaticElementLocator locates topological elements, i.e., tuples of point ids, e.g., an edge
 * (v0, v1) with v0 < v1, with data associated with them, e.g., to find the duplicate edges of an
 * isocontour. It is templated on the type of the ids (IDType) and the element tuple (ElementType),
 * which carries the data.
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

#include <utility> // For std::forward, std::swap

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

VTK_ABI_NAMESPACE_END
#include "vtkStaticElementLocator.txx"

#endif
// VTK-HeaderTest-Exclude: vtkStaticElementLocator.h
