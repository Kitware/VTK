/*==============================================================================

  Program:   Visualization Toolkit
  Module:    vtkMappedDataArray.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

==============================================================================*/

#ifndef __vtkMappedDataArray_txx
#define __vtkMappedDataArray_txx

#include "vtkMappedDataArray.h"

#include "vtkVariant.h" // for vtkVariant

//------------------------------------------------------------------------------
template<class Scalar>
vtkMappedDataArray<Scalar>::vtkMappedDataArray()
{
  this->TemporaryScalarPointer = NULL;
  this->TemporaryScalarPointerSize = 0;
}

//------------------------------------------------------------------------------
template<class Scalar>
vtkMappedDataArray<Scalar>::~vtkMappedDataArray()
{
  if (this->TemporaryScalarPointer)
  {
    delete [] this->TemporaryScalarPointer;
    this->TemporaryScalarPointer = NULL;
    this->TemporaryScalarPointerSize = 0;
  }
}

//------------------------------------------------------------------------------
template<class Scalar>
void * vtkMappedDataArray<Scalar>::GetVoidPointer(vtkIdType id)
{
  vtkWarningMacro(<<"GetVoidPointer called. This is very expensive for "
                  "vtkMappedDataArray subclasses, since the scalar array must "
                  "be generated for each call. Consider using "
                  "a vtkTypedDataArrayIterator instead.");
  size_t numValues = this->NumberOfComponents * this->GetNumberOfTuples();

  if (this->TemporaryScalarPointer &&
      this->TemporaryScalarPointerSize != numValues)
    {
    delete [] this->TemporaryScalarPointer;
    this->TemporaryScalarPointer = NULL;
    this->TemporaryScalarPointerSize = 0;
    }

  if (!this->TemporaryScalarPointer)
    {
    this->TemporaryScalarPointer = new Scalar[numValues];
    this->TemporaryScalarPointerSize = numValues;
    }

  this->ExportToVoidPointer(static_cast<void*>(this->TemporaryScalarPointer));

  return static_cast<void*>(this->TemporaryScalarPointer + id);
}

//------------------------------------------------------------------------------
template<class Scalar>
void vtkMappedDataArray<Scalar>::ExportToVoidPointer(void *voidPtr)
{
  vtkTypedDataArrayIterator<Scalar> begin(this, 0);
  vtkTypedDataArrayIterator<Scalar> end =
      begin + (this->NumberOfComponents * this->GetNumberOfTuples());

  Scalar *ptr = static_cast<Scalar*>(voidPtr);

  while (begin != end)
    {
    *ptr = *begin;
    ++begin;
    ++ptr;
    }
}

//------------------------------------------------------------------------------
template<class Scalar>
void vtkMappedDataArray<Scalar>::SetVoidArray(void *, vtkIdType, int)
{
  vtkErrorMacro(<<"SetVoidArray not supported for vtkMappedDataArray "
                "subclasses.");
  return;
}

//------------------------------------------------------------------------------
template<class Scalar>
void vtkMappedDataArray<Scalar>::DataChanged()
{
  if (!this->TemporaryScalarPointer)
    {
    vtkWarningMacro(<<"DataChanged called, but no scalar pointer available.");
    return;
    }

  vtkTypedDataArrayIterator<Scalar> begin(this, 0);
  vtkTypedDataArrayIterator<Scalar> end =
      begin + this->TemporaryScalarPointerSize;

  Scalar *ptr = this->TemporaryScalarPointer;

  while (begin != end)
    {
    *begin = *ptr;
    ++begin;
    ++ptr;
    }

  this->Modified();
}

//------------------------------------------------------------------------------
template<class Scalar> inline vtkMappedDataArray<Scalar>*
vtkMappedDataArray<Scalar>::FastDownCast(vtkAbstractArray *source)
{
  switch (source->GetArrayType())
    {
    case vtkAbstractArray::MappedDataArray:
      if (source->GetDataType() == vtkTypeTraits<Scalar>::VTK_TYPE_ID)
        {
        return static_cast<vtkMappedDataArray<Scalar>*>(source);
        }
    default:
      return NULL;
    }
}

//------------------------------------------------------------------------------
template<class Scalar>
void vtkMappedDataArray<Scalar>::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "TemporaryScalarPointer: "
     << this->TemporaryScalarPointer << "\n";
  os << indent << "TemporaryScalarPointerSize: "
     << this->TemporaryScalarPointerSize << "\n";
}

//------------------------------------------------------------------------------
template<class Scalar>
void vtkMappedDataArray<Scalar>::Modified()
{
  this->vtkTypedDataArray<Scalar>::Modified();

  if (this->TemporaryScalarPointer == NULL)
    {
    return;
    }

  delete [] this->TemporaryScalarPointer;
  this->TemporaryScalarPointer = NULL;
  this->TemporaryScalarPointerSize = 0;
}

#endif //__vtkMappedDataArray_txx
