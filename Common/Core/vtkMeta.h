// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkMeta_h
#define vtkMeta_h

#include "vtkABINamespace.h"

#include <type_traits>
#include <utility>

/**
 * @file vtkMeta.h
 * This file contains a variety of metaprogramming constructs for working
 * with vtk types.
 */

// Forward decs for StripPointers:
VTK_ABI_NAMESPACE_BEGIN
template <typename ArrayType>
class vtkNew;
template <typename ArrayType>
class vtkSmartPointer;
template <typename ArrayType>
class vtkWeakPointer;
VTK_ABI_NAMESPACE_END

namespace vtk
{
namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

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

template <typename ArrayType>
struct StripPointers<vtkWeakPointer<ArrayType>>
{
  using type = ArrayType;
};

//------------------------------------------------------------------------------
// Test if a type is defined (true) or just forward declared (false).
template <typename T>
struct IsComplete
{
private:
  // Can't take the sizeof an incomplete class.
  template <typename U, std::size_t = sizeof(U)>
  static std::true_type impl(U*);
  static std::false_type impl(...);
  using bool_constant = decltype(impl(std::declval<T*>()));

public:
  static constexpr bool value = bool_constant::value;
};

VTK_ABI_NAMESPACE_END
}
} // end namespace vtk::detail

#endif // vtkMeta_h

// VTK-HeaderTest-Exclude: vtkMeta.h
