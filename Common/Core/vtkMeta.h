/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMeta.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkMeta_h
#define vtkMeta_h

#include "vtkConfigure.h"

#include <cassert>
#include <type_traits>
#include <utility>

/**
 * @file vtkMeta
 * This file contains a variety of metaprogramming constructs for working
 * with vtk types.
 */

// When enabled, extra debugging checks are enabled for the iterators. This is
// only effective in debugging mode. Specifically:
// - Specializations are disabled (All code uses the generic implementation).
// - Additional assertions are inserted to ensure correct runtime usage.
#ifdef VTK_DEBUG_RANGE_ITERATORS
#define VTK_ITER_ASSERT(x, msg) assert((x) && msg)
#else
#define VTK_ITER_ASSERT(x, msg)
#endif

// Forward decs for StripPointers:
template <typename ArrayType> class vtkNew;
template <typename ArrayType> class vtkSmartPointer;

namespace vtk
{
namespace detail
{

//------------------------------------------------------------------------------
// Strip vtkNew, vtkSmartPointer, etc from a type.
template <typename T>
struct StripPointers
{
  using type = T;
};

template <typename T>
struct StripPointers<T*>
{
  using type = T;
};

template <typename ArrayType>
struct StripPointers<vtkNew<ArrayType>>
{
  using type = ArrayType;
};

template <typename ArrayType>
struct StripPointers<vtkSmartPointer<ArrayType>>
{
  using type = ArrayType;
};

//------------------------------------------------------------------------------
// Test if a type is complete, or if a specialization exists.
template <typename T>
struct IsComplete
{
private:
  // Can't take the sizeof an incomplete class.
  template <typename U, std::size_t = sizeof(U)>
  static std::true_type impl(U*);
  static std::false_type impl(...);

public:
  using value = decltype(impl(std::declval<T*>()));
};

// Traits class that should define RangeType, which would point to the range
// proxy for Iterable. This is only used by vtk::Range calls.
// It is included here to ensure that the stub is defined before any
// specializations.
template <typename Iterable, typename /*SFINAE stub*/>
struct IterableTraits;

}
} // end namespace vtk::detail

#endif // vtkMeta_h

// VTK-HeaderTest-Exclude: vtkMeta.h
