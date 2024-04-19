// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkCompositeDataSetNodeReference_h
#define vtkCompositeDataSetNodeReference_h

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkWeakPointer.h"

#include <cassert>
#include <type_traits>

namespace vtk
{
namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// MTimeWatcher:
// operator() return true if the MTime of its argument is less than or equal
// to the MTime of the object used to construct it.
//
// Create/reset using `mtime_watcher = MTimeWatcher{obj};`
//
// Test using `bool cacheIsValid = mtime_watcher(obj);`
//
// There are two variants of this:
// - MTimeWatcher can be used to ALWAYS check for valid mtimes.
// - DebugMTimeWatcher can be used to check mtimes ONLY in debugging builds,
//   and is defined as an empty, transparent no-op object in optimized builds.
//   The optimized version will always return true from operator().
struct MTimeWatcher
{
  vtkMTimeType MTime{ 0 };

  MTimeWatcher() = default;
  explicit MTimeWatcher(vtkObject* o)
    : MTime{ o->GetMTime() }
  {
  }
  bool operator()(vtkObject* o) const { return o->GetMTime() <= this->MTime; }
  void Reset(vtkObject* o) { this->MTime = o->GetMTime(); }
  bool MTimeIsValid(vtkObject* o) const { return o->GetMTime() <= this->MTime; }
};

// empty, transparent, does nothing. operator() always returns true.
struct NoOpMTimeWatcher
{
  NoOpMTimeWatcher() = default;
  explicit NoOpMTimeWatcher(vtkObject*) {}
  bool operator()(vtkObject*) const { return true; }
  void Reset(vtkObject*) {}
  bool MTimeIsValid(vtkObject*) const { return true; }
};

// Debug-dependent version:
#ifndef _NDEBUG
using DebugMTimeWatcher = MTimeWatcher;
#else
using DebugMTimeWatcher = NoOpMTimeWatcher;
#endif

//------------------------------------------------------------------------------
// DebugWeakPointer: Defined to vtkWeakPointer on debugging builds, T* on
// non-debugging builds.
#ifndef _NDEBUG
template <class ObjectType>
using DebugWeakPointer = vtkWeakPointer<ObjectType>;
#else
template <class ObjectType>
using DebugWeakPointer = ObjectType*;
#endif

VTK_ABI_NAMESPACE_END
} // end namespace detail

VTK_ABI_NAMESPACE_BEGIN

/**
 * A reference proxy into a vtkCompositeDataSet, obtained by dereferencing an
 * iterator from the vtk::Range(vtkCompositeDataSet*) overloads.
 *
 * This proxy may be used as a pointer, in which case it will forward the
 * currently pointed-to vtkDataObject*. This means that the following code is
 * legal:
 *
 * ```cpp
 * for (auto node : vtk::Range(cds))
 * { // decltype(node) == CompositeDataSetNodeReference
 *   if (node)                  // same as: if (node.GetDataObject() != nullptr)
 *   {
 *     assert(node->IsA("vtkDataObject"));     // node.GetDataObject()->IsA(...)
 *     node = nullptr;                         // node.SetDataObject(nullptr)
 *   }
 * }
 *
 * for (vtkDataObject *dObj : vtk::Range(cds))
 * {
 *   // Work with dObj
 * }
 * ```
 *
 * This allows for simple access to the objects in the composite dataset. If
 * more advanced operations are required, the CompositeDataSetNodeReference can:
 *
 * - Access the current vtkDataObject*:
 *   - `vtkDataObject* NodeReference::GetDataObject() const`
 *   - `NodeReference::operator vtkDataObject* () const` (implicit conversion)
 *   - `vtkDataObject* NodeReference::operator->() const` (arrow operator)
 * - Replace the current vtkDataObject* in the composite dataset:
 *   - `void NodeReference::SetDataObject(vtkDataObject*)`
 *   - `NodeReference& NodeReference::operator=(vtkDataObject*)` (assignment)
 * - SetGet the vtkDataObject at the same position in another composite dataset
 *   - `void NodeReference::SetDataObject(vtkCompositeDataSet*, vtkDataObject*)`
 *   - `vtkDataObject* NodeReference::GetDataObject(vtkCompositeDataSet*) const`
 * - Check and access node metadata (if any):
 *   - `bool NodeReference::HasMetaData() const`
 *   - `vtkInformation* NodeReference::GetMetaData() const`
 * - Get the current flat index within the parent range:
 *   - `unsigned int NodeReference::GetFlatIndex() const`
 *
 * Assigning one reference to another assigns the vtkDataObject* pointer to the
 * target reference. Assigning to non-leaf nodes invalidates all iterators /
 * references.
 *
 * Equality testing compares each reference's DataObject and FlatIndex.
 *
 * @warning The NodeReference shares state with the OwnerType iterator that
 * generates it. Incrementing or destroying the parent iterator will invalidate
 * the reference. In debugging builds, these misuses will be caught via runtime
 * assertions.
 */
template <typename IteratorType,
  typename OwnerType>
class CompositeDataSetNodeReference
  : private detail::DebugMTimeWatcher // empty-base optimization when NDEBUG
{
  static_assert(std::is_base_of<vtkCompositeDataIterator, IteratorType>::value,
    "CompositeDataSetNodeReference's IteratorType must be a "
    "subclass of vtkCompositeDataIterator.");

  // Either a vtkWeakPointer (debug builds) or raw pointer (non-debug builds)
  mutable detail::DebugWeakPointer<IteratorType> Iterator{ nullptr };

  // Check that the reference has not been invalidated by having the
  // borrowed internal iterator modified.
  void AssertValid() const
  {

    // Test that the weak pointer hasn't been cleared
    assert(
      "Invalid CompositeDataNodeReference accessed (iterator freed)." && this->Iterator != nullptr);
    // Check MTime:
    assert("Invalid CompositeDataNodeReference accessed (iterator modified)." &&
      this->MTimeIsValid(this->Iterator));
  }

protected:
  explicit CompositeDataSetNodeReference(IteratorType* iterator)
    : detail::DebugMTimeWatcher(iterator)
    , Iterator(iterator)
  {
  }

public:
  friend OwnerType; // To allow access to protected methods/base class

  CompositeDataSetNodeReference() = delete;
  CompositeDataSetNodeReference(const CompositeDataSetNodeReference& src) = default;
  CompositeDataSetNodeReference(CompositeDataSetNodeReference&&) noexcept = default;
  ~CompositeDataSetNodeReference() = default;

  // Assigns the DataObject from src to this:
  CompositeDataSetNodeReference& operator=(const CompositeDataSetNodeReference& src)
  {
    this->SetDataObject(src.GetDataObject());
    return *this;
  }

  // Compares data object and flat index:
  friend bool operator==(
    const CompositeDataSetNodeReference& lhs, const CompositeDataSetNodeReference& rhs)
  {
    return lhs.GetDataObject() == rhs.GetDataObject() && lhs.GetFlatIndex() == rhs.GetFlatIndex();
  }

  // Compares data object and flat index:
  friend bool operator!=(
    const CompositeDataSetNodeReference& lhs, const CompositeDataSetNodeReference& rhs)
  {
    return lhs != rhs;
  }

  vtkDataObject* GetDataObject() const
  {
    this->AssertValid();
    // GetCurrentDataObject is buggy -- the iterator caches the current dataset
    // internally, so if the object has changed since the iterator was
    // incremented, the changes will not be visible through the iterator's
    // API. See VTK issue #17529.
    // Instead, look it up in the dataset. It's a bit slower, but will always be
    // correct.
    //    return this->Iterator->GetCurrentDataObject();
    return this->Iterator->GetDataSet()->GetDataSet(this->Iterator);
  }

  vtkDataObject* GetDataObject(vtkCompositeDataSet* other)
  {
    this->AssertValid();
    return other->GetDataSet(this->Iterator);
  }

  operator bool() const { return this->GetDataObject() != nullptr; }

  operator vtkDataObject*() const { return this->GetDataObject(); }

  vtkDataObject* operator->() const { return this->GetDataObject(); }

  void SetDataObject(vtkDataObject* obj)
  {
    this->AssertValid();
    vtkCompositeDataSet* cds = this->Iterator->GetDataSet();
    cds->SetDataSet(this->Iterator, obj);
  }

  void SetDataObject(vtkCompositeDataSet* other, vtkDataObject* dObj)
  {
    this->AssertValid();
    other->SetDataSet(this->Iterator, dObj);
  }

  CompositeDataSetNodeReference& operator=(vtkDataObject* obj)
  {
    this->SetDataObject(obj);
    return *this;
  }

  unsigned int GetFlatIndex() const
  {
    this->AssertValid();
    return this->Iterator->GetCurrentFlatIndex();
  }

  bool HasMetaData() const
  {
    this->AssertValid();
    return this->Iterator->HasCurrentMetaData() != 0;
  }

  vtkInformation* GetMetaData() const
  {
    this->AssertValid();
    return this->Iterator->GetCurrentMetaData();
  }
};

VTK_ABI_NAMESPACE_END
} // end namespace vtk

#endif // vtkCompositeDataSetNodeReference_h

// VTK-HeaderTest-Exclude: vtkCompositeDataSetNodeReference.h
