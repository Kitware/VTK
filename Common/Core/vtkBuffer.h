// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBuffer
 * @brief   internal storage class used by vtkSOADataArrayTemplate,
 * vtkAOSDataArrayTemplate, and others.
 *
 * vtkBuffer makes it easier to keep data pointers in vtkDataArray subclasses.
 * This is an internal class and not intended for direct use expect when writing
 * new types of vtkDataArray subclasses.
 */

#ifndef vtkBuffer_h
#define vtkBuffer_h

#include "vtkObject.h"
#include "vtkObjectFactory.h" // New() implementation

#include <algorithm> // for std::min and std::copy

VTK_ABI_NAMESPACE_BEGIN
template <class ScalarTypeT>
class vtkBuffer : public vtkObject
{
public:
  vtkTemplateTypeMacro(vtkBuffer<ScalarTypeT>, vtkObject);
  typedef ScalarTypeT ScalarType;

  static vtkBuffer<ScalarTypeT>* New();
  static vtkBuffer<ScalarTypeT>* ExtendedNew();

  /**
   * Access the buffer as a scalar pointer.
   */
  ScalarType* GetBuffer() { return this->Pointer; }
  const ScalarType* GetBuffer() const { return this->Pointer; }

  /**
   * Set the memory buffer that this vtkBuffer object will manage. @a array
   * is a pointer to the buffer data and @a size is the size of the buffer (in
   * number of elements).
   */
  void SetBuffer(ScalarType* array, vtkIdType size);

  /**
   * Set the malloc function to be used when allocating space inside this object.
   **/
  void SetMallocFunction(vtkMallocingFunction mallocFunction = malloc);

  /**
   * Set the realloc function to be used when allocating space inside this object.
   **/
  void SetReallocFunction(vtkReallocingFunction reallocFunction = realloc);

  /**
   * Set the free function to be used when releasing this object.
   * If @a noFreeFunction is true, the buffer will not be freed when
   * this vtkBuffer object is deleted or resize -- otherwise, @a deleteFunction
   * will be called to free the buffer
   **/
  void SetFreeFunction(bool noFreeFunction, vtkFreeingFunction deleteFunction = free);

  /**
   * Return the number of elements the current buffer can hold.
   */
  vtkIdType GetSize() const { return this->Size; }

  /**
   * Allocate a new buffer that holds @a size elements. Old data is not saved.
   */
  bool Allocate(vtkIdType size);

  /**
   * Allocate a new buffer that holds @a newsize elements. Old data is
   * preserved.
   */
  bool Reallocate(vtkIdType newsize);

protected:
  vtkBuffer()
    : Pointer(nullptr)
    , Size(0)
  {
    this->SetMallocFunction(vtkObjectBase::GetCurrentMallocFunction());
    this->SetReallocFunction(vtkObjectBase::GetCurrentReallocFunction());
    this->SetFreeFunction(false, vtkObjectBase::GetCurrentFreeFunction());
  }

  ~vtkBuffer() override { this->SetBuffer(nullptr, 0); }

  ScalarType* Pointer;
  vtkIdType Size;
  vtkMallocingFunction MallocFunction;
  vtkReallocingFunction ReallocFunction;
  vtkFreeingFunction DeleteFunction;

private:
  vtkBuffer(const vtkBuffer&) = delete;
  void operator=(const vtkBuffer&) = delete;
};

template <class ScalarT>
inline vtkBuffer<ScalarT>* vtkBuffer<ScalarT>::New()
{
  VTK_STANDARD_NEW_BODY(vtkBuffer<ScalarT>);
}

template <class ScalarT>
inline vtkBuffer<ScalarT>* vtkBuffer<ScalarT>::ExtendedNew()
{
  auto mkhold = vtkMemkindRAII(true);
  return vtkBuffer<ScalarT>::New();
}

//------------------------------------------------------------------------------
template <typename ScalarT>
void vtkBuffer<ScalarT>::SetBuffer(typename vtkBuffer<ScalarT>::ScalarType* array, vtkIdType sz)
{
  if (this->Pointer != array)
  {
    if (this->DeleteFunction)
    {
      this->DeleteFunction(this->Pointer);
    }
    this->Pointer = array;
  }
  this->Size = sz;
}
//------------------------------------------------------------------------------
template <typename ScalarT>
void vtkBuffer<ScalarT>::SetMallocFunction(vtkMallocingFunction mallocFunction)
{
  this->MallocFunction = mallocFunction;
}
//------------------------------------------------------------------------------
template <typename ScalarT>
void vtkBuffer<ScalarT>::SetReallocFunction(vtkReallocingFunction reallocFunction)
{
  this->ReallocFunction = reallocFunction;
}

//------------------------------------------------------------------------------
template <typename ScalarT>
void vtkBuffer<ScalarT>::SetFreeFunction(bool noFreeFunction, vtkFreeingFunction deleteFunction)
{
  if (noFreeFunction)
  {
    this->DeleteFunction = nullptr;
  }
  else
  {
    this->DeleteFunction = deleteFunction;
  }
}

//------------------------------------------------------------------------------
template <typename ScalarT>
bool vtkBuffer<ScalarT>::Allocate(vtkIdType size)
{
  // release old memory.
  this->SetBuffer(nullptr, 0);
  if (size > 0)
  {
    ScalarType* newArray;
    if (this->MallocFunction)
    {
      newArray = static_cast<ScalarType*>(this->MallocFunction(size * sizeof(ScalarType)));
    }
    else
    {
      newArray = static_cast<ScalarType*>(malloc(size * sizeof(ScalarType)));
    }
    if (newArray)
    {
      this->SetBuffer(newArray, size);
      if (!this->MallocFunction)
      {
        this->DeleteFunction = free;
      }
      return true;
    }
    return false;
  }
  return true; // size == 0
}

//------------------------------------------------------------------------------
template <typename ScalarT>
bool vtkBuffer<ScalarT>::Reallocate(vtkIdType newsize)
{
  if (newsize == 0)
  {
    return this->Allocate(0);
  }

  if (this->Pointer && this->DeleteFunction != free)
  {
    ScalarType* newArray;
    bool forceFreeFunction = false;
    if (this->MallocFunction)
    {
      newArray = static_cast<ScalarType*>(this->MallocFunction(newsize * sizeof(ScalarType)));
      if (this->MallocFunction == malloc)
      {
        // This must be done because the array passed in may have been
        // allocated outside of the memory management of `vtkBuffer` and
        // therefore have been registered with a `DeleteFunction` such as
        // `delete` or `delete[]`. Since the memory is now allocated with
        // `malloc` here, we must also reset `DeleteFunction` to something
        // which matches.
        forceFreeFunction = true;
      }
    }
    else
    {
      newArray = static_cast<ScalarType*>(malloc(newsize * sizeof(ScalarType)));
    }
    if (!newArray)
    {
      return false;
    }
    std::copy(this->Pointer, this->Pointer + (std::min)(this->Size, newsize), newArray);
    // now save the new array and release the old one too.
    this->SetBuffer(newArray, newsize);
    if (!this->MallocFunction || forceFreeFunction)
    {
      this->DeleteFunction = free;
    }
  }
  else
  {
    // Try to reallocate with minimal memory usage and possibly avoid
    // copying.
    ScalarType* newArray = nullptr;
    if (this->ReallocFunction)
    {
      newArray = static_cast<ScalarType*>(
        this->ReallocFunction(this->Pointer, newsize * sizeof(ScalarType)));
    }
    else
    {
      newArray = static_cast<ScalarType*>(realloc(this->Pointer, newsize * sizeof(ScalarType)));
    }
    if (!newArray)
    {
      return false;
    }
    this->Pointer = newArray;
    this->Size = newsize;
  }
  return true;
}

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkBuffer.h
