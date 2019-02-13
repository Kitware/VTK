/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRange.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkRange_h
#define vtkRange_h

#include "vtkMeta.h"
#include "vtkRangeIterableTraits.h"

#include <iterator>
#include <type_traits>
#include <utility>

namespace vtk
{

/**
 * Generate an iterable STL proxy object for a VTK container.
 *
 * Currently supports:
 *
 * - vtkCollection and subclasses (`#include <vtkCollectionRange.h>`):
 *   - ItemType is the (non-pointer) result type of GetNextItem() if this method
 *     exists on the collection type, otherwise vtkObject is used.
 *   - Iterators fulfill the STL InputIterator concept with some exceptions:
 *     - Const iterators/references are mutable, since vtkObjects are generally
 *       unusable when const.
 *     - Value/pointer/reference types are just ItemType*, since:
 *       - Plain ItemType wouldn't be usable (vtkObjects cannot be
 *         copied/assigned)
 *       - ItemType*& references aren't generally desired.
 *       - ItemType& references are unconventional for vtkObjects.
 *       - ItemType** pointers are unruly.
 *
 * - vtkCompositeDataSet (`#include <vtkCompositeDataSetRange.h>`)
 *   - vtk::CompositeDataSetOptions: None, SkipEmptyNodes.
 *     - Ex. vtk::Range(compDS, vtk::CompositeDataSetOptions::SkipEmptyNodes);
 *   - Reverse iteration is not supported. Use vtkCompositeDataIterator directly
 *     instead for this.
 *   - Dereferencing the iterator yeilds a vtk::CompositeDataSetNodeReference
 *     that provides additional API to get the node's flat index, data object,
 *     and metadata. See that class's documentation for more information.
 *
 * - vtkDataObjectTree (`#include <vtkDataObjectTreeRange.h>`)
 *   - vtk::DataObjectTreeOptions:
 *     None, SkipEmptyNodes, VisitOnlyLeaves, TraverseSubTree.
 *     - Ex. vtk::Range(dObjTree, vtk::DataObjectTreeOptions::TraverseSubTree |
 *                                vtk::DataObjectTreeOptions::SkipEmptyNodes);
 *   - Reverse iteration is not supported. Use vtkDataObjectTreeIterator
 *     directly instead for this.
 *   - Dereferencing the iterator yeilds a vtk::CompositeDataSetNodeReference
 *     that provides additional API to get the node's flat index, data object,
 *     and metadata. See that class's documentation for more information.
 *
 * Usage:
 *
 * ```
 * for (auto item : vtk::Range(myCollection))
 * {
 *   // Use item.
 * }
 *
 * // or:
 *
 * using Opts = vtk::vtkDataObjectTreeOptions;
 * auto range = vtk::Range(dataObjTree,
 *                         Opts::TraverseSubTree | Opts::VisitOnlyLeaves);
 * some_algo(range.begin(), range.end());
 *
 * ```
 */
template <typename IterablePtr, typename... Options>
auto Range(IterablePtr iterable, Options&&... opts)
-> typename detail::IterableTraits<
       typename detail::StripPointers<IterablePtr>::type
   >::RangeType
{
  using Iterable = typename detail::StripPointers<IterablePtr>::type;
  using RangeType = typename detail::IterableTraits<Iterable>::RangeType;
  return RangeType{iterable, std::forward<Options>(opts)...};
}

} // end namespace vtk

#endif // vtkRange_h

// VTK-HeaderTest-Exclude: vtkRange.h
