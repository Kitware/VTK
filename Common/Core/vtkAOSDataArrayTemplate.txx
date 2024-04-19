// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkAOSDataArrayTemplate_txx
#define vtkAOSDataArrayTemplate_txx

#ifdef VTK_AOS_DATA_ARRAY_TEMPLATE_INSTANTIATING
#define VTK_GDA_VALUERANGE_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#undef VTK_GDA_VALUERANGE_INSTANTIATING
#endif

#include "vtkAOSDataArrayTemplate.h"

#include "vtkArrayIteratorTemplate.h"

//-----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
template <class ValueTypeT>
vtkAOSDataArrayTemplate<ValueTypeT>* vtkAOSDataArrayTemplate<ValueTypeT>::New()
{
  VTK_STANDARD_NEW_BODY(vtkAOSDataArrayTemplate<ValueType>);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
vtkAOSDataArrayTemplate<typename vtkAOSDataArrayTemplate<ValueTypeT>::ValueType>*
vtkAOSDataArrayTemplate<ValueTypeT>::FastDownCast(vtkAbstractArray* source)
{
  if (source)
  {
    switch (source->GetArrayType())
    {
      case vtkAbstractArray::AoSDataArrayTemplate:
        if (vtkDataTypesCompare(source->GetDataType(), vtkTypeTraits<ValueType>::VTK_TYPE_ID))
        {
          return static_cast<vtkAOSDataArrayTemplate<ValueType>*>(source);
        }
        break;
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
vtkAOSDataArrayTemplate<ValueTypeT>::vtkAOSDataArrayTemplate()
{
  this->Buffer = vtkBuffer<ValueType>::New();
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
vtkAOSDataArrayTemplate<ValueTypeT>::~vtkAOSDataArrayTemplate()
{
  this->Buffer->Delete();
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::SetArray(
  ValueType* array, vtkIdType size, int save, int deleteMethod)
{

  this->Buffer->SetBuffer(array, size);

  if (deleteMethod == VTK_DATA_ARRAY_DELETE)
  {
    this->Buffer->SetFreeFunction(save != 0, ::operator delete[]);
  }
  else if (deleteMethod == VTK_DATA_ARRAY_ALIGNED_FREE)
  {
#ifdef _WIN32
    this->Buffer->SetFreeFunction(save != 0, _aligned_free);
#else
    this->Buffer->SetFreeFunction(save != 0, free);
#endif
  }
  else if (deleteMethod == VTK_DATA_ARRAY_USER_DEFINED || deleteMethod == VTK_DATA_ARRAY_FREE)
  {
    this->Buffer->SetFreeFunction(save != 0, free);
  }

  this->Size = size;
  this->MaxId = this->Size - 1;
  this->DataChanged();
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::SetArray(ValueType* array, vtkIdType size, int save)
{
  this->SetArray(array, size, save, VTK_DATA_ARRAY_FREE);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::SetVoidArray(void* array, vtkIdType size, int save)
{
  this->SetArray(static_cast<ValueType*>(array), size, save);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::SetVoidArray(
  void* array, vtkIdType size, int save, int deleteMethod)
{
  this->SetArray(static_cast<ValueType*>(array), size, save, deleteMethod);
}

//-----------------------------------------------------------------------------
template <class ValueType>
void vtkAOSDataArrayTemplate<ValueType>::SetArrayFreeFunction(void (*callback)(void*))
{
  this->Buffer->SetFreeFunction(false, callback);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::SetTuple(vtkIdType tupleIdx, const float* tuple)
{
  // While std::copy is the obvious choice here, it kills performance on MSVC
  // debugging builds as their STL calls are poorly optimized. Just use a for
  // loop instead.
  ValueTypeT* data = this->Buffer->GetBuffer() + tupleIdx * this->NumberOfComponents;
  for (int i = 0; i < this->NumberOfComponents; ++i)
  {
    data[i] = static_cast<ValueType>(tuple[i]);
  }
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::SetTuple(vtkIdType tupleIdx, const double* tuple)
{
  // See note in SetTuple about std::copy vs for loops on MSVC.
  ValueTypeT* data = this->Buffer->GetBuffer() + tupleIdx * this->NumberOfComponents;
  for (int i = 0; i < this->NumberOfComponents; ++i)
  {
    data[i] = static_cast<ValueType>(tuple[i]);
  }
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::InsertTuple(vtkIdType tupleIdx, const float* tuple)
{
  if (this->EnsureAccessToTuple(tupleIdx))
  {
    // See note in SetTuple about std::copy vs for loops on MSVC.
    const vtkIdType valueIdx = tupleIdx * this->NumberOfComponents;
    ValueTypeT* data = this->Buffer->GetBuffer() + valueIdx;
    for (int i = 0; i < this->NumberOfComponents; ++i)
    {
      data[i] = static_cast<ValueType>(tuple[i]);
    }
    this->MaxId = std::max(this->MaxId, valueIdx + this->NumberOfComponents - 1);
  }
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::InsertTuple(vtkIdType tupleIdx, const double* tuple)
{
  if (this->EnsureAccessToTuple(tupleIdx))
  {
    // See note in SetTuple about std::copy vs for loops on MSVC.
    const vtkIdType valueIdx = tupleIdx * this->NumberOfComponents;
    ValueTypeT* data = this->Buffer->GetBuffer() + valueIdx;
    for (int i = 0; i < this->NumberOfComponents; ++i)
    {
      data[i] = static_cast<ValueType>(tuple[i]);
    }
    this->MaxId = std::max(this->MaxId, valueIdx + this->NumberOfComponents - 1);
  }
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::InsertComponent(
  vtkIdType tupleIdx, int compIdx, double value)
{
  const vtkIdType newMaxId = tupleIdx * this->NumberOfComponents + compIdx;
  if (newMaxId >= this->Size)
  {
    if (!this->Resize(newMaxId / this->NumberOfComponents + 1))
    {
      return;
    }
  }

  this->Buffer->GetBuffer()[newMaxId] = static_cast<ValueTypeT>(value);
  this->MaxId = std::max(newMaxId, this->MaxId);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
vtkIdType vtkAOSDataArrayTemplate<ValueTypeT>::InsertNextTuple(const float* tuple)
{
  vtkIdType newMaxId = this->MaxId + this->NumberOfComponents;
  const vtkIdType tupleIdx = newMaxId / this->NumberOfComponents;
  if (newMaxId >= this->Size)
  {
    if (!this->Resize(tupleIdx + 1))
    {
      return -1;
    }
  }

  // See note in SetTuple about std::copy vs for loops on MSVC.
  ValueTypeT* data = this->Buffer->GetBuffer() + this->MaxId + 1;
  for (int i = 0; i < this->NumberOfComponents; ++i)
  {
    data[i] = static_cast<ValueType>(tuple[i]);
  }
  this->MaxId = newMaxId;
  return tupleIdx;
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
vtkIdType vtkAOSDataArrayTemplate<ValueTypeT>::InsertNextTuple(const double* tuple)
{
  vtkIdType newMaxId = this->MaxId + this->NumberOfComponents;
  const vtkIdType tupleIdx = newMaxId / this->NumberOfComponents;
  if (newMaxId >= this->Size)
  {
    if (!this->Resize(tupleIdx + 1))
    {
      return -1;
    }
  }

  // See note in SetTuple about std::copy vs for loops on MSVC.
  ValueTypeT* data = this->Buffer->GetBuffer() + this->MaxId + 1;
  for (int i = 0; i < this->NumberOfComponents; ++i)
  {
    data[i] = static_cast<ValueType>(tuple[i]);
  }
  this->MaxId = newMaxId;
  return tupleIdx;
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::GetTuple(vtkIdType tupleIdx, double* tuple)
{
  ValueTypeT* data = this->Buffer->GetBuffer() + tupleIdx * this->NumberOfComponents;
  // See note in SetTuple about std::copy vs for loops on MSVC.
  for (int i = 0; i < this->NumberOfComponents; ++i)
  {
    tuple[i] = static_cast<double>(data[i]);
  }
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
double* vtkAOSDataArrayTemplate<ValueTypeT>::GetTuple(vtkIdType tupleIdx)
{
  ValueTypeT* data = this->Buffer->GetBuffer() + tupleIdx * this->NumberOfComponents;
  double* tuple = &this->LegacyTuple[0];
  // See note in SetTuple about std::copy vs for loops on MSVC.
  for (int i = 0; i < this->NumberOfComponents; ++i)
  {
    tuple[i] = static_cast<double>(data[i]);
  }
  return &this->LegacyTuple[0];
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
vtkArrayIterator* vtkAOSDataArrayTemplate<ValueTypeT>::NewIterator()
{
  vtkArrayIterator* iter = vtkArrayIteratorTemplate<ValueType>::New();
  iter->Initialize(this);
  return iter;
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::ShallowCopy(vtkDataArray* other)
{
  SelfType* o = SelfType::FastDownCast(other);
  if (o)
  {
    this->Size = o->Size;
    this->MaxId = o->MaxId;
    this->SetName(o->Name);
    this->SetNumberOfComponents(o->NumberOfComponents);
    this->CopyComponentNames(o);
    if (this->Buffer != o->Buffer)
    {
      this->Buffer->Delete();
      this->Buffer = o->Buffer;
      this->Buffer->Register(nullptr);
    }
    this->DataChanged();
  }
  else
  {
    this->Superclass::ShallowCopy(other);
  }
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::InsertTuples(
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

  ValueType* srcBegin = other->GetPointer(srcStart * numComps);
  ValueType* srcEnd = srcBegin + (n * numComps);
  ValueType* dstBegin = this->GetPointer(dstStart * numComps);

  std::copy(srcBegin, srcEnd, dstBegin);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::FillTypedComponent(int compIdx, ValueType value)
{
  if (this->NumberOfComponents <= 1)
  {
    this->FillValue(value);
  }
  else
  {
    this->Superclass::FillTypedComponent(compIdx, value);
  }
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::FillValue(ValueType value)
{
  std::ptrdiff_t offset = this->MaxId + 1;
  std::fill(this->Buffer->GetBuffer(), this->Buffer->GetBuffer() + offset, value);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::Fill(double value)
{
  this->FillValue(static_cast<ValueType>(value));
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
typename vtkAOSDataArrayTemplate<ValueTypeT>::ValueType*
vtkAOSDataArrayTemplate<ValueTypeT>::WritePointer(vtkIdType valueIdx, vtkIdType numValues)
{
  vtkIdType newSize = valueIdx + numValues;
  if (newSize > this->Size)
  {
    if (!this->Resize(newSize / this->NumberOfComponents + 1))
    {
      return nullptr;
    }
    this->MaxId = (newSize - 1);
  }

  // For extending the in-use ids but not the size:
  this->MaxId = std::max(this->MaxId, newSize - 1);

  this->DataChanged();
  return this->GetPointer(valueIdx);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void* vtkAOSDataArrayTemplate<ValueTypeT>::WriteVoidPointer(vtkIdType valueIdx, vtkIdType numValues)
{
  return this->WritePointer(valueIdx, numValues);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
typename vtkAOSDataArrayTemplate<ValueTypeT>::ValueType*
vtkAOSDataArrayTemplate<ValueTypeT>::GetPointer(vtkIdType valueIdx)
{
  return this->Buffer->GetBuffer() + valueIdx;
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void* vtkAOSDataArrayTemplate<ValueTypeT>::GetVoidPointer(vtkIdType valueIdx)
{
  return this->GetPointer(valueIdx);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
bool vtkAOSDataArrayTemplate<ValueTypeT>::AllocateTuples(vtkIdType numTuples)
{
  vtkIdType numValues = numTuples * this->GetNumberOfComponents();
  if (this->Buffer->Allocate(numValues))
  {
    this->Size = this->Buffer->GetSize();
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
bool vtkAOSDataArrayTemplate<ValueTypeT>::ReallocateTuples(vtkIdType numTuples)
{
  if (this->Buffer->Reallocate(numTuples * this->GetNumberOfComponents()))
  {
    this->Size = this->Buffer->GetSize();
    return true;
  }
  return false;
}

VTK_ABI_NAMESPACE_END
#endif // header guard
