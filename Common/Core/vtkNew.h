// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkNew
 * @brief   Allocate and hold a VTK object.
 *
 * vtkNew is a class template that on construction allocates and
 * initializes an instance of its template argument using T::New().
 * It assumes ownership of one reference during its lifetime, and calls
 * T->Delete() on destruction.
 *
 * Automatic casting to raw pointer is available for convenience, but
 * users of this method should ensure that they do not
 * return this pointer if the vtkNew will go out of scope without
 * incrementing its reference count.
 *
 * vtkNew is a drop in replacement for vtkSmartPointer, for example,
 *
 * \code
 * vtkNew<vtkRenderer> ren;
 * vtkNew<vtkRenderWindow> renWin;
 * renWin->AddRenderer(ren);
 * vtkNew<vtkRenderWindowInteractor> iren;
 * iren->SetRenderWindow(renWin);
 * \endcode
 *
 *
 * @sa
 * vtkSmartPointer vtkWeakPointer
 */

#ifndef vtkNew_h
#define vtkNew_h

#include "vtkIOStream.h"
#include "vtkMeta.h" // for IsComplete

#include <type_traits> // for is_base_of

VTK_ABI_NAMESPACE_BEGIN
class vtkObjectBase;
class vtkGarbageCollector;

template <class T>
class vtkNew;

template <class T>
void vtkGarbageCollectorReport(vtkGarbageCollector* collector, vtkNew<T>& ptr, const char* desc);

template <class T>
class vtkNew
{
  // Allow other smart pointers friend access:
  template <typename U>
  friend class vtkNew;
  template <typename U>
  friend class vtkSmartPointer;
  template <typename U>
  friend class vtkWeakPointer;

  // These static asserts only fire when the function calling CheckTypes is
  // used. Thus, this smart pointer class may still be used as a member variable
  // with a forward declared T, so long as T is defined by the time the calling
  // function is used.
  template <typename U = T>
  static void CheckTypes() noexcept
  {
    static_assert(vtk::detail::IsComplete<T>::value,
      "vtkNew<T>'s T type has not been defined. Missing include?");
    static_assert(vtk::detail::IsComplete<U>::value,
      "Cannot store an object with undefined type in "
      "vtkNew. Missing include?");
    static_assert(std::is_base_of<T, U>::value,
      "Argument type is not compatible with vtkNew<T>'s "
      "T type.");
    static_assert(std::is_base_of<vtkObjectBase, T>::value,
      "vtkNew can only be used with subclasses of vtkObjectBase.");
  }

public:
  /**
   * Create a new T on construction.
   */
  vtkNew()
    : Object(T::New())
  {
    vtkNew::CheckTypes();
  }

  /**
   * Move the object into the constructed vtkNew wrapper, stealing its
   * reference. The argument is reset to nullptr.
   * @{
   */
  vtkNew(vtkNew&& o) noexcept
    : Object(o.Object)
  {
    o.Object = nullptr;
  }

  template <typename U>
  vtkNew(vtkNew<U>&& o) noexcept
    : Object(o.Object)
  {
    vtkNew::CheckTypes<U>();

    o.Object = nullptr;
  }
  ///@}

  ///@{
  /**
   * Deletes reference to instance of T.
   */
  ~vtkNew() { this->Reset(); }

  void Reset()
  {
    T* obj = this->Object;
    if (obj)
    {
      this->Object = nullptr;
      obj->Delete();
    }
  }
  ///@}

  /**
   * Enable pointer-like dereference syntax. Returns a pointer to the contained
   * object.
   */
  T* operator->() const noexcept { return this->Object; }

  ///@{
  /**
   * Get a raw pointer to the contained object. When using this function be
   * careful that the reference count does not drop to 0 when using the pointer
   * returned. This will happen when the vtkNew object goes out of
   * scope for example.
   */
  T* GetPointer() const noexcept { return this->Object; }
  T* Get() const noexcept { return this->Object; }
  operator T*() const noexcept { return static_cast<T*>(this->Object); }
  ///@}
  /**
   * Dereference the pointer and return a reference to the contained object.
   * When using this function be careful that the reference count does not
   * drop to 0 when using the pointer returned.
   * This will happen when the vtkNew object goes out of scope for example.
   */
  T& operator*() const noexcept { return *static_cast<T*>(this->Object); }

  /**
   * Move assignment operator.
   */
  vtkNew<T>& operator=(vtkNew<T>&& other) noexcept
  {
    this->Reset();
    this->Object = other.Object;
    other.Object = nullptr;
    return *this;
  }

private:
  vtkNew(vtkNew<T> const&) = delete;
  void operator=(vtkNew<T> const&) = delete;
  friend void vtkGarbageCollectorReport<T>(
    vtkGarbageCollector* collector, vtkNew<T>& ptr, const char* desc);
  T* Object;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkNew.h
