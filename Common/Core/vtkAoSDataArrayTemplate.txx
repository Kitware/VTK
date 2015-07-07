/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAoSDataArrayTemplate.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#define vtkAoSDataArrayTemplateT(returnType) \
  template <class ScalarType> \
  returnType vtkAoSDataArrayTemplate<ScalarType>

//-----------------------------------------------------------------------------
vtkAoSDataArrayTemplateT(vtkAoSDataArrayTemplate<ScalarType>*)::New()
{
  VTK_STANDARD_NEW_BODY(vtkAoSDataArrayTemplate<ScalarType>);
}

//-----------------------------------------------------------------------------
template <class ScalarType>
vtkAoSDataArrayTemplate<ScalarType>::vtkAoSDataArrayTemplate()
{
  this->SaveUserArray = false;
  this->DeleteMethod = VTK_DATA_ARRAY_FREE;
}

//-----------------------------------------------------------------------------
template <class ScalarType>
vtkAoSDataArrayTemplate<ScalarType>::~vtkAoSDataArrayTemplate()
{
  this->SetArray(NULL, 0, 0);
  this->Buffer.SetBuffer(NULL, 0);
}

//-----------------------------------------------------------------------------
vtkAoSDataArrayTemplateT(void)::SetArray(
  ScalarType* array, vtkIdType size, int save, int deleteMethod)
{
  this->Buffer.SetBuffer(array, size, save, deleteMethod);
  this->Size = size;
  this->MaxId = this->Size - 1;
  this->DataChanged();
}

//-----------------------------------------------------------------------------
template <class ScalarTypeT>
typename vtkAoSDataArrayTemplate<ScalarTypeT>::ScalarType*
vtkAoSDataArrayTemplate<ScalarTypeT>::WritePointer(vtkIdType id, vtkIdType number)
{
  vtkIdType newSize = (id+number)+1;
  if (newSize > this->Size)
    {
    if (!this->ReallocateTuples(newSize / this->NumberOfComponents))
      {
      return NULL;
      }
    this->MaxId = (newSize - 1);
    }
  this->DataChanged();
  return this->GetPointer(id);
}

//-----------------------------------------------------------------------------
vtkAoSDataArrayTemplateT(bool)::AllocateTuples(vtkIdType numTuples)
{
  vtkIdType numValues = numTuples * this->GetNumberOfComponents();
  return this->Buffer.Allocate(numValues);
}

//-----------------------------------------------------------------------------
vtkAoSDataArrayTemplateT(bool)::ReallocateTuples(vtkIdType numTuples)
{
  return this->Buffer.Reallocate(numTuples * this->GetNumberOfComponents());
}
