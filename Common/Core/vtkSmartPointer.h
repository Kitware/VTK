// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSmartPointer
 * @brief   Hold a reference to a vtkObjectBase instance.
 *
 * vtkSmartPointer is a class template that provides automatic casting
 * for objects held by the vtkSmartPointerBase superclass.
 */

#ifndef vtkSmartPointer_h
#define vtkSmartPointer_h

#include "vtkSmartPointerBase.h"

#include "vtkMeta.h" // for IsComplete
#include "vtkNew.h"  // for vtkNew.h

#include <functional>  // for std::hash
#include <type_traits> // for is_base_of
#include <utility>     // for std::move

VTK_ABI_NAMESPACE_BEGIN
template <class T>
class vtkSmartPointer : public vtkSmartPointerBase
{
  // These static asserts only fire when the function calling CheckTypes is
  // used. Thus, this smart pointer class may still be used as a member variable
  // with a forward declared T, so long as T is defined by the time the calling
  // function is used.
  template <typename U = T>
  static void CheckTypes() noexcept
  {
    static_assert(vtk::detail::IsComplete<T>::value,
      "vtkSmartPointer<T>'s T type has not been defined. Missing "
      "include?");
    static_assert(vtk::detail::IsComplete<U>::value,
      "Cannot store an object with undefined type in "
      "vtkSmartPointer. Missing include?");
    static_assert(std::is_base_of<T, U>::value,
      "Argument type is not compatible with vtkSmartPointer<T>'s "
      "T type.");
    static_assert(std::is_base_of<vtkObjectBase, T>::value,
      "vtkSmartPointer can only be used with subclasses of "
      "vtkObjectBase.");
  }

public:
  /**
   * Initialize smart pointer to nullptr.
   */
  vtkSmartPointer() noexcept
    : vtkSmartPointerBase()
  {
  }

  /**
   * Initialize smart pointer with a new reference to the same object
   * referenced by given smart pointer.
   * @{
   */
  // Need both overloads because the copy-constructor must be non-templated:
  vtkSmartPointer(const vtkSmartPointer& r)
    : vtkSmartPointerBase(r)
  {
  }

  template <class U>
  vtkSmartPointer(const vtkSmartPointer<U>& r)
    : vtkSmartPointerBase(r)
  {
    vtkSmartPointer::CheckTypes<U>();
  }
  /* @} **/

  /**
   * Move the contents of @a r into @a this.
   * @{
   */
  // Need both overloads because the move-constructor must be non-templated:
  vtkSmartPointer(vtkSmartPointer&& r) noexcept
    : vtkSmartPointerBase(std::move(r))
  {
  }

  template <class U>
  vtkSmartPointer(vtkSmartPointer<U>&& r) noexcept
    : vtkSmartPointerBase(std::move(r))
  {
    vtkSmartPointer::CheckTypes<U>();
  }
  /**@}*/

  /**
   * Initialize smart pointer to given object.
   * @{
   */
  vtkSmartPointer(T* r)
    : vtkSmartPointerBase(r)
  {
    vtkSmartPointer::CheckTypes();
  }

  template <typename U>
  vtkSmartPointer(const vtkNew<U>& r)
    : vtkSmartPointerBase(r.Object)
  { // Create a new reference on copy
    vtkSmartPointer::CheckTypes<U>();
  }
  ///@}

  /**
   * Move the pointer from the vtkNew smart pointer to the new vtkSmartPointer,
   * stealing its reference and resetting the vtkNew object to nullptr.
   */
  template <typename U>
  vtkSmartPointer(vtkNew<U>&& r) noexcept
    : vtkSmartPointerBase(r.Object, vtkSmartPointerBase::NoReference{})
  { // Steal the reference on move
    vtkSmartPointer::CheckTypes<U>();

    r.Object = nullptr;
  }

  ///@{
  /**
   * Assign object to reference.  This removes any reference to an old
   * object.
   */
  // Need this since the compiler won't recognize template functions as
  // assignment operators.
  vtkSmartPointer& operator=(const vtkSmartPointer& r)
  {
    this->vtkSmartPointerBase::operator=(r.GetPointer());
    return *this;
  }

  template <class U>
  vtkSmartPointer& operator=(const vtkSmartPointer<U>& r)
  {
    vtkSmartPointer::CheckTypes<U>();

    this->vtkSmartPointerBase::operator=(r.GetPointer());
    return *this;
  }
  ///@}

  /**
   * Assign object to reference.  This removes any reference to an old
   * object.
   */
  template <typename U>
  vtkSmartPointer& operator=(const vtkNew<U>& r)
  {
    vtkSmartPointer::CheckTypes<U>();

    this->vtkSmartPointerBase::operator=(r.Object);
    return *this;
  }

  /**
   * Assign object to reference.  This adds a new reference to an old
   * object.
   */
  template <typename U>
  vtkSmartPointer& operator=(U* r)
  {
    vtkSmartPointer::CheckTypes<U>();

    this->vtkSmartPointerBase::operator=(r);
    return *this;
  }

  ///@{
  /**
   * Get the contained pointer.
   */
  T* GetPointer() const noexcept { return static_cast<T*>(this->Object); }
  T* Get() const noexcept { return static_cast<T*>(this->Object); }
  ///@}

  /**
   * Get the contained pointer.
   */
  operator T*() const noexcept { return static_cast<T*>(this->Object); }

  /**
   * Dereference the pointer and return a reference to the contained
   * object.
   */
  T& operator*() const noexcept { return *static_cast<T*>(this->Object); }

  /**
   * Provides normal pointer target member access using operator ->.
   */
  T* operator->() const noexcept { return static_cast<T*>(this->Object); }

  /**
   * Transfer ownership of one reference to the given VTK object to
   * this smart pointer.  This does not increment the reference count
   * of the object, but will decrement it later.  The caller is
   * effectively passing ownership of one reference to the smart
   * pointer.  This is useful for code like:

   * vtkSmartPointer<vtkFoo> foo;
   * foo.TakeReference(bar->NewFoo());

   * The input argument may not be another smart pointer.
   */
  void TakeReference(T* t) { *this = vtkSmartPointer<T>(t, NoReference()); }

  ///@{
  /**
   * Create an instance of a VTK object.
   */
  static vtkSmartPointer<T> New() { return vtkSmartPointer<T>(T::New(), NoReference()); }
  template <class... ArgsT>
  static vtkSmartPointer<T> New(ArgsT&&... args)
  {
    return vtkSmartPointer<T>(T::New(std::forward<ArgsT>(args)...), NoReference());
  }
  ///@}

  /**
   * Create an instance of a VTK object in a memkind extended memory space. Note that not all
   * vtkObjects support this yet and that VTK needs to be compiled with VTK_USE_MEMKIND to enable
   * those that do. If not enabled, this is equivalent to calling New()
   */
  static vtkSmartPointer<T> ExtendedNew()
  {
    return vtkSmartPointer<T>(T::ExtendedNew(), NoReference());
  }

  /**
   * Create a new instance of the given VTK object.
   */
  static vtkSmartPointer<T> NewInstance(T* t)
  {
    return vtkSmartPointer<T>(t->NewInstance(), NoReference());
  }

  /**
   * Transfer ownership of one reference to the given VTK object to a
   * new smart pointer.  The returned smart pointer does not increment
   * the reference count of the object on construction but will
   * decrement it on destruction.  The caller is effectively passing
   * ownership of one reference to the smart pointer.  This is useful
   * for code like:

   * vtkSmartPointer<vtkFoo> foo =
   * vtkSmartPointer<vtkFoo>::Take(bar->NewFoo());

   * The input argument may not be another smart pointer.
   */
  static vtkSmartPointer<T> Take(T* t) { return vtkSmartPointer<T>(t, NoReference()); }

  // Work-around for HP and IBM overload resolution bug.  Since
  // NullPointerOnly is a private type the only pointer value that can
  // be passed by user code is a null pointer.  This operator will be
  // chosen by the compiler when comparing against null explicitly and
  // avoid the bogus ambiguous overload error.
#if defined(__HP_aCC) || defined(__IBMCPP__)
#define VTK_SMART_POINTER_DEFINE_OPERATOR_WORKAROUND(op)                                           \
  bool operator op(NullPointerOnly*) const { return ::operator op(*this, 0); }

private:
  class NullPointerOnly
  {
  };

public:
  VTK_SMART_POINTER_DEFINE_OPERATOR_WORKAROUND(==)
  VTK_SMART_POINTER_DEFINE_OPERATOR_WORKAROUND(!=)
  VTK_SMART_POINTER_DEFINE_OPERATOR_WORKAROUND(<)
  VTK_SMART_POINTER_DEFINE_OPERATOR_WORKAROUND(<=)
  VTK_SMART_POINTER_DEFINE_OPERATOR_WORKAROUND(>)
  VTK_SMART_POINTER_DEFINE_OPERATOR_WORKAROUND(>=)
#undef VTK_SMART_POINTER_DEFINE_OPERATOR_WORKAROUND
#endif
protected:
  vtkSmartPointer(T* r, const NoReference& n)
    : vtkSmartPointerBase(r, n)
  {
  }

private:
  // These are purposely not implemented to prevent callers from
  // trying to take references from other smart pointers.
  void TakeReference(const vtkSmartPointerBase&) = delete;
  static void Take(const vtkSmartPointerBase&) = delete;
};
VTK_ABI_NAMESPACE_END

namespace std
{
template <class T>
struct hash<vtkSmartPointer<T>>
{
  std::size_t operator()(const vtkSmartPointer<T>& p) const { return this->Hasher(p.Get()); }

  std::hash<T*> Hasher;
};
}

VTK_ABI_NAMESPACE_BEGIN
#define VTK_SMART_POINTER_DEFINE_OPERATOR(op)                                                      \
  template <class T, class U>                                                                      \
  inline bool operator op(const vtkSmartPointer<T>& l, const vtkSmartPointer<U>& r)                \
  {                                                                                                \
    return (l.GetPointer() op r.GetPointer());                                                     \
  }                                                                                                \
  template <class T, class U>                                                                      \
  inline bool operator op(T* l, const vtkSmartPointer<U>& r)                                       \
  {                                                                                                \
    return (l op r.GetPointer());                                                                  \
  }                                                                                                \
  template <class T, class U>                                                                      \
  inline bool operator op(const vtkSmartPointer<T>& l, U* r)                                       \
  {                                                                                                \
    return (l.GetPointer() op r);                                                                  \
  }                                                                                                \
  template <class T, class U>                                                                      \
  inline bool operator op(const vtkNew<T>& l, const vtkSmartPointer<U>& r)                         \
  {                                                                                                \
    return (l.GetPointer() op r.GetPointer());                                                     \
  }                                                                                                \
  template <class T, class U>                                                                      \
  inline bool operator op(const vtkSmartPointer<T>& l, const vtkNew<U>& r)                         \
  {                                                                                                \
    return (l.GetPointer() op r.GetPointer);                                                       \
  }

/**
 * Compare smart pointer values.
 */
VTK_SMART_POINTER_DEFINE_OPERATOR(==)
VTK_SMART_POINTER_DEFINE_OPERATOR(!=)
VTK_SMART_POINTER_DEFINE_OPERATOR(<)
VTK_SMART_POINTER_DEFINE_OPERATOR(<=)
VTK_SMART_POINTER_DEFINE_OPERATOR(>)
VTK_SMART_POINTER_DEFINE_OPERATOR(>=)

#undef VTK_SMART_POINTER_DEFINE_OPERATOR
VTK_ABI_NAMESPACE_END

namespace vtk
{
VTK_ABI_NAMESPACE_BEGIN

/// Construct a vtkSmartPointer<T> containing @a obj. A new reference is added
/// to @a obj.
template <typename T>
vtkSmartPointer<T> MakeSmartPointer(T* obj)
{
  return vtkSmartPointer<T>{ obj };
}

/// Construct a vtkSmartPointer<T> containing @a obj. @a obj's reference count
/// is not changed.
template <typename T>
vtkSmartPointer<T> TakeSmartPointer(T* obj)
{
  return vtkSmartPointer<T>::Take(obj);
}

VTK_ABI_NAMESPACE_END
} // end namespace vtk

VTK_ABI_NAMESPACE_BEGIN
/**
 * Streaming operator to print smart pointer like regular pointers.
 */
template <class T>
inline ostream& operator<<(ostream& os, const vtkSmartPointer<T>& p)
{
  return os << static_cast<const vtkSmartPointerBase&>(p);
}

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkSmartPointer.h
