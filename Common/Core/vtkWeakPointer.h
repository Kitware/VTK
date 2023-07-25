// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWeakPointer
 * @brief   a weak reference to a vtkObject.
 *
 * A weak reference to a vtkObject, which means that assigning
 * a vtkObject to the vtkWeakPointer does not affect the reference count of the
 * vtkObject. However, when the vtkObject is destroyed, the vtkWeakPointer gets
 * initialized to nullptr, thus avoiding any dangling references.
 *
 * \code
 * vtkTable *table = vtkTable::New();
 * vtkWeakPointer<vtkTable> weakTable = table;
 * \endcode
 *
 * Some time later the table may be deleted, but if it is tested for null then
 * the weak pointer will not leave a dangling pointer.
 *
 * \code
 * table->Delete();
 * if (weakTable)
 *   {
 *   // Never executed as the weak table pointer will be null here
 *   cout << "Number of columns in table: " << weakTable->GetNumberOfColumns()
 *        << endl;
 *   }
 * \endcode
 */

#ifndef vtkWeakPointer_h
#define vtkWeakPointer_h

#include "vtkWeakPointerBase.h"

#include "vtkMeta.h" // for IsComplete
#include "vtkNew.h"  // for vtkNew

#include <type_traits> // for is_base_of
#include <utility>     // for std::move

VTK_ABI_NAMESPACE_BEGIN
template <class T>
class vtkWeakPointer : public vtkWeakPointerBase
{
  // These static asserts only fire when the function calling CheckTypes is
  // used. Thus, this smart pointer class may still be used as a member variable
  // with a forward declared T, so long as T is defined by the time the calling
  // function is used.
  template <typename U = T>
  static void CheckTypes() noexcept
  {
    static_assert(vtk::detail::IsComplete<T>::value,
      "vtkWeakPointer<T>'s T type has not been defined. Missing "
      "include?");
    static_assert(vtk::detail::IsComplete<U>::value,
      "Cannot store an object with undefined type in "
      "vtkWeakPointer. Missing include?");
    static_assert(std::is_base_of<T, U>::value,
      "Argument type is not compatible with vtkWeakPointer<T>'s "
      "T type.");
    static_assert(std::is_base_of<vtkObjectBase, T>::value,
      "vtkWeakPointer can only be used with subclasses of "
      "vtkObjectBase.");
  }

public:
  /**
   * Initialize smart pointer to nullptr.
   */
  vtkWeakPointer() noexcept
    : vtkWeakPointerBase()
  {
  }

  /**
   * Initialize smart pointer with the given smart pointer.
   * @{
   */
  vtkWeakPointer(const vtkWeakPointer& r)
    : vtkWeakPointerBase(r)
  {
  }

  template <class U>
  vtkWeakPointer(const vtkWeakPointer<U>& r)
    : vtkWeakPointerBase(r)
  {
    vtkWeakPointer::CheckTypes<U>();
  }
  /* @} **/

  /**
   * Move r's object into the new weak pointer, setting r to nullptr.
   * @{
   */
  vtkWeakPointer(vtkWeakPointer&& r) noexcept
    : vtkWeakPointerBase(std::move(r))
  {
  }

  template <class U>
  vtkWeakPointer(vtkWeakPointer<U>&& r) noexcept
    : vtkWeakPointerBase(std::move(r))
  {
    vtkWeakPointer::CheckTypes<U>();
  }
  /* @} **/

  /**
   * Initialize smart pointer to given object.
   * @{
   */
  vtkWeakPointer(T* r)
    : vtkWeakPointerBase(r)
  {
    vtkWeakPointer::CheckTypes();
  }

  template <typename U>
  vtkWeakPointer(const vtkNew<U>& r)
    : vtkWeakPointerBase(r.Object)
  { // Create a new reference on copy
    vtkWeakPointer::CheckTypes<U>();
  }
  ///@}

  ///@{
  /**
   * Assign object to reference.
   */
  vtkWeakPointer& operator=(const vtkWeakPointer& r)
  {
    this->vtkWeakPointerBase::operator=(r);
    return *this;
  }

  template <class U>
  vtkWeakPointer& operator=(const vtkWeakPointer<U>& r)
  {
    vtkWeakPointer::CheckTypes<U>();

    this->vtkWeakPointerBase::operator=(r);
    return *this;
  }
  ///@}

  ///@{
  /**
   * Move r's object into this weak pointer, setting r to nullptr.
   */
  vtkWeakPointer& operator=(vtkWeakPointer&& r) noexcept
  {
    this->vtkWeakPointerBase::operator=(std::move(r));
    return *this;
  }

  template <class U>
  vtkWeakPointer& operator=(vtkWeakPointer<U>&& r) noexcept
  {
    vtkWeakPointer::CheckTypes<U>();

    this->vtkWeakPointerBase::operator=(std::move(r));
    return *this;
  }
  ///@}

  ///@{
  /**
   * Assign object to reference.
   */
  vtkWeakPointer& operator=(T* r)
  {
    vtkWeakPointer::CheckTypes();
    this->vtkWeakPointerBase::operator=(r);
    return *this;
  }

  template <typename U>
  vtkWeakPointer& operator=(const vtkNew<U>& r)
  {
    vtkWeakPointer::CheckTypes<U>();

    this->vtkWeakPointerBase::operator=(r.Object);
    return *this;
  }
  ///@}

  ///@{
  /**
   * Get the contained pointer.
   */
  T* GetPointer() const noexcept { return static_cast<T*>(this->Object); }
  T* Get() const noexcept { return static_cast<T*>(this->Object); }
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

  // Work-around for HP and IBM overload resolution bug.  Since
  // NullPointerOnly is a private type the only pointer value that can
  // be passed by user code is a null pointer.  This operator will be
  // chosen by the compiler when comparing against null explicitly and
  // avoid the bogus ambiguous overload error.
#if defined(__HP_aCC) || defined(__IBMCPP__)
#define VTK_WEAK_POINTER_DEFINE_OPERATOR_WORKAROUND(op)                                            \
  bool operator op(NullPointerOnly*) const { return ::operator op(*this, 0); }

private:
  class NullPointerOnly
  {
  };

public:
  VTK_WEAK_POINTER_DEFINE_OPERATOR_WORKAROUND(==)
  VTK_WEAK_POINTER_DEFINE_OPERATOR_WORKAROUND(!=)
  VTK_WEAK_POINTER_DEFINE_OPERATOR_WORKAROUND(<)
  VTK_WEAK_POINTER_DEFINE_OPERATOR_WORKAROUND(<=)
  VTK_WEAK_POINTER_DEFINE_OPERATOR_WORKAROUND(>)
  VTK_WEAK_POINTER_DEFINE_OPERATOR_WORKAROUND(>=)
#undef VTK_WEAK_POINTER_DEFINE_OPERATOR_WORKAROUND
#endif
protected:
  vtkWeakPointer(T* r, const NoReference& n)
    : vtkWeakPointerBase(r, n)
  {
  }

private:
  // These are purposely not implemented to prevent callers from
  // trying to take references from other smart pointers.
  void TakeReference(const vtkWeakPointerBase&) = delete;
  static void Take(const vtkWeakPointerBase&) = delete;
};

#define VTK_WEAK_POINTER_DEFINE_OPERATOR(op)                                                       \
  template <class T, class U>                                                                      \
  inline bool operator op(const vtkWeakPointer<T>& l, const vtkWeakPointer<U>& r)                  \
  {                                                                                                \
    return (l.GetPointer() op r.GetPointer());                                                     \
  }                                                                                                \
  template <class T, class U>                                                                      \
  inline bool operator op(T* l, const vtkWeakPointer<U>& r)                                        \
  {                                                                                                \
    return (l op r.GetPointer());                                                                  \
  }                                                                                                \
  template <class T, class U>                                                                      \
  inline bool operator op(const vtkWeakPointer<T>& l, U* r)                                        \
  {                                                                                                \
    return (l.GetPointer() op r);                                                                  \
  }                                                                                                \
  template <class T, class U>                                                                      \
  inline bool operator op(const vtkNew<T>& l, const vtkWeakPointer<U>& r)                          \
  {                                                                                                \
    return (l.GetPointer() op r.GetPointer());                                                     \
  }                                                                                                \
  template <class T, class U>                                                                      \
  inline bool operator op(const vtkWeakPointer<T>& l, const vtkNew<U>& r)                          \
  {                                                                                                \
    return (l.GetPointer() op r.GetPointer);                                                       \
  }

/**
 * Compare smart pointer values.
 */
VTK_WEAK_POINTER_DEFINE_OPERATOR(==)
VTK_WEAK_POINTER_DEFINE_OPERATOR(!=)
VTK_WEAK_POINTER_DEFINE_OPERATOR(<)
VTK_WEAK_POINTER_DEFINE_OPERATOR(<=)
VTK_WEAK_POINTER_DEFINE_OPERATOR(>)
VTK_WEAK_POINTER_DEFINE_OPERATOR(>=)

#undef VTK_WEAK_POINTER_DEFINE_OPERATOR

VTK_ABI_NAMESPACE_END

namespace vtk
{
VTK_ABI_NAMESPACE_BEGIN

/// Construct a vtkWeakPointer<T> containing @a obj. @a obj's reference count
/// is not changed.
template <typename T>
vtkWeakPointer<T> TakeWeakPointer(T* obj)
{
  return vtkWeakPointer<T>(obj);
}

VTK_ABI_NAMESPACE_END
} // end namespace vtk

VTK_ABI_NAMESPACE_BEGIN
/**
 * Streaming operator to print smart pointer like regular pointers.
 */
template <class T>
inline ostream& operator<<(ostream& os, const vtkWeakPointer<T>& p)
{
  return os << static_cast<const vtkWeakPointerBase&>(p);
}

VTK_ABI_NAMESPACE_END
#endif

// VTK-HeaderTest-Exclude: vtkWeakPointer.h
