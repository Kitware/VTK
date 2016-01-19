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

#define vtkGenericDataArrayT(returnType) \
  template <class DerivedT, class ValueTypeT> \
  returnType vtkGenericDataArray<DerivedT, ValueTypeT>

//-----------------------------------------------------------------------------
vtkGenericDataArrayT(double *)::GetTuple(vtkIdType i)
{
  assert(!this->LegacyTuple.empty() && "Number of components in nonzero.");
  this->GetTuple(i, &this->LegacyTuple[0]);
  return &this->LegacyTuple[0];
}

//-----------------------------------------------------------------------------
vtkGenericDataArrayT(void)::GetTuple(vtkIdType i, double *tuple)
{
  for (int c = 0; c < this->NumberOfComponents; ++c)
    {
    tuple[c] = static_cast<double>(this->GetComponentValue(i, c));
    }
}

//-----------------------------------------------------------------------------
vtkGenericDataArrayT(void)::RemoveTuple(vtkIdType id)
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
      this->SetComponentValue(toTuple, comp,
                              this->GetComponentValue(fromTuple, comp));
      }
    }
  this->SetNumberOfTuples(this->GetNumberOfTuples() - 1);
  this->DataChanged();
}

//-----------------------------------------------------------------------------
vtkGenericDataArrayT(void)::SetVoidArray(void*, vtkIdType, int)
{
  vtkErrorMacro("SetVoidArray is not supported by this class.");
}

//-----------------------------------------------------------------------------
vtkGenericDataArrayT(void)::SetVoidArray(void*, vtkIdType, int, int)
{
  vtkErrorMacro("SetVoidArray is not supported by this class.");
}

//-----------------------------------------------------------------------------
vtkGenericDataArrayT(void*)::WriteVoidPointer(vtkIdType, vtkIdType)
{
  vtkErrorMacro("WriteVoidPointer is not supported by this class.");
  return NULL;
}

//-----------------------------------------------------------------------------
vtkGenericDataArrayT(void*)::GetVoidPointer(vtkIdType)
{

  vtkErrorMacro("GetVoidPointer is not supported by this class.");
  return NULL;
}

//-----------------------------------------------------------------------------
vtkGenericDataArrayT(vtkIdType)::LookupValue(vtkVariant valueVariant)
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
vtkGenericDataArrayT(vtkIdType)::LookupTypedValue(ValueType value)
{
  return this->Lookup.LookupValue(value);
}

//-----------------------------------------------------------------------------
vtkGenericDataArrayT(void)::LookupValue(vtkVariant valueVariant, vtkIdList* ids)
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
vtkGenericDataArrayT(void)::LookupTypedValue(ValueType value, vtkIdList* ids)
{
  ids->Reset();
  this->Lookup.LookupValue(value, ids);
}

//-----------------------------------------------------------------------------
vtkGenericDataArrayT(void)::SetVariantValue(vtkIdType valueIdx,
                                            vtkVariant valueVariant)
{
  bool valid = true;
  ValueType value = vtkVariantCast<ValueType>(valueVariant, &valid);
  if (valid)
    {
    this->SetValue(valueIdx, value);
    }
}

//-----------------------------------------------------------------------------
vtkGenericDataArrayT(void)::InsertVariantValue(vtkIdType valueIdx,
                                               vtkVariant valueVariant)
{
  bool valid = true;
  ValueType value = vtkVariantCast<ValueType>(valueVariant, &valid);
  if (valid)
    {
    this->InsertValue(valueIdx, value);
    }
}

//-----------------------------------------------------------------------------
vtkGenericDataArrayT(int)::Allocate(vtkIdType size, vtkIdType vtkNotUsed(ext))
{
  DerivedT* self = static_cast<DerivedT*>(this);

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
    if (self->AllocateTuples(numTuples) == false)
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
vtkGenericDataArrayT(vtkArrayIterator*)::NewIterator()
{
  vtkWarningMacro(<< "No vtkArrayIterator defined for " << this->GetClassName()
                  << " arrays.");
  return NULL;
}

#undef vtkGenericDataArrayT

#endif // header guard
