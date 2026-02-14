// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSOADataArrayTemplate_txx
#define vtkSOADataArrayTemplate_txx

#ifdef VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATING
#define VTK_GDA_VALUERANGE_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#undef VTK_GDA_VALUERANGE_INSTANTIATING
#endif

#include "vtkSOADataArrayTemplate.h"

#include "vtkArrayIteratorTemplate.h"
#include "vtkBuffer.h"
#include "vtkCommand.h"

#include <array>
#include <cassert>

//-----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
template <class ValueType>
vtkSOADataArrayTemplate<ValueType>* vtkSOADataArrayTemplate<ValueType>::New()
{
  VTK_STANDARD_NEW_BODY(vtkSOADataArrayTemplate<ValueType>);
}

//-----------------------------------------------------------------------------
template <class ValueType>
vtkSOADataArrayTemplate<ValueType>::vtkSOADataArrayTemplate()
{
  this->SetNumberOfComponents(1); // create at least one component buffer
}

//-----------------------------------------------------------------------------
template <class ValueType>
vtkSOADataArrayTemplate<ValueType>::~vtkSOADataArrayTemplate()
{
  for (size_t cc = 0; cc < this->Data.size(); ++cc)
  {
    // vtkBuffer knows the free function and whether or not to actually deallocate the memory
    this->Data[cc]->Delete();
  }
  this->Data.clear();
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::SetNumberOfComponents(int val)
{
  this->GenericDataArrayType::SetNumberOfComponents(val);
  size_t numComps = static_cast<size_t>(this->GetNumberOfComponents());
  assert(numComps >= 1);
  while (this->Data.size() > numComps)
  {
    this->Data.back()->Delete();
    this->Data.pop_back();
  }
  while (this->Data.size() < numComps)
  {
    this->Data.push_back(vtkBuffer<ValueType>::New());
  }
}

//-----------------------------------------------------------------------------
template <class ValueType>
vtkArrayIterator* vtkSOADataArrayTemplate<ValueType>::NewIterator()
{
  vtkArrayIterator* iter = vtkArrayIteratorTemplate<ValueType>::New();
  iter->Initialize(this);
  return iter;
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::ShallowCopy(vtkDataArray* other)
{
  SelfType* o = SelfType::FastDownCast(other);
  if (o)
  {
    this->Size = o->Size;
    this->MaxId = o->MaxId;
    this->SetName(o->Name);
    this->SetNumberOfComponents(o->NumberOfComponents);
    this->CopyComponentNames(o);
    assert(this->Data.size() == o->Data.size());
    for (size_t cc = 0; cc < this->Data.size(); ++cc)
    {
      vtkBuffer<ValueType>* thisBuffer = this->Data[cc];
      vtkBuffer<ValueType>* otherBuffer = o->Data[cc];
      if (thisBuffer != otherBuffer)
      {
        thisBuffer->Delete();
        this->Data[cc] = otherBuffer;
        otherBuffer->Register(nullptr);
      }
    }
    this->DataChanged();
    this->InvokeEvent(vtkCommand::BufferChangedEvent);
  }
  else
  {
    this->Superclass::ShallowCopy(other);
  }
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::InsertTuples(
  vtkIdType dstStart, vtkIdType n, vtkIdType srcStart, vtkAbstractArray* source)
{
  // First, check for the common case of typeid(source) == typeid(this). This
  // way we don't waste time redoing the other checks in the superclass, and
  // can avoid doing a dispatch for the most common usage of this method.
  SelfType* other = vtkArrayDownCast<SelfType>(source);
  if (!other)
  {
    // Let the superclass handle dispatch/fallback.
    this->Superclass::InsertTuples(dstStart, n, srcStart, source);
    return;
  }

  if (n == 0)
  {
    return;
  }

  int numComps = this->GetNumberOfComponents();
  if (other->GetNumberOfComponents() != numComps)
  {
    vtkErrorMacro("Number of components do not match: Source: "
      << other->GetNumberOfComponents() << " Dest: " << this->GetNumberOfComponents());
    return;
  }

  vtkIdType maxSrcTupleId = srcStart + n - 1;
  vtkIdType maxDstTupleId = dstStart + n - 1;

  if (maxSrcTupleId >= other->GetNumberOfTuples())
  {
    vtkErrorMacro("Source array too small, requested tuple at index "
      << maxSrcTupleId << ", but there are only " << other->GetNumberOfTuples()
      << " tuples in the array.");
    return;
  }

  vtkIdType newSize = (maxDstTupleId + 1) * this->NumberOfComponents;
  if (this->Size < newSize)
  {
    if (!this->Resize(maxDstTupleId + 1))
    {
      vtkErrorMacro("Resize failed.");
      return;
    }
  }

  this->MaxId = std::max(this->MaxId, newSize - 1);

  for (int c = 0; c < numComps; ++c)
  {
    ValueType* srcBegin = other->GetComponentArrayPointer(c) + srcStart;
    ValueType* srcEnd = srcBegin + n;
    ValueType* dstBegin = this->GetComponentArrayPointer(c) + dstStart;
    std::copy(srcBegin, srcEnd, dstBegin);
  }
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::FillTypedComponent(int compIdx, ValueType value)
{
  ValueType* buffer = this->Data[compIdx]->GetBuffer();
  std::fill(buffer, buffer + this->GetNumberOfTuples(), value);
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::SetArray(
  int comp, ValueType* array, vtkIdType size, bool updateMaxId, bool save, int deleteMethod)
{
  const int numComps = this->GetNumberOfComponents();
  if (comp >= numComps || comp < 0)
  {
    vtkErrorMacro("Invalid component number '"
      << comp
      << "' specified. "
         "Use `SetNumberOfComponents` first to set the number of components.");
    return;
  }

  while (this->Data.size() < static_cast<size_t>(numComps))
  {
    this->Data.push_back(vtkBuffer<ValueType>::New());
  }

  this->Data[comp]->SetBuffer(array, size);

  if (deleteMethod == VTK_DATA_ARRAY_DELETE)
  {
    this->Data[comp]->SetFreeFunction(save != 0, ::operator delete[]);
  }
  else if (deleteMethod == VTK_DATA_ARRAY_ALIGNED_FREE)
  {
#ifdef _WIN32
    this->Data[comp]->SetFreeFunction(save != 0, _aligned_free);
#else
    this->Data[comp]->SetFreeFunction(save != 0, free);
#endif
  }
  else if (deleteMethod == VTK_DATA_ARRAY_USER_DEFINED || deleteMethod == VTK_DATA_ARRAY_FREE)
  {
    this->Data[comp]->SetFreeFunction(save != 0, free);
  }

  if (updateMaxId)
  {
    this->Size = numComps * size;
    this->MaxId = this->Size - 1;
  }

  this->DataChanged();
  this->InvokeEvent(vtkCommand::BufferChangedEvent);
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::SetBuffer(
  int comp, vtkAbstractBuffer* buffer, bool updateMaxId)
{
  if (buffer == nullptr)
  {
    vtkErrorMacro("Cannot set a null buffer.");
    return;
  }

  vtkBuffer<ValueType>* typedBuffer = vtkBuffer<ValueType>::SafeDownCast(buffer);
  if (typedBuffer == nullptr)
  {
    vtkErrorMacro("Buffer type does not match array type. Expected vtkBuffer<"
      << this->GetDataTypeAsString() << ">.");
    return;
  }

  this->SetBuffer(comp, typedBuffer, updateMaxId);
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::SetBuffer(
  int comp, vtkBuffer<ValueType>* buffer, bool updateMaxId)
{
  const int numComps = this->GetNumberOfComponents();
  if (comp >= numComps || comp < 0)
  {
    vtkErrorMacro("Invalid component number '"
      << comp
      << "' specified. "
         "Use `SetNumberOfComponents` first to set the number of components.");
    return;
  }

  if (buffer == nullptr)
  {
    vtkErrorMacro("Cannot set a null buffer.");
    return;
  }

  while (this->Data.size() < static_cast<size_t>(numComps))
  {
    this->Data.push_back(vtkBuffer<ValueType>::New());
  }

  // Replace the old buffer with the new one
  if (this->Data[comp] != buffer)
  {
    this->Data[comp]->Delete();
    this->Data[comp] = buffer;
    buffer->Register(nullptr);
  }

  if (updateMaxId)
  {
    this->Size = numComps * buffer->GetSize();
    this->MaxId = this->Size - 1;
  }
  this->DataChanged();
  this->InvokeEvent(vtkCommand::BufferChangedEvent);
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::SetArrayFreeFunction(void (*callback)(void*))
{
  const int numComps = this->GetNumberOfComponents();
  for (int i = 0; i < numComps; ++i)
  {
    this->SetArrayFreeFunction(i, callback);
  }
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::SetArrayFreeFunction(int comp, void (*callback)(void*))
{
  const int numComps = this->GetNumberOfComponents();
  if (comp >= numComps || comp < 0)
  {
    vtkErrorMacro("Invalid component number '"
      << comp
      << "' specified. "
         "Use `SetNumberOfComponents` first to set the number of components.");
    return;
  }
  this->Data[comp]->SetFreeFunction(false, callback);
}

//-----------------------------------------------------------------------------
template <class ValueType>
typename vtkSOADataArrayTemplate<ValueType>::ValueType*
vtkSOADataArrayTemplate<ValueType>::GetComponentArrayPointer(int comp)
{
  const int numComps = this->GetNumberOfComponents();
  if (comp >= numComps || comp < 0)
  {
    vtkErrorMacro("Invalid component number '" << comp << "' specified.");
    return nullptr;
  }

  return this->Data[comp]->GetBuffer();
}

//-----------------------------------------------------------------------------
template <class ValueType>
vtkBuffer<ValueType>* vtkSOADataArrayTemplate<ValueType>::GetComponentBuffer(int comp)
{
  const int numComps = this->GetNumberOfComponents();
  if (comp >= numComps || comp < 0)
  {
    vtkErrorMacro("Invalid component number '" << comp << "' specified.");
    return nullptr;
  }

  return this->Data[comp];
}

//-----------------------------------------------------------------------------
template <class ValueType>
bool vtkSOADataArrayTemplate<ValueType>::AllocateTuples(vtkIdType numTuples)
{
  for (size_t cc = 0, max = this->Data.size(); cc < max; ++cc)
  {
    if (!this->Data[cc]->Allocate(numTuples))
    {
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
template <class ValueType>
bool vtkSOADataArrayTemplate<ValueType>::ReallocateTuples(vtkIdType numTuples)
{
  bool bufferChanged = false;
  for (size_t cc = 0, max = this->Data.size(); cc < max; ++cc)
  {
    vtkIdType oldSize = this->Data[cc]->GetSize();
    if (numTuples != oldSize)
    {
      if (!this->Data[cc]->Reallocate(numTuples))
      {
        return false;
      }
      bufferChanged = true;
    }
  }

  // Notify observers that the buffer may have changed
  if (bufferChanged)
  {
    this->InvokeEvent(vtkCommand::BufferChangedEvent);
  }
  return true;
}

//-----------------------------------------------------------------------------
template <class ValueType>
void* vtkSOADataArrayTemplate<ValueType>::GetVoidPointer(vtkIdType valueIdx)
{
  if (this->GetNumberOfComponents() == 1)
  {
    // if there's only a single component the data will be stored in
    // contiguous memory so we can return the pointer to that array
    return static_cast<void*>(this->Data[0]->GetBuffer() + valueIdx);
  }
  return this->Superclass::GetVoidPointer(valueIdx); // NOLINT(bugprone-unsafe-functions)
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::ExportToVoidPointer(void* voidPtr)
{
  vtkIdType numTuples = this->GetNumberOfTuples();
  if (this->NumberOfComponents * numTuples == 0)
  {
    // Nothing to do.
    return;
  }

  if (!voidPtr)
  {
    vtkErrorMacro(<< "Buffer is nullptr.");
    return;
  }

  ValueType* ptr = static_cast<ValueType*>(voidPtr);
  for (vtkIdType t = 0; t < numTuples; ++t)
  {
    for (int c = 0; c < this->NumberOfComponents; ++c)
    {
      *ptr++ = this->Data[c]->GetBuffer()[t];
    }
  }
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::CopyData(vtkSOADataArrayTemplate<ValueType>* src)
{
  int numberOfComponents = this->GetNumberOfComponents();
  vtkIdType numberOfTuples = this->GetNumberOfTuples();
  if (numberOfComponents == 1)
  { // first optimization is if we have contiguous memory for both src and this
    ValueType* srcBegin = src->GetComponentArrayPointer(0);
    ValueType* srcEnd = srcBegin + numberOfTuples;
    ValueType* dstBegin = this->GetComponentArrayPointer(0);

    std::copy(srcBegin, srcEnd, dstBegin);
  }
  else
  {
    for (int comp = 0; comp < src->GetNumberOfComponents(); ++comp)
    {
      ValueType* srcBegin = src->GetComponentArrayPointer(comp);
      ValueType* srcEnd = srcBegin + numberOfTuples;
      ValueType* dstBegin = this->GetComponentArrayPointer(comp);

      std::copy(srcBegin, srcEnd, dstBegin);
    }
  }
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
{
  for (size_t cc = 0; cc < this->Data.size(); cc++)
  {
    tuple[cc] = this->Data[cc]->GetBuffer()[tupleIdx];
  }
}

//-----------------------------------------------------------------------------
template <class ValueType>
typename vtkSOADataArrayTemplate<ValueType>::ValueType
vtkSOADataArrayTemplate<ValueType>::GetTypedComponent(vtkIdType tupleIdx, int comp) const
{
  return this->Data[comp]->GetBuffer()[tupleIdx];
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkSOADataArrayTemplate<ValueType>::SetTypedComponent(
  vtkIdType tupleIdx, int comp, ValueType value)
{
  this->Data[comp]->GetBuffer()[tupleIdx] = value;
}

VTK_ABI_NAMESPACE_END
#endif
