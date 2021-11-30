// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWeakPtr
 * @brief   a weak reference to a vtkObjectBase.
 *
 * A weak reference to a vtkObjectBase, which means that assigning a
 * vtkObjectBase to the vtkWeakPtr does not affect the reference count of the
 * vtkObjectBase.
 *
 * \code
 * vtkTable *table = vtkTable::New();
 * vtkWeakPtr<vtkTable> weakTable = table;
 * \endcode
 *
 * Some time later the table may be deleted, but if it is tested for null then
 * the weak pointer will not leave a dangling pointer.
 *
 * \code
 * table->Delete();
 * if (auto strongTable = weakTable.Lock())
 * {
 *   // Never executed as the weak table pointer will be null here
 *   cout << "Number of columns in table: " << strongTable->GetNumberOfColumns()
 *        << endl;
 * }
 * \endcode
 *
 * Note that, unlike vtkWeakPointer, the pointer itself is only accessible
 * after calling lock to avoid the situation of another thread deleting the
 * last instance between the check and its usage inside the conditional.
 */

#ifndef vtkWeakPtr_h
#define vtkWeakPtr_h

#include "vtkMeta.h"         // for IsComplete
#include "vtkObjectBase.h"   // for vtkObjectBase
#include "vtkSmartPointer.h" // for vtkSmartPointer

#include <memory>      // for std::shared_ptr
#include <mutex>       // for std::mutex
#include <type_traits> // for std::is_base_of

VTK_ABI_NAMESPACE_BEGIN

template <typename T>
class vtkWeakPtr
{
  // These static asserts only fire when the function calling CheckTypes is
  // used. Thus, this smart pointer class may still be used as a member variable
  // with a forward declared T, so long as T is defined by the time the calling
  // function is used.
  template <typename U = T>
  static void CheckTypes() noexcept
  {
    static_assert(vtk::detail::IsComplete<T>::value,
      "vtkWeakPtr<T>'s T type has not been defined. Missing include?");
    static_assert(vtk::detail::IsComplete<U>::value,
      "Cannot store an object with undefined type in vtkWeakPtr. Missing include?");
    static_assert(
      std::is_base_of<T, U>::value, "Argument type is not compatible with vtkWeakPtr<T>'s T type.");
    static_assert(std::is_base_of<vtkObjectBase, T>::value,
      "vtkWeakPtr can only be used with subclasses of vtkObjectBase.");
  }

public:
  ///@{
  /**
   * Default construction.
   */
  vtkWeakPtr() noexcept { vtkWeakPtr::CheckTypes<T>(); }
  ///@}

  ///@{
  /**
   * Pointer construction and assignment.
   *
   * Only `vtkNew` and `vtkSmartPointer` instance may be used with the default
   * constructors and assignments because these classes imply that the caller
   * has a strong ownership in the object which guarantees it will exist for at
   * least the duration of the `vtkWeakPtr` constructor.
   */
  vtkWeakPtr(const vtkNew<T>& r)
    : Block(r ? r->GetWeakControlBlock() : nullptr)
  {
    vtkWeakPtr::CheckTypes<T>();
  }
  template <typename U>
  vtkWeakPtr(vtkNew<U>& r)
    : Block(r ? r->GetWeakControlBlock() : nullptr)
  {
    vtkWeakPtr::CheckTypes<U>();
  }
  vtkWeakPtr& operator=(vtkNew<T>& r)
  {
    this->Block = r ? r->GetWeakControlBlock() : nullptr;
    vtkWeakPtr::CheckTypes<T>();
    return *this;
  }
  template <typename U>
  vtkWeakPtr& operator=(vtkNew<U>& r)
  {
    this->Block = r ? r->GetWeakControlBlock() : nullptr;
    vtkWeakPtr::CheckTypes<U>();
    return *this;
  }
  vtkWeakPtr(const vtkSmartPointer<T>& r)
    : Block(r ? r->GetWeakControlBlock() : nullptr)
  {
    vtkWeakPtr::CheckTypes<T>();
  }
  template <typename U>
  vtkWeakPtr(vtkSmartPointer<U>& r)
    : Block(r ? r->GetWeakControlBlock() : nullptr)
  {
    vtkWeakPtr::CheckTypes<U>();
  }
  vtkWeakPtr& operator=(vtkSmartPointer<T>& r)
  {
    this->Block = r ? r->GetWeakControlBlock() : nullptr;
    vtkWeakPtr::CheckTypes<T>();
    return *this;
  }
  template <typename U>
  vtkWeakPtr& operator=(vtkSmartPointer<U>& r)
  {
    this->Block = r ? r->GetWeakControlBlock() : nullptr;
    vtkWeakPtr::CheckTypes<U>();
    return *this;
  }
  ///@}

  ///@{
  /**
   * Raw pointer constructors and assignment.
   *
   * Not immediately accessible because the caller must declare that it owns a
   * reference to the pointer so that it does not become invalid before the
   * `vtkWeakPtr` is done with its construction.
   */
  static vtkWeakPtr FromOwningRawPointer(T* r) { return vtkWeakPtr(r); }
  template <typename U>
  static vtkWeakPtr FromOwningRawPointer(U* r)
  {
    return vtkWeakPtr(r);
  }
  vtkWeakPtr& Reset(T* r)
  {
    this->Block = r ? r->GetWeakControlBlock() : nullptr;
    return *this;
  }
  template <typename U>
  vtkWeakPtr& Reset(U* r)
  {
    this->Block = r ? r->GetWeakControlBlock() : nullptr;
    vtkWeakPtr::CheckTypes<U>();
    return vtkWeakPtr(r);
  }
  ///@}

  ///@{
  /**
   * Copy construction and assignment.
   */
  vtkWeakPtr(const vtkWeakPtr& r) = default;
  template <typename U>
  vtkWeakPtr(const vtkWeakPtr<U>& r)
    : Block(r.Block)
  {
    vtkWeakPtr::CheckTypes<U>();
  }
  vtkWeakPtr& operator=(const vtkWeakPtr& r) = default;
  template <typename U>
  vtkWeakPtr& operator=(const vtkWeakPtr<U>& r)
  {
    this->Block = r.Block;
    vtkWeakPtr::CheckTypes<U>();
    return *this;
  }
  ///@}

  ///@{
  /**
   * Move construction and assignment.
   */
  vtkWeakPtr(vtkWeakPtr&& r) noexcept = default;
  template <typename U>
  vtkWeakPtr(vtkWeakPtr<U>&& r) noexcept
    : Block(std::move(r.Block))
  {
    vtkWeakPtr::CheckTypes<U>();
  }
  vtkWeakPtr& operator=(vtkWeakPtr&& r) noexcept = default;
  template <typename U>
  vtkWeakPtr& operator=(vtkWeakPtr<U>&& r) noexcept
  {
    this->Block = std::move(r.Block);
    vtkWeakPtr::CheckTypes<U>();
    return *this;
  }
  ///@}

  ///@{
  ~vtkWeakPtr() noexcept = default;
  ///@}

  ///@{
  /**
   * Check whether the held object has expired or not.
   *
   * The only trustworthy result from this method is `true`. That is, once the
   * pointed-to object has expired, the state will not change without modifying
   * the `vtkWeakPtr` itself. Any indication of non-expiration is a
   * point-in-time check and does not guarantee that the object will not expire
   * between the check and any future use.
   *
   * Use of `Lock` is required to actually get access to the pointed-to object
   * and guarantees that it is accessible afterwards by adding a new reference.
   */
  bool Expired() const { return !this->Block || !this->Block->Object; }
  ///@}

  ///@{
  /**
   * Obtain a new reference to the held object, if available.
   *
   * If passed, the given object will be the owner of the new reference.
   *
   * "It is better to ask for forgiveness than permission."
   */
  vtkSmartPointer<T> Lock(vtkObjectBase* owner = nullptr) const
  {
    if (this->Block)
    {
      // Ensure that while we're working on the block, another thread does not
      // make `obj` `nullptr` behind us.
      std::lock_guard<std::mutex> guard(this->Block->Mutex);
      (void)guard;

      if (T* obj = static_cast<T*>(this->Block->Object))
      {
        // Add a reference, but check if we're working with a doomed instance
        // first. This is because `Register` is only safe when a strong
        // reference exists already, so this call checks to see if it can
        // create a new one from the collective ownership of the program.
        if (obj->TryUpgradeRegister(owner))
        {
          // We added a strong reference, give it to a smart pointer.
          return vtkSmartPointer<T>::Take(obj);
        }
        else
        {
          // We tried, but we're working with a doomed instance, so return
          // `nullptr`.
          return nullptr;
        }
      }
    }

    return nullptr;
  }
  ///@}

  ///@{
  /**
   * Compatibility with `std::owner_less` for use in comparison-based containers.
   */
  bool owner_before(const vtkWeakPtr& r) const { return this->Block < r.Block; }
  ///@}

private:
  ///@{
  /**
   * Raw pointer constructors (accessible via `FromOwningRawPointer`).
   */
  vtkWeakPtr(T* r)
    : Block(r ? r->GetWeakControlBlock() : nullptr)
  {
    vtkWeakPtr::CheckTypes<T>();
  }
  template <typename U>
  vtkWeakPtr(U* r)
    : Block(r ? r->GetWeakControlBlock() : nullptr)
  {
    vtkWeakPtr::CheckTypes<U>();
  }
  ///@}

  template <typename U>
  friend class vtkWeakPtr;
  std::shared_ptr<vtkObjectBase::WeakControlBlock> Block;
};

VTK_ABI_NAMESPACE_END

#endif

// VTK-HeaderTest-Exclude: vtkWeakPtr.h
