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

#include "vtkCollectionRange.h"
#include "vtkMeta.h"

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
 * - vtkCollection:
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
 * auto range = vtk::Range(myCollection);
 * some_algo(range.begin(), range.end());
 *
 * ```
 */
template <typename IterablePtr>
auto Range(IterablePtr iterable)
-> typename detail::IterableTraits<
       typename detail::StripPointers<IterablePtr>::type
   >::RangeType
{
  using Iterable = typename detail::StripPointers<IterablePtr>::type;
  using RangeType = typename detail::IterableTraits<Iterable>::RangeType;
  return RangeType{iterable};
}

} // end namespace vtk

#endif // vtkRange_h

// VTK-HeaderTest-Exclude: vtkRange.h
