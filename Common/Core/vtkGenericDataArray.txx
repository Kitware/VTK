/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataArray.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkGenericDataArray_txx
#define vtkGenericDataArray_txx

#include "vtkGenericDataArray.h"

#include "vtkObjectFactory.h"
#include "vtkIdList.h"
#include "vtkVariantCast.h"

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
double* vtkGenericDataArray<DerivedT, ValueTypeT>::GetTuple(vtkIdType tupleIdx)
{
  assert(!this->LegacyTuple.empty() && "Number of components is nonzero.");
  this->GetTuple(tupleIdx, &this->LegacyTuple[0]);
  return &this->LegacyTuple[0];
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>
::GetTuple(vtkIdType tupleIdx, double *tuple)
{
  for (int c = 0; c < this->NumberOfComponents; ++c)
    {
    tuple[c] = static_cast<double>(this->GetTypedComponent(tupleIdx, c));
    }
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>
::SetComponent(vtkIdType tupleIdx, int compIdx, double value)
{
  // Reimplemented for efficiency (base impl allocates heap memory)
  this->SetTypedComponent(tupleIdx, compIdx, static_cast<ValueType>(value));
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
double vtkGenericDataArray<DerivedT, ValueTypeT>
::GetComponent(vtkIdType tupleIdx, int compIdx)
{
  // Reimplemented for efficiency (base impl allocates heap memory)
  return static_cast<double>(this->GetTypedComponent(tupleIdx, compIdx));
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>::RemoveTuple(vtkIdType id)
{
  if (id < 0 || id >= this->GetNumberOfTuples())
    {
    // Nothing to be done
    return;
    }
  if (id == (this->GetNumberOfTuples() - 1))
    {
    // To remove last item, just decrease the size by one
    this->RemoveLastTuple();
    return;
    }

  // This is a very slow implementation since it uses generic API. Subclasses
  // are encouraged to provide a faster implementation.
  assert(((this->GetNumberOfTuples() - id) - 1) /* (length) */ > 0);

  int numComps = this->GetNumberOfComponents();
  vtkIdType fromTuple = id + 1;
  vtkIdType toTuple = id;
  vtkIdType endTuple = this->GetNumberOfTuples();
  for (; fromTuple != endTuple; ++toTuple, ++fromTuple)
    {
    for (int comp=0; comp < numComps; ++comp)
      {
      this->SetTypedComponent(toTuple, comp,
                              this->GetTypedComponent(fromTuple, comp));
      }
    }
  this->SetNumberOfTuples(this->GetNumberOfTuples() - 1);
  this->DataChanged();
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>
::SetVoidArray(void*, vtkIdType, int)
{
  vtkErrorMacro("SetVoidArray is not supported by this class.");
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>
::SetVoidArray(void*, vtkIdType, int, int)
{
  vtkErrorMacro("SetVoidArray is not supported by this class.");
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void* vtkGenericDataArray<DerivedT, ValueTypeT>
::WriteVoidPointer(vtkIdType, vtkIdType)
{
  vtkErrorMacro("WriteVoidPointer is not supported by this class.");
  return NULL;
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
typename vtkGenericDataArray<DerivedT, ValueTypeT>::ValueType *
vtkGenericDataArray<DerivedT, ValueTypeT>
::WritePointer(vtkIdType id, vtkIdType number)
{
  return static_cast<ValueType*>(this->WriteVoidPointer(id, number));
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
int vtkGenericDataArray<DerivedT, ValueTypeT>::GetDataType()
{
  return vtkTypeTraits<ValueType>::VTK_TYPE_ID;
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
int vtkGenericDataArray<DerivedT, ValueTypeT>::GetDataTypeSize()
{
  return static_cast<int>(sizeof(ValueType));
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
bool vtkGenericDataArray<DerivedT, ValueTypeT>::HasStandardMemoryLayout()
{
  // False by default, AoS should set true.
  return false;
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void* vtkGenericDataArray<DerivedT, ValueTypeT>::GetVoidPointer(vtkIdType)
{
  vtkErrorMacro("GetVoidPointer is not supported by this class.");
  return NULL;
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
typename vtkGenericDataArray<DerivedT, ValueTypeT>::ValueType *
vtkGenericDataArray<DerivedT, ValueTypeT>::GetPointer(vtkIdType id)
{
  return static_cast<ValueType*>(this->GetVoidPointer(id));
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
vtkIdType vtkGenericDataArray<DerivedT, ValueTypeT>
::LookupValue(vtkVariant valueVariant)
{
  bool valid = true;
  ValueType value = vtkVariantCast<ValueType>(valueVariant, &valid);
  if (valid)
    {
    return this->LookupTypedValue(value);
    }
  return -1;
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
vtkIdType vtkGenericDataArray<DerivedT, ValueTypeT>
::LookupTypedValue(ValueType value)
{
  return this->Lookup.LookupValue(value);
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>
::LookupValue(vtkVariant valueVariant, vtkIdList* ids)
{
  ids->Reset();
  bool valid = true;
  ValueType value = vtkVariantCast<ValueType>(valueVariant, &valid);
  if (valid)
    {
    this->LookupTypedValue(value, ids);
    }
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>
::LookupTypedValue(ValueType value, vtkIdList* ids)
{
  ids->Reset();
  this->Lookup.LookupValue(value, ids);
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>::ClearLookup()
{
  this->Lookup.ClearLookup();
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>::DataChanged()
{
  this->Lookup.ClearLookup();
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>
::SetVariantValue(vtkIdType valueIdx, vtkVariant valueVariant)
{
  bool valid = true;
  ValueType value = vtkVariantCast<ValueType>(valueVariant, &valid);
  if (valid)
    {
    this->SetValue(valueIdx, value);
    }
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
vtkVariant vtkGenericDataArray<DerivedT, ValueTypeT>
::GetVariantValue(vtkIdType valueIdx)
{
  return vtkVariant(this->GetValue(valueIdx));
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>
::InsertVariantValue(vtkIdType valueIdx, vtkVariant valueVariant)
{
  bool valid = true;
  ValueType value = vtkVariantCast<ValueType>(valueVariant, &valid);
  if (valid)
    {
    this->InsertValue(valueIdx, value);
    }
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
int vtkGenericDataArray<DerivedT, ValueTypeT>
::Allocate(vtkIdType size, vtkIdType vtkNotUsed(ext))
{
  // Allocator must updated this->Size and this->MaxId properly.
  this->MaxId = -1;
  if (size > this->Size || size == 0)
    {
    this->Size = 0;

    // let's keep the size an integral multiple of the number of components.
    size = size < 0 ? 0 : size;
    int numComps = this->GetNumberOfComponents() > 0
        ? this->GetNumberOfComponents() : 1;
    vtkIdType numTuples = ceil(size / static_cast<double>(numComps));
    // NOTE: if numTuples is 0, AllocateTuples is expected to release the
    // memory.
    if (this->AllocateTuples(numTuples) == false)
      {
      vtkErrorMacro("Unable to allocate " << size
                    << " elements of size " << sizeof(ValueType)
                    << " bytes. ");
#if !defined NDEBUG
      // We're debugging, crash here preserving the stack
      abort();
#elif !defined VTK_DONT_THROW_BAD_ALLOC
      // We can throw something that has universal meaning
      throw std::bad_alloc();
#else
      // We indicate that alloc failed by return
      return 0;
#endif
      }
    this->Size = numTuples * numComps;
    }
  this->DataChanged();
  return 1;
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
int vtkGenericDataArray<DerivedT, ValueTypeT>::Resize(vtkIdType numTuples)
{
  int numComps = this->GetNumberOfComponents();
  vtkIdType curNumTuples = this->Size / (numComps> 0? numComps : 1);
  if (numTuples > curNumTuples)
    {
    // Requested size is bigger than current size.  Allocate enough
    // memory to fit the requested size and be more than double the
    // currently allocated memory.
    numTuples = curNumTuples + numTuples;
    }
  else if (numTuples == curNumTuples)
    {
    return 1;
    }
  else
    {
    // Requested size is smaller than current size.  Squeeze the
    // memory.
    this->DataChanged();
    }

  assert(numTuples >= 0);

  if (!this->ReallocateTuples(numTuples))
    {
    vtkErrorMacro("Unable to allocate " << numTuples * numComps
                  << " elements of size " << sizeof(ValueType)
                  << " bytes. ");
#if !defined NDEBUG
    // We're debugging, crash here preserving the stack
    abort();
#elif !defined VTK_DONT_THROW_BAD_ALLOC
    // We can throw something that has universal meaning
    throw std::bad_alloc();
#else
    // We indicate that malloc failed by return
    return 0;
#endif
    }

  // Allocation was successful. Save it.
  this->Size = numTuples * numComps;

  // Update MaxId if we truncated:
  if ((this->Size - 1) < this->MaxId)
    {
    this->MaxId = (this->Size - 1);
    }

  return 1;
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>::SetNumberOfComponents(int num)
{
  this->vtkDataArray::SetNumberOfComponents(num);
  this->LegacyTuple.resize(num);
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>::
SetNumberOfTuples(vtkIdType number)
{
  vtkIdType newSize = number * this->NumberOfComponents;
  if (this->Allocate(newSize, 0))
    {
    this->MaxId = newSize - 1;
    }
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>::Initialize()
{
  this->Resize(0);
  this->DataChanged();
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>::Squeeze()
{
  this->Resize(this->GetNumberOfTuples());
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>
::InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source)
{
  this->EnsureAccessToTuple(i);
  this->SetTuple(i, j, source);
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>
::InsertTuple(vtkIdType i, const float *source)
{
  this->EnsureAccessToTuple(i);
  this->SetTuple(i, source);
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>
::InsertTuple(vtkIdType i, const double *source)
{
  this->EnsureAccessToTuple(i);
  this->SetTuple(i, source);
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>
::InsertComponent(vtkIdType tupleIdx, int compIdx, double value)
{
  // Update MaxId to the inserted component (not the complete tuple) for
  // compatibility with InsertNextValue.
  vtkIdType newMaxId = tupleIdx * this->NumberOfComponents + compIdx;
  if (newMaxId < this->MaxId)
    {
    newMaxId = this->MaxId;
    }
  this->EnsureAccessToTuple(tupleIdx);
  assert("Sufficient space allocated." && this->MaxId >= newMaxId);
  this->MaxId = newMaxId;
  this->SetComponent(tupleIdx, compIdx, value);
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
vtkIdType vtkGenericDataArray<DerivedT, ValueTypeT>
::InsertNextTuple(vtkIdType srcTupleIdx, vtkAbstractArray *source)
{
  vtkIdType nextTuple = this->GetNumberOfTuples();
  this->InsertTuple(nextTuple, srcTupleIdx, source);
  return nextTuple;
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
vtkIdType vtkGenericDataArray<DerivedT, ValueTypeT>
::InsertNextTuple(const float *tuple)
{
  vtkIdType nextTuple = this->GetNumberOfTuples();
  this->InsertTuple(nextTuple, tuple);
  return nextTuple;
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
vtkIdType vtkGenericDataArray<DerivedT, ValueTypeT>
::InsertNextTuple(const double *tuple)
{
  vtkIdType nextTuple = this->GetNumberOfTuples();
  this->InsertTuple(nextTuple, tuple);
  return nextTuple;
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
vtkArrayIterator* vtkGenericDataArray<DerivedT, ValueTypeT>::NewIterator()
{
  vtkWarningMacro(<< "No vtkArrayIterator defined for " << this->GetClassName()
                  << " arrays.");
  return NULL;
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
vtkIdType vtkGenericDataArray<DerivedT, ValueTypeT>
::InsertNextValue(ValueType value)
{
  vtkIdType nextValueIdx = this->MaxId + 1;
  if (nextValueIdx >= this->Size)
    {
    vtkIdType tuple = nextValueIdx / this->NumberOfComponents;
    this->EnsureAccessToTuple(tuple);
    // Since EnsureAccessToTuple will update the MaxId to point to the last
    // component in the last tuple, we move it back to support this method on
    // multi-component arrays.
    this->MaxId = nextValueIdx;
    }

  // Extending array without needing to reallocate:
  if (this->MaxId < nextValueIdx)
    {
    this->MaxId = nextValueIdx;
    }

  this->SetValue(nextValueIdx, value);
  return nextValueIdx;
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>
::InsertValue(vtkIdType valueIdx, ValueType value)
{
  vtkIdType tuple = valueIdx / this->NumberOfComponents;
  // Update MaxId to the inserted component (not the complete tuple) for
  // compatibility with InsertNextValue.
  vtkIdType newMaxId = valueIdx > this->MaxId ? valueIdx : this->MaxId;
  if (this->EnsureAccessToTuple(tuple))
    {
    assert("Sufficient space allocated." && this->MaxId >= newMaxId);
    this->MaxId = newMaxId;
    this->SetValue(valueIdx, value);
    }
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>
::InsertTypedTuple(vtkIdType tupleIdx, const ValueType *t)
{
  if (this->EnsureAccessToTuple(tupleIdx))
    {
    this->SetTypedTuple(tupleIdx, t);
    }
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
vtkIdType vtkGenericDataArray<DerivedT, ValueTypeT>
::InsertNextTypedTuple(const ValueType *t)
{
  vtkIdType nextTuple = this->GetNumberOfTuples();
  this->InsertTypedTuple(nextTuple, t);
  return nextTuple;
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>
::InsertTypedComponent(vtkIdType tupleIdx, int compIdx, ValueType val)
{
  // Update MaxId to the inserted component (not the complete tuple) for
  // compatibility with InsertNextValue.
  vtkIdType newMaxId = tupleIdx * this->NumberOfComponents + compIdx;
  if (this->MaxId > newMaxId)
    {
    newMaxId = this->MaxId;
    }
  this->EnsureAccessToTuple(tupleIdx);
  assert("Sufficient space allocated." && this->MaxId >= newMaxId);
  this->MaxId = newMaxId;
  this->SetTypedComponent(tupleIdx, compIdx, val);
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
void vtkGenericDataArray<DerivedT, ValueTypeT>
::GetValueRange(ValueType range[2], int comp)
{
  // TODO This is how vtkDataArrayTemplate implemented this. It should be
  // reimplemented to avoid truncation of e.g. longer integers.
  double doubleRange[2];
  this->ComputeRange(doubleRange, comp);
  range[0] = static_cast<ValueType>(doubleRange[0]);
  range[1] = static_cast<ValueType>(doubleRange[1]);
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
typename vtkGenericDataArray<DerivedT, ValueTypeT>::ValueType *
vtkGenericDataArray<DerivedT, ValueTypeT>::GetValueRange(int comp)
{
  this->LegacyValueRange.resize(2);
  this->GetValueRange(&this->LegacyValueRange[0], comp);
  return &this->LegacyValueRange[0];
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
vtkGenericDataArray<DerivedT, ValueTypeT>::vtkGenericDataArray()
{
  // Initialize internal data structures:
  this->Lookup.SetArray(this);
  this->SetNumberOfComponents(this->NumberOfComponents);
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
vtkGenericDataArray<DerivedT, ValueTypeT>::~vtkGenericDataArray()
{
}

//-----------------------------------------------------------------------------
template <class DerivedT, class ValueTypeT>
bool vtkGenericDataArray<DerivedT, ValueTypeT>
::EnsureAccessToTuple(vtkIdType tupleIdx)
{
  if (tupleIdx < 0)
    {
    return false;
    }
  vtkIdType minSize = (1 + tupleIdx) * this->NumberOfComponents;
  vtkIdType expectedMaxId = minSize - 1;
  if (this->MaxId < expectedMaxId)
    {
    if (this->Size < minSize)
      {
      if (!this->Resize(tupleIdx + 1))
        {
        return false;
        }
      }
    this->MaxId = expectedMaxId;
    }
  return true;
}

#undef vtkGenericDataArrayT

#endif // header guard
