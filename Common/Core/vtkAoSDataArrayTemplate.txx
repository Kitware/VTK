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
  this->Array = NULL;
  this->SaveUserArray = false;
  this->DeleteMethod = VTK_DATA_ARRAY_FREE;
}

//-----------------------------------------------------------------------------
template <class ScalarType>
vtkAoSDataArrayTemplate<ScalarType>::~vtkAoSDataArrayTemplate()
{
  this->SetArray(NULL, 0, 0);
}

//-----------------------------------------------------------------------------
vtkAoSDataArrayTemplateT(void)::SetArray(
  ScalarType* array, vtkIdType size, int save, int deleteMethod)
{
  if (this->Array != array && this->Array)
    {
    if (this->SaveUserArray == false)
      {
      if (this->DeleteMethod == VTK_DATA_ARRAY_FREE)
        {
        free(this->Array);
        }
      else
        {
        delete [] this->Array;
        }
      }
    this->Array = array;
    }
  this->Size = size;
  this->MaxId = this->Size - 1;
  this->DeleteMethod = deleteMethod;
  this->SaveUserArray = static_cast<bool>(save);
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
  return this->Array + id;
}

//-----------------------------------------------------------------------------
vtkAoSDataArrayTemplateT(bool)::AllocateTuples(vtkIdType numTuples)
{
  // Release old memory.
  this->SetArray(NULL, 0, 0);
  if (numTuples > 0)
    {
    vtkIdType numValues = numTuples * this->GetNumberOfComponents();
    ScalarType* newArray = static_cast<ScalarType*>(malloc(numValues*sizeof(ScalarType)));
    if (!newArray)
      {
      return false;
      }
    this->SetArray(newArray, numValues, false, VTK_DATA_ARRAY_FREE);
    }
  return true;
}

//-----------------------------------------------------------------------------
vtkAoSDataArrayTemplateT(bool)::ReallocateTuples(vtkIdType numTuples)
{
  // Trivial case:
  if (numTuples == 0)
    {
    return this->AllocateTuples(0);
    }

  assert(numTuples > 0);

  // OS X's realloc does not free memory if the new block is smaller.  This
  // is a very serious problem and causes huge amount of memory to be
  // wasted. Do not use realloc on the Mac.
  bool dontUseRealloc=false;
#if defined __APPLE__
  dontUseRealloc=true;
#endif

  vtkIdType numValues = numTuples * this->GetNumberOfComponents();
  if (this->Array &&
    (this->SaveUserArray || this->DeleteMethod == VTK_DATA_ARRAY_DELETE || dontUseRealloc))
    {
    ScalarType* newArray = static_cast<ScalarType*>(malloc(numValues*sizeof(ScalarType)));
    if (!newArray)
      {
      return false;
      }
    std::copy(this->Array, this->Array + std::min(this->Size, numValues), newArray);
    // now save the new array and release the old one too.
    this->SetArray(newArray, numValues, 0, VTK_DATA_ARRAY_FREE);
    }
  else
    {
    // Try to reallocate with minimal memory usage and possibly avoid
    // copying.
    this->Array = static_cast<ScalarType*>(realloc(this->Array, numValues*sizeof(ScalarType)));
    if (!this->Array)
      {
      return false;
      }
    }
  return true;
}
