// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkStaticElementLocator_txx
#define vtkStaticElementLocator_txx

#include "vtkStaticElementLocator.h"

#include "vtkSMPTools.h"

#include <algorithm>
#include <memory>
#include <utility>

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
template <typename TID, typename TElement>
vtkIdType vtkStaticElementLocator<TID, TElement>::BuildLocator(
  vtkIdType numElements, TElement* elementArray)
{
  this->NumElements = 0;
  this->ElementArray = nullptr;
  if (numElements <= 0 || !elementArray)
  {
    this->Index.Reset();
    return 0;
  }

  // 1. Scatter the elements by key into a copy, keeping the unique elements at the front of every
  // key. Default-initialized, so that trivial elements are not filled before being scattered.
  std::unique_ptr<TElement[]> scattered(new TElement[numElements]);
  const auto keys = vtk::keys::ScatterDense(elementArray, numElements, scattered.get());
  auto unique = vtk::keys::Reduce(keys, scattered.get(), vtk::keys::SortUnique{});

  // 2. Compact the unique elements to the front of elementArray, so that they are numbered
  // contiguously, keeping their keys as the search index: the scattered keys are released.
  const vtkIdType numUnique = vtk::keys::Compact(keys, unique, scattered.get(), elementArray);
  this->Index = std::move(unique);
  // The unique elements fit TID, see the class warning.
  this->NumElements = static_cast<TID>(numUnique);
  this->ElementArray = elementArray;
  return numUnique;
}

//----------------------------------------------------------------------------
template <typename TID, typename TElement>
TID vtkStaticElementLocator<TID, TElement>::IsInsertedElement(const TElement& query) const
{
  if (this->NumElements == 0)
  {
    return -1;
  }

  const TID key = query.GetKey();
  if (key < this->Index.GetMinKey() || key > this->Index.GetMaxKey())
  {
    return -1;
  }

  // The elements of the key are contiguous and sorted, so the scan stops at the first greater one.
  const TID keyId = this->Index.GetKeyId(key);
  // The unique elements fit TID, see the class warning.
  auto curId = static_cast<TID>(this->Index.GetOffset(keyId));
  const auto keyEnd = static_cast<TID>(this->Index.GetOffset(keyId + 1));
  for (; curId < keyEnd; ++curId)
  {
    if (this->ElementArray[curId] == query)
    {
      return curId;
    }
    if (query < this->ElementArray[curId])
    {
      return -1;
    }
  }
  return -1;
}

//----------------------------------------------------------------------------
template <typename TID, typename TElement>
const typename vtkStaticElementLocator<TID, TElement>::GroupedElements&
vtkStaticElementLocator<TID, TElement>::GroupElements(vtkIdType numElements, TElement* elementArray)
{
  // The elements cannot be binned in place: they are binned from a copy.
  std::unique_ptr<TElement[]> input(new TElement[std::max<vtkIdType>(numElements, 0)]);
  vtkSMPTools::For(0, numElements, [&](vtkIdType begin, vtkIdType end)
    { std::copy(elementArray + begin, elementArray + end, input.get() + begin); });
  // Only the groups are kept: the keys of the groups are released.
  const auto keys = vtk::keys::ScatterDense(input.get(), numElements, elementArray);
  this->Groups = vtk::keys::Group(keys, elementArray, vtk::keys::SortGroup{}).Groups;
  return this->Groups;
}

VTK_ABI_NAMESPACE_END

#endif // vtkStaticElementLocator_txx
