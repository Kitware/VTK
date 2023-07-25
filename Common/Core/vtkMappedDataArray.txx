// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkMappedDataArray_txx
#define vtkMappedDataArray_txx

#include "vtkMappedDataArray.h"

#include "vtkVariant.h" // for vtkVariant

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
template <class Scalar>
vtkMappedDataArray<Scalar>::vtkMappedDataArray()
{
  this->TemporaryScalarPointer = nullptr;
  this->TemporaryScalarPointerSize = 0;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkMappedDataArray<Scalar>::~vtkMappedDataArray()
{
  delete[] this->TemporaryScalarPointer;
  this->TemporaryScalarPointer = nullptr;
  this->TemporaryScalarPointerSize = 0;
}

//------------------------------------------------------------------------------
template <class Scalar>
void* vtkMappedDataArray<Scalar>::GetVoidPointer(vtkIdType id)
{
  vtkWarningMacro(<< "GetVoidPointer called. This is very expensive for "
                     "vtkMappedDataArray subclasses, since the scalar array must "
                     "be generated for each call. Consider using "
                     "a vtkTypedDataArrayIterator instead.");
  size_t numValues = this->NumberOfComponents * this->GetNumberOfTuples();

  if (this->TemporaryScalarPointer && this->TemporaryScalarPointerSize != numValues)
  {
    delete[] this->TemporaryScalarPointer;
    this->TemporaryScalarPointer = nullptr;
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
template <class Scalar>
void vtkMappedDataArray<Scalar>::ExportToVoidPointer(void* voidPtr)
{
  vtkTypedDataArrayIterator<Scalar> begin(this, 0);
  vtkTypedDataArrayIterator<Scalar> end =
    begin + (this->NumberOfComponents * this->GetNumberOfTuples());

  Scalar* ptr = static_cast<Scalar*>(voidPtr);

  while (begin != end)
  {
    *ptr = *begin;
    ++begin;
    ++ptr;
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMappedDataArray<Scalar>::SetVoidArray(void*, vtkIdType, int)
{
  vtkErrorMacro(<< "SetVoidArray not supported for vtkMappedDataArray "
                   "subclasses.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMappedDataArray<Scalar>::SetVoidArray(void*, vtkIdType, int, int)
{
  vtkErrorMacro(<< "SetVoidArray not supported for vtkMappedDataArray "
                   "subclasses.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMappedDataArray<Scalar>::DataChanged()
{
  if (!this->TemporaryScalarPointer)
  {
    vtkWarningMacro(<< "DataChanged called, but no scalar pointer available.");
    return;
  }

  vtkTypedDataArrayIterator<Scalar> begin(this, 0);
  vtkTypedDataArrayIterator<Scalar> end = begin + this->TemporaryScalarPointerSize;

  Scalar* ptr = this->TemporaryScalarPointer;

  while (begin != end)
  {
    *begin = *ptr;
    ++begin;
    ++ptr;
  }

  this->Modified();
}

//------------------------------------------------------------------------------
template <class Scalar>
inline vtkMappedDataArray<Scalar>* vtkMappedDataArray<Scalar>::FastDownCast(
  vtkAbstractArray* source)
{
  if (source)
  {
    switch (source->GetArrayType())
    {
      case vtkAbstractArray::MappedDataArray:
        if (vtkDataTypesCompare(source->GetDataType(), vtkTypeTraits<Scalar>::VTK_TYPE_ID))
        {
          return static_cast<vtkMappedDataArray<Scalar>*>(source);
        }
      default:
        break;
    }
  }
  return nullptr;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMappedDataArray<Scalar>::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "TemporaryScalarPointer: " << this->TemporaryScalarPointer << "\n";
  os << indent << "TemporaryScalarPointerSize: " << this->TemporaryScalarPointerSize << "\n";
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkMappedDataArray<Scalar>::Modified()
{
  this->vtkTypedDataArray<Scalar>::Modified();

  if (this->TemporaryScalarPointer == nullptr)
  {
    return;
  }

  delete[] this->TemporaryScalarPointer;
  this->TemporaryScalarPointer = nullptr;
  this->TemporaryScalarPointerSize = 0;
}

VTK_ABI_NAMESPACE_END
#endif // vtkMappedDataArray_txx
