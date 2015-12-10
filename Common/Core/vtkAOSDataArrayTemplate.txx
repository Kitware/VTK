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

#define vtkAOSDataArrayTemplateT(returnType) \
  template <class ValueType> \
  returnType vtkAOSDataArrayTemplate<ValueType>

//-----------------------------------------------------------------------------
vtkAOSDataArrayTemplateT(vtkAOSDataArrayTemplate<ValueType>*)::New()
{
  VTK_STANDARD_NEW_BODY(vtkAOSDataArrayTemplate<ValueType>);
}

//-----------------------------------------------------------------------------
template <class ValueType>
vtkAOSDataArrayTemplate<ValueType>::vtkAOSDataArrayTemplate()
{
  this->SaveUserArray = false;
  this->DeleteMethod = VTK_DATA_ARRAY_FREE;
}

//-----------------------------------------------------------------------------
template <class ValueType>
vtkAOSDataArrayTemplate<ValueType>::~vtkAOSDataArrayTemplate()
{
  this->SetArray(NULL, 0, 0);
  this->Buffer.SetBuffer(NULL, 0);
}

//-----------------------------------------------------------------------------
vtkAOSDataArrayTemplateT(void)::SetArray(
  ValueType* array, vtkIdType size, int save, int deleteMethod)
{
  this->Buffer.SetBuffer(array, size, save != 0, deleteMethod);
  this->Size = size;
  this->MaxId = this->Size - 1;
  this->DataChanged();
}

//-----------------------------------------------------------------------------
vtkAOSDataArrayTemplateT(vtkArrayIterator*)::NewIterator()
{
  vtkArrayIterator *iter = vtkArrayIteratorTemplate<ValueType>::New();
  iter->Initialize(this);
  return iter;
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
typename vtkAOSDataArrayTemplate<ValueTypeT>::ValueType*
vtkAOSDataArrayTemplate<ValueTypeT>::WritePointer(vtkIdType id,
                                                  vtkIdType number)
{
  vtkIdType newSize = id + number;
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
  return this->GetPointer(id);
}

//-----------------------------------------------------------------------------
vtkAOSDataArrayTemplateT(bool)::AllocateTuples(vtkIdType numTuples)
{
  vtkIdType numValues = numTuples * this->GetNumberOfComponents();
  if (this->Buffer.Allocate(numValues))
    {
    this->Size = this->Buffer.GetSize();
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
vtkAOSDataArrayTemplateT(bool)::ReallocateTuples(vtkIdType numTuples)
{
  if (this->Buffer.Reallocate(numTuples * this->GetNumberOfComponents()))
    {
    this->Size = this->Buffer.GetSize();
    return true;
    }
  return false;
}

#endif // header guard
