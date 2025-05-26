// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkEmbindSmartPointerTrait_h
#define vtkEmbindSmartPointerTrait_h

#include "vtkSmartPointer.h"

#include <emscripten/bind.h>

namespace emscripten
{
// Teaches emscripten to work with vtkSmartPointer. All vtkObjects will
// be constructed through vtkSmartPointer<T>::New
template <typename T>
struct smart_ptr_trait<vtkSmartPointer<T>>
{
  using pointer_type = vtkSmartPointer<T>;
  using element_type = T;

  static sharing_policy get_sharing_policy()
  {
    // Intrusive because element_type is a vtkObject derived instance which
    // keeps reference count.
    return sharing_policy::INTRUSIVE;
  }

  static element_type* get(const pointer_type& p) { return p.Get(); }

  static pointer_type share(const pointer_type&, T* ptr) { return pointer_type(ptr); }

  static pointer_type* construct_null() { return new pointer_type; }
};
} // namespace emscripten

namespace vtk
{
// Similar to std::make_shared<T>. Used to construct VTK objects as smart pointers by default.
template <typename T>
vtkSmartPointer<T> MakeAvtkSmartPointer()
{
  return vtkSmartPointer<T>::New();
}
} // vtk

#endif
// VTK-HeaderTest-Exclude: vtkEmbindSmartPointerTrait.h
