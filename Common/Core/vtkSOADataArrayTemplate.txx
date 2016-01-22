/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSOADataArrayTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSOADataArrayTemplate_txx
#define vtkSOADataArrayTemplate_txx

#include "vtkSOADataArrayTemplate.h"

#include "vtkArrayIteratorTemplate.h"
#include "vtkBuffer.h"

#include <cassert>

//-----------------------------------------------------------------------------
template<class ValueType>
vtkSOADataArrayTemplate<ValueType>*
vtkSOADataArrayTemplate<ValueType>::New()
{
  VTK_STANDARD_NEW_BODY(vtkSOADataArrayTemplate<ValueType>);
}

//-----------------------------------------------------------------------------
template<class ValueType>
vtkSOADataArrayTemplate<ValueType>::vtkSOADataArrayTemplate()
  : AoSCopy(NULL),
    Resizeable(true),
    NumberOfComponentsReciprocal(1.0)
{
}

//-----------------------------------------------------------------------------
template<class ValueType>
vtkSOADataArrayTemplate<ValueType>::~vtkSOADataArrayTemplate()
{
  for (size_t cc = 0; cc < this->Data.size(); ++cc)
    {
    this->Data[cc]->Delete();
    }
  this->Data.clear();
  if (this->AoSCopy)
    {
    this->AoSCopy->Delete();
    this->AoSCopy = NULL;
    }
}

//-----------------------------------------------------------------------------
template<class ValueType>
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
  this->NumberOfComponentsReciprocal = 1.0 / this->NumberOfComponents;
}

//-----------------------------------------------------------------------------
template<class ValueType>
vtkArrayIterator* vtkSOADataArrayTemplate<ValueType>::NewIterator()
{
  vtkArrayIterator *iter = vtkArrayIteratorTemplate<ValueType>::New();
  iter->Initialize(this);
  return iter;
}

//-----------------------------------------------------------------------------
template<class ValueType>
void vtkSOADataArrayTemplate<ValueType>::ShallowCopy(vtkDataArray *other)
{
  SelfType *o = SelfType::FastDownCast(other);
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
      vtkBuffer<ValueType> *thisBuffer = this->Data[cc];
      vtkBuffer<ValueType> *otherBuffer = o->Data[cc];
      if (thisBuffer != otherBuffer)
        {
        thisBuffer->Delete();
        this->Data[cc] = otherBuffer;
        otherBuffer->Register(NULL);
        }
      }
    this->DataChanged();
    }
  else
    {
    this->Superclass::ShallowCopy(other);
    }
}

//-----------------------------------------------------------------------------
template<class ValueType>
void vtkSOADataArrayTemplate<ValueType>::SetArray(int comp, ValueType* array,
                                                  vtkIdType size,
                                                  bool updateMaxId,
                                                  bool save, int deleteMethod)
{
  const int numComps = this->GetNumberOfComponents();
  if (comp >= numComps || comp < 0)
    {
    vtkErrorMacro("Invalid component number '" << comp << "' specified. "
      "Use `SetNumberOfComponents` first to set the number of components.");
    return;
    }

  this->Data[comp]->SetBuffer(array, size, save, deleteMethod);
  if (updateMaxId)
    {
    this->Size = numComps * size;
    this->MaxId = this->Size - 1;
    }
  this->DataChanged();
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
    return NULL;
    }

  return this->Data[comp]->GetBuffer();
}

//-----------------------------------------------------------------------------
template<class ValueType>
bool vtkSOADataArrayTemplate<ValueType>::AllocateTuples(vtkIdType numTuples)
{
  if (!this->Resizeable)
    {
    vtkIdType minTuples = VTK_ID_MAX;
    for (size_t cc = 0, max = this->Data.size(); cc < max; cc++)
      {
      minTuples = std::min(minTuples, this->Data[cc]->GetSize());
      }
    if (numTuples <= minTuples)
      {
      return true;
      }
    else
      {
      vtkErrorMacro("AllocateTuples cannot be called on a non-resizeable "
                    "array!");
      return false;
      }
    }

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
template<class ValueType>
bool vtkSOADataArrayTemplate<ValueType>::ReallocateTuples(vtkIdType numTuples)
{
  if (!this->Resizeable)
    {
    vtkIdType minTuples = VTK_ID_MAX;
    for (size_t cc = 0, max = this->Data.size(); cc < max; cc++)
      {
      minTuples = std::min(minTuples, this->Data[cc]->GetSize());
      }
    if (numTuples <= minTuples)
      {
      return true;
      }
    vtkErrorMacro("Resize attempted on a non-resizable array!");
    return false;
    }

  for (size_t cc = 0, max = this->Data.size(); cc < max; ++cc)
    {
    if (!this->Data[cc]->Reallocate(numTuples))
      {
      return false;
      }
    }
  return true;
}

//-----------------------------------------------------------------------------
template<class ValueType>
void *vtkSOADataArrayTemplate<ValueType>::GetVoidPointer(vtkIdType id)
{
  // Allow warnings to be silenced:
  const char *silence = getenv("VTK_SILENCE_GET_VOID_POINTER_WARNINGS");
  if (!silence)
    {
    vtkWarningMacro(<<"GetVoidPointer called. This is very expensive for "
                      "non-array-of-structs subclasses, as the scalar array "
                      "must be generated for each call. Using the "
                      "vtkGenericDataArray API with vtkArrayDispatch are "
                      "preferred. Define the environment variable "
                      "VTK_SILENCE_GET_VOID_POINTER_WARNINGS to silence "
                      "this warning.");
    }

  size_t numValues = this->GetNumberOfValues();

  if (!this->AoSCopy)
    {
    this->AoSCopy = vtkBuffer<ValueType>::New();
    }

  if (!this->AoSCopy->Allocate(numValues))
    {
    vtkErrorMacro(<<"Error allocating a buffer of " << numValues << " '"
                  << this->GetDataTypeAsString() << "' elements.");
    return NULL;
    }

  this->ExportToVoidPointer(static_cast<void*>(this->AoSCopy->GetBuffer()));

  return static_cast<void*>(this->AoSCopy->GetBuffer() + id);
}

//-----------------------------------------------------------------------------
template<class ValueType>
void vtkSOADataArrayTemplate<ValueType>::ExportToVoidPointer(void *voidPtr)
{
  vtkIdType numTuples = this->GetNumberOfTuples();
  if (this->NumberOfComponents * numTuples == 0)
    {
    // Nothing to do.
    return;
    }

  if (!voidPtr)
    {
    vtkErrorMacro(<< "Buffer is NULL.");
    return;
    }

  ValueType *ptr = static_cast<ValueType*>(voidPtr);
  for (vtkIdType t = 0; t < numTuples; ++t)
    {
    for (int c = 0; c < this->NumberOfComponents; ++c)
      {
      *ptr++ = this->Data[c]->GetBuffer()[t];
      }
    }
}

#endif
