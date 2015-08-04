/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSoADataArrayTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSoADataArrayTemplate_txx
#define vtkSoADataArrayTemplate_txx

#include "vtkSoADataArrayTemplate.h"

#include "vtkArrayIteratorTemplate.h"
#include "vtkBuffer.h"

#include <cassert>

//-----------------------------------------------------------------------------
template<class ValueType>
vtkSoADataArrayTemplate<ValueType>*
vtkSoADataArrayTemplate<ValueType>::New()
{
  VTK_STANDARD_NEW_BODY(vtkSoADataArrayTemplate<ValueType>);
}

//-----------------------------------------------------------------------------
template<class ValueType>
vtkSoADataArrayTemplate<ValueType>::vtkSoADataArrayTemplate()
  : Resizeable(true),
  NumberOfComponentsReciprocal(1.0)
{
}

//-----------------------------------------------------------------------------
template<class ValueType>
vtkSoADataArrayTemplate<ValueType>::~vtkSoADataArrayTemplate()
{
  for (size_t cc=0; cc < this->Data.size(); ++cc)
    {
    this->Data[cc].SetBuffer(NULL, 0);
    }
}

//-----------------------------------------------------------------------------
template<class ValueType>
void vtkSoADataArrayTemplate<ValueType>::SetNumberOfComponents(int val)
{
  this->GenericDataArrayType::SetNumberOfComponents(val);
  size_t numComps = static_cast<size_t>(this->GetNumberOfComponents());
  assert(numComps >= 1);
  while (this->Data.size() > numComps)
    {
    this->Data.back().SetBuffer(NULL, 0);
    this->Data.pop_back();
    }
  this->Data.resize(numComps);
  this->NumberOfComponentsReciprocal = 1.0 / this->NumberOfComponents;
}

//-----------------------------------------------------------------------------
template<class ValueType>
vtkArrayIterator* vtkSoADataArrayTemplate<ValueType>::NewIterator()
{
  vtkArrayIterator *iter = vtkArrayIteratorTemplate<ValueType>::New();
  iter->Initialize(this);
  return iter;
}

//-----------------------------------------------------------------------------
template<class ValueType>
void vtkSoADataArrayTemplate<ValueType>::SetArray(int comp, ValueType* array,
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

  this->Data[comp].SetBuffer(array, size, save, deleteMethod);
  if (updateMaxId)
    {
    this->Size = numComps * size;
    this->MaxId = this->Size - 1;
    }
  this->DataChanged();
}

//-----------------------------------------------------------------------------
template <class ValueType>
typename vtkSoADataArrayTemplate<ValueType>::ValueType*
vtkSoADataArrayTemplate<ValueType>::GetComponentArrayPointer(int comp)
{
  const int numComps = this->GetNumberOfComponents();
  if (comp >= numComps || comp < 0)
    {
    vtkErrorMacro("Invalid component number '" << comp << "' specified.");
    return NULL;
    }

  return this->Data[comp].GetBuffer();
}

//-----------------------------------------------------------------------------
template<class ValueType>
bool vtkSoADataArrayTemplate<ValueType>::AllocateTuples(vtkIdType numTuples)
{
  if (!this->Resizeable)
    {
    vtkIdType minTuples = VTK_ID_MAX;
    for (int cc=0, max=this->Data.size(); cc < max; cc++)
      {
      minTuples = std::min(minTuples, this->Data[cc].GetSize());
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

  for (int cc=0, max=this->Data.size(); cc < max; ++cc)
    {
    if (!this->Data[cc].Allocate(numTuples))
      {
      return false;
      }
    }
  return true;
}

//-----------------------------------------------------------------------------
template<class ValueType>
bool vtkSoADataArrayTemplate<ValueType>::ReallocateTuples(vtkIdType numTuples)
{
  if (!this->Resizeable)
    {
    vtkIdType minTuples = VTK_ID_MAX;
    for (int cc=0, max=this->Data.size(); cc < max; cc++)
      {
      minTuples = std::min(minTuples, this->Data[cc].GetSize());
      }
    if (numTuples <= minTuples)
      {
      return true;
      }
    vtkErrorMacro("Resize attempted on a non-resizable array!");
    return false;
    }

  for (int cc=0, max=this->Data.size(); cc < max; ++cc)
    {
    if (!this->Data[cc].Reallocate(numTuples))
      {
      return false;
      }
    }
  return true;
}

//-----------------------------------------------------------------------------
template<class ValueType>
void *vtkSoADataArrayTemplate<ValueType>::GetVoidPointer(vtkIdType id)
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

  size_t numValues = this->NumberOfComponents * this->GetNumberOfTuples();

  if (!this->AoSCopy.Allocate(numValues))
    {
    vtkErrorMacro(<<"Error allocating a buffer of " << numValues << " '"
                  << this->GetDataTypeAsString() << "' elements.");
    return NULL;
    }

  this->ExportToVoidPointer(static_cast<void*>(this->AoSCopy.GetBuffer()));

  return static_cast<void*>(this->AoSCopy.GetBuffer() + id);
}

//-----------------------------------------------------------------------------
template<class ValueType>
void vtkSoADataArrayTemplate<ValueType>::ExportToVoidPointer(void *voidPtr)
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
      *ptr++ = this->Data[c].GetBuffer()[t];
      }
    }
}

#endif
