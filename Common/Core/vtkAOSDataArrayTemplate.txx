/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAOSDataArrayTemplate.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkAOSDataArrayTemplate_txx
#define vtkAOSDataArrayTemplate_txx

#include "vtkAOSDataArrayTemplate.h"

#include "vtkArrayIteratorTemplate.h"

//-----------------------------------------------------------------------------
template <class ValueTypeT>
vtkAOSDataArrayTemplate<ValueTypeT>*
vtkAOSDataArrayTemplate<ValueTypeT>::New()
{
  VTK_STANDARD_NEW_BODY(vtkAOSDataArrayTemplate<ValueType>);
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
  this->SetArray(NULL, 0, 0);
  this->Buffer->Delete();
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>
::SetArray(ValueType* array, vtkIdType size, int save, int deleteMethod)
{
  this->Buffer->SetBuffer(array, size, save != 0, deleteMethod);
  this->Size = size;
  this->MaxId = this->Size - 1;
  this->DataChanged();
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>
::SetArray(ValueType *array, vtkIdType size, int save)
{
  this->SetArray(array, size, save, VTK_DATA_ARRAY_FREE);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>
::SetVoidArray(void *array, vtkIdType size, int save)
{
  this->SetArray(static_cast<ValueType*>(array), size, save);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>
::SetVoidArray(void *array, vtkIdType size, int save, int deleteMethod)
{
  this->SetArray(static_cast<ValueType*>(array), size, save, deleteMethod);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
vtkArrayIterator* vtkAOSDataArrayTemplate<ValueTypeT>::NewIterator()
{
  vtkArrayIterator *iter = vtkArrayIteratorTemplate<ValueType>::New();
  iter->Initialize(this);
  return iter;
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::ShallowCopy(vtkDataArray *other)
{
  SelfType *o = SelfType::FastDownCast(other);
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
      this->Buffer->Register(NULL);
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
    vtkIdType dstStart, vtkIdType n, vtkIdType srcStart,
    vtkAbstractArray *source)
{
  // First, check for the common case of typeid(source) == typeid(this). This
  // way we don't waste time redoing the other checks in the superclass, and
  // can avoid doing a dispatch for the most common usage of this method.
  SelfType *other = vtkArrayDownCast<SelfType>(source);
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
                  << other->GetNumberOfComponents() << " Dest: "
                  << this->GetNumberOfComponents());
    return;
  }

  vtkIdType maxSrcTupleId = srcStart + n - 1;
  vtkIdType maxDstTupleId = dstStart + n - 1;

  if (maxSrcTupleId >= other->GetNumberOfTuples())
  {
    vtkErrorMacro("Source array too small, requested tuple at index "
                  << maxSrcTupleId << ", but there are only "
                  << other->GetNumberOfTuples() << " tuples in the array.");
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

  ValueType *srcBegin = other->GetPointer(srcStart * numComps);
  ValueType *srcEnd = srcBegin + (n * numComps);
  ValueType *dstBegin = this->GetPointer(dstStart * numComps);

  std::copy(srcBegin, srcEnd, dstBegin);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
typename vtkAOSDataArrayTemplate<ValueTypeT>::ValueType*
vtkAOSDataArrayTemplate<ValueTypeT>
::WritePointer(vtkIdType valueIdx, vtkIdType numValues)
{
  vtkIdType newSize = valueIdx + numValues;
  if (newSize > this->Size)
  {
    if (!this->Resize(newSize / this->NumberOfComponents + 1))
    {
      return NULL;
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
void* vtkAOSDataArrayTemplate<ValueTypeT>
::WriteVoidPointer(vtkIdType valueIdx, vtkIdType numValues)
{
  return this->WritePointer(valueIdx, numValues);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
typename vtkAOSDataArrayTemplate<ValueTypeT>::ValueType *
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

// Deprecated API:
#ifndef VTK_LEGACY_REMOVE

//------------------------------------------------------------------------------
template <typename ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::GetTupleValue(vtkIdType tupleIdx,
                                                        ValueType *tuple)
{
  VTK_LEGACY_REPLACED_BODY(vtkAOSDataArrayTemplate::GetTupleValue, "VTK 7.1",
                           vtkGenericDataArray::GetTypedTuple);
  this->GetTypedTuple(tupleIdx, tuple);
}

//------------------------------------------------------------------------------
template <typename ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::SetTupleValue(vtkIdType tupleIdx,
                                                        const ValueType *tuple)
{
  VTK_LEGACY_REPLACED_BODY(vtkAOSDataArrayTemplate::SetTupleValue, "VTK 7.1",
                           vtkGenericDataArray::SetTypedTuple);
  this->SetTypedTuple(tupleIdx, tuple);
}

//------------------------------------------------------------------------------
template <typename ValueTypeT>
void vtkAOSDataArrayTemplate<ValueTypeT>::
InsertTupleValue(vtkIdType tupleIdx, const ValueType *tuple)
{
  VTK_LEGACY_REPLACED_BODY(vtkAOSDataArrayTemplate::InsertTupleValue, "VTK 7.1",
                           vtkGenericDataArray::InsertTypedTuple);
  this->InsertTypedTuple(tupleIdx, tuple);
}

//------------------------------------------------------------------------------
template <typename ValueTypeT>
vtkIdType vtkAOSDataArrayTemplate<ValueTypeT>::
InsertNextTupleValue(const ValueType *tuple)
{
  VTK_LEGACY_REPLACED_BODY(vtkAOSDataArrayTemplate::InsertNextTupleValue,
                           "VTK 7.1",
                           vtkGenericDataArray::InsertNextTypedTuple);
  return this->InsertNextTypedTuple(tuple);
}

#endif // VTK_LEGACY_REMOVE

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

#endif // header guard
